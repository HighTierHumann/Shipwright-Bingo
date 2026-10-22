#include "BingoOAuth.h"
#include "BingoClient.h"

#include "soh/cvar_prefixes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>

#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

extern "C" {
extern const char gBuildVersion[];
}

namespace Bingo {

namespace {

constexpr const char* kDefaultBaseUrl = "https://shipbingo.zarashane.dev";
constexpr const char* kClientId = "shipbingo-tracker";
constexpr const char* kScope = "feed:read tracker:write";
constexpr const char* kDeviceGrant = "urn:ietf:params:oauth:grant-type:device_code";
constexpr const char* kTokenFile = "shipbingo-link.json";
constexpr int64_t kRefreshMarginSeconds = 60;
constexpr int kSlowDownSeconds = 5;
constexpr size_t kMaxResponseBytes = 1024 * 1024;

using SteadyClock = std::chrono::steady_clock;

int64_t NowUnixSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

size_t WriteBody(char* data, size_t size, size_t count, void* user) {
    auto* body = static_cast<std::string*>(user);
    if (body->size() + size * count > kMaxResponseBytes) {
        return 0;
    }
    body->append(data, size * count);
    return size * count;
}

int AbortIfCancelled(void* user, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return static_cast<const std::atomic<bool>*>(user)->load() ? 1 : 0;
}

std::string FormEscape(const std::string& value) {
    char* escaped = curl_easy_escape(nullptr, value.c_str(), static_cast<int>(value.size()));
    std::string result = escaped ? escaped : "";
    curl_free(escaped);
    return result;
}

std::string ClientVersion() {
    std::string version = gBuildVersion;
    for (char& c : version) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '+' && c != '_') {
            c = '-';
        }
    }
    return version.empty() ? "unknown" : version;
}

std::string DeviceName() {
    std::string name;
#ifdef _WIN32
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        name.assign(buffer, size);
    }
#else
    char buffer[256] = {};
    if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
        name = buffer;
    }
#endif
    if (name.empty()) {
        name = "Ship of Harkinian";
    }
    if (name.size() > 100) {
        name.resize(100);
    }
    return name;
}

std::string TokenFilePath() {
    return Ship::Context::GetPathRelativeToAppDirectory(kTokenFile);
}

std::string StringField(const nlohmann::json& object, const char* key) {
    auto it = object.find(key);
    return it != object.end() && it->is_string() ? it->get<std::string>() : "";
}

std::string ErrorCode(const nlohmann::json& body) {
    return StringField(body, "error");
}

nlohmann::json ParseJson(const std::string& text) {
    return nlohmann::json::parse(text, nullptr, false);
}

std::string ExpiredMessage() {
    return "The code expired. Request a new code.";
}

} // namespace

OAuthClient& OAuthClient::Instance() {
    static OAuthClient instance;
    return instance;
}

OAuthClient::OAuthClient() : baseUrl(kDefaultBaseUrl) {
    LoadTokens();
}

OAuthClient::~OAuthClient() {
    shutdown = true;
    linkCancel = true;
    {
        std::lock_guard<std::mutex> lock(mutex);
    }
    wake.notify_all();
    for (std::thread* worker : { &linkWorker, &taskWorker, &revokeWorker }) {
        if (worker->joinable()) {
            worker->join();
        }
    }
}

void OAuthClient::SyncSettings() {
    std::string url = CVarGetString(CVAR_BINGO("OAuth.BaseUrl"), "");
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    url.erase(url.begin(), std::find_if_not(url.begin(), url.end(), isSpace));
    url.erase(std::find_if_not(url.rbegin(), url.rend(), isSpace).base(), url.end());
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
        url = kDefaultBaseUrl;
    }

    std::lock_guard<std::mutex> lock(mutex);
    baseUrl = std::move(url);
}

bool OAuthClient::IsLinked() {
    std::lock_guard<std::mutex> lock(mutex);
    return snapshot.state == LinkState::Linked;
}

LinkSnapshot OAuthClient::GetSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    LinkSnapshot copy = snapshot;
    if (copy.state == LinkState::Linking && !copy.userCode.empty()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(codeExpiresAt - SteadyClock::now());
        copy.secondsRemaining = static_cast<int>(std::max<int64_t>(0, remaining.count()));
    }
    return copy;
}

// ---- HTTP -----------------------------------------------------------------------------------------------------

HttpResult OAuthClient::Send(const std::string& method, const std::string& path,
                             const std::vector<std::string>& headerLines, const std::string& body,
                             const std::atomic<bool>& cancel) {
    EnsureCurlInitialized();

    HttpResult result;
    std::string base;
    {
        std::lock_guard<std::mutex> lock(mutex);
        base = baseUrl;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        result.error = "Failed to initialize HTTP client";
        return result;
    }

    const std::string url = base + path;
    curl_slist* headers = curl_slist_append(nullptr, ("X-ShipBingo-Client: " + std::string(kClientId) + "/" +
                                                      ClientVersion()).c_str());
    headers = curl_slist_append(headers, "Accept: application/json");
    for (const std::string& line : headerLines) {
        headers = curl_slist_append(headers, line.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Shipwright-Bingo");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, AbortIfCancelled);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &cancel);
    if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
    }

    const CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    } else {
        result.error = curl_easy_strerror(code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

HttpResult OAuthClient::PostForm(const std::string& path,
                                 const std::vector<std::pair<std::string, std::string>>& fields,
                                 const std::atomic<bool>& cancel) {
    std::string body;
    for (const auto& [key, value] : fields) {
        if (!body.empty()) {
            body += '&';
        }
        body += FormEscape(key) + "=" + FormEscape(value);
    }
    return Send("POST", path, { "Content-Type: application/x-www-form-urlencoded" }, body, cancel);
}

// ---- Token storage --------------------------------------------------------------------------------------------

bool OAuthClient::LoadTokens() {
    std::ifstream file(TokenFilePath(), std::ios::binary);
    if (!file) {
        return false;
    }
    const nlohmann::json stored = nlohmann::json::parse(file, nullptr, false);
    const std::string refresh = StringField(stored, "refresh_token");
    if (refresh.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);
    accessToken = StringField(stored, "access_token");
    refreshToken = refresh;
    auto expires = stored.find("expires_at");
    accessExpiresAt = expires != stored.end() && expires->is_number_integer() ? expires->get<int64_t>() : 0;
    snapshot.state = LinkState::Linked;
    return true;
}

bool OAuthClient::StoreTokensLocked(const std::string& access, const std::string& refresh, int64_t expiresAt) {
    accessToken = access;
    refreshToken = refresh;
    accessExpiresAt = expiresAt;

    const nlohmann::json stored = { { "access_token", access }, { "refresh_token", refresh },
                                    { "expires_at", expiresAt } };
    const std::filesystem::path path = TokenFilePath();
    std::filesystem::path temp = path;
    temp += ".tmp";
    try {
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            out << stored.dump();
            if (!out) {
                return false;
            }
        }
        std::filesystem::permissions(temp, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace);
        std::filesystem::rename(temp, path);
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[Bingo] Could not save the ShipBingo link: {}", e.what());
        return false;
    }
}

void OAuthClient::ClearTokensLocked() {
    accessToken.clear();
    refreshToken.clear();
    accessExpiresAt = 0;
    std::error_code ignored;
    std::filesystem::remove(TokenFilePath(), ignored);
}

// ---- State helpers ----------------------------------------------------------------------------------------------

void OAuthClient::SetLinkError(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    if (linkCancel) {
        return;
    }
    snapshot = LinkSnapshot{};
    snapshot.state = LinkState::Error;
    snapshot.message = message;
}

void OAuthClient::SetLinkMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!linkCancel && snapshot.state == LinkState::Linking) {
        snapshot.message = message;
    }
}

void OAuthClient::HandleOutdated(const nlohmann::json& body) {
    outdated = true;
    std::lock_guard<std::mutex> lock(mutex);
    minVersion = StringField(body, "min_version");
    snapshot = LinkSnapshot{};
    snapshot.state = LinkState::Outdated;
    snapshot.message = "Update the game to keep using ShipBingo";
    if (!minVersion.empty()) {
        snapshot.message += " (version " + minVersion + " or newer is required)";
    }
    snapshot.message += ".";
}

void OAuthClient::MarkUnlinked(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    if (snapshot.state == LinkState::Linking) {
        return;
    }
    snapshot = LinkSnapshot{};
    snapshot.message = message;
}

// ---- Linking (device authorization grant) ---------------------------------------------------------------------

void OAuthClient::StartLinking() {
    SyncSettings();

    std::lock_guard<std::mutex> startLock(linkMutex);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (snapshot.state == LinkState::Linking || snapshot.state == LinkState::Linked) {
            return;
        }
        if (outdated) {
            snapshot.state = LinkState::Outdated;
            return;
        }
    }

    if (linkWorker.joinable()) {
        linkWorker.join();
    }
    linkCancel = false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        snapshot = LinkSnapshot{};
        snapshot.state = LinkState::Linking;
        snapshot.message = "Requesting a code...";
    }
    linkWorker = std::thread(&OAuthClient::RunLinking, this);
}

void OAuthClient::CancelLinking() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (snapshot.state != LinkState::Linking) {
            return;
        }
    }
    linkCancel = true;
    {
        std::lock_guard<std::mutex> lock(mutex);
    }
    wake.notify_all();

    std::lock_guard<std::mutex> startLock(linkMutex);
    if (linkWorker.joinable()) {
        linkWorker.join();
    }
    std::lock_guard<std::mutex> lock(mutex);
    if (snapshot.state == LinkState::Linking) {
        snapshot = LinkSnapshot{};
    }
}

void OAuthClient::RunLinking() {
    const auto handleOutdated = [this](const HttpResult& http) {
        if (http.status == 426) {
            HandleOutdated(ParseJson(http.body));
            return true;
        }
        return false;
    };

    HttpResult http = PostForm("/oauth/device_authorization",
                               { { "client_id", kClientId }, { "scope", kScope }, { "device_name", DeviceName() } },
                               linkCancel);
    if (linkCancel) {
        return;
    }
    if (http.status == 0) {
        SPDLOG_WARN("[Bingo] ShipBingo link request failed: {}", http.error);
        SetLinkError("Could not reach ShipBingo. Check your connection and try again.");
        return;
    }
    if (handleOutdated(http)) {
        return;
    }

    nlohmann::json body = ParseJson(http.body);
    if (http.status != 200) {
        const std::string code = ErrorCode(body);
        SPDLOG_WARN("[Bingo] ShipBingo link request rejected (HTTP {}, {})", http.status, code);
        const std::string description = StringField(body, "error_description");
        SetLinkError(description.empty() ? "ShipBingo could not start linking (HTTP " + std::to_string(http.status) + ")."
                                         : "ShipBingo could not start linking: " + description);
        return;
    }

    std::string deviceCode = StringField(body, "device_code"); // secret: kept in memory only
    const std::string userCode = StringField(body, "user_code");
    const std::string verificationUri = StringField(body, "verification_uri");
    std::string verificationUriComplete = StringField(body, "verification_uri_complete");
    if (deviceCode.empty() || userCode.empty() || verificationUri.empty()) {
        SetLinkError("ShipBingo returned an invalid response. Try again.");
        return;
    }
    if (verificationUriComplete.empty()) {
        verificationUriComplete = verificationUri;
    }

    const auto numberField = [&body](const char* key, int64_t fallback) {
        auto it = body.find(key);
        return it != body.end() && it->is_number_integer() ? it->get<int64_t>() : fallback;
    };
    const int64_t expiresIn = std::max<int64_t>(1, numberField("expires_in", 600));
    int64_t interval = std::max<int64_t>(1, numberField("interval", 5));

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (linkCancel) {
            return;
        }
        codeExpiresAt = SteadyClock::now() + std::chrono::seconds(expiresIn);
        snapshot.userCode = userCode;
        snapshot.verificationUri = verificationUri;
        snapshot.verificationUriComplete = verificationUriComplete;
        snapshot.message = "Waiting for approval...";
    }

    auto nextPoll = SteadyClock::now();
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            wake.wait_until(lock, std::min(nextPoll, codeExpiresAt), [this]() { return linkCancel.load(); });
        }
        if (linkCancel) {
            return;
        }
        if (SteadyClock::now() >= codeExpiresAt) {
            SetLinkError(ExpiredMessage());
            return;
        }

        http = PostForm("/oauth/token",
                        { { "grant_type", kDeviceGrant }, { "client_id", kClientId }, { "device_code", deviceCode } },
                        linkCancel);
        if (linkCancel) {
            return;
        }
        nextPoll = SteadyClock::now() + std::chrono::seconds(interval);

        if (http.status == 0) {
            SetLinkMessage("Connection problem, retrying...");
            continue;
        }
        if (handleOutdated(http)) {
            return;
        }

        body = ParseJson(http.body);
        if (http.status == 200) {
            const std::string newAccess = StringField(body, "access_token");
            const std::string newRefresh = StringField(body, "refresh_token");
            auto expiresField = body.find("expires_in");
            const int64_t accessLifetime =
                expiresField != body.end() && expiresField->is_number_integer() ? expiresField->get<int64_t>() : 3600;
            if (newAccess.empty() || newRefresh.empty()) {
                SetLinkError("ShipBingo returned an invalid response. Try again.");
                return;
            }

            bool saved;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (linkCancel) {
                    return;
                }
                saved = StoreTokensLocked(newAccess, newRefresh, NowUnixSeconds() + accessLifetime);
                if (saved) {
                    snapshot = LinkSnapshot{};
                    snapshot.state = LinkState::Linked;
                } else {
                    ClearTokensLocked();
                }
            }
            if (!saved) {
                RevokeToken(newRefresh);
                SetLinkError("Could not save the ShipBingo link to disk. Try again.");
                return;
            }
            StartIdentityFetch();
            return;
        }

        const std::string code = ErrorCode(body);
        if (code == "authorization_pending") {
            continue;
        }
        if (code == "slow_down") {
            interval += kSlowDownSeconds;
            nextPoll = SteadyClock::now() + std::chrono::seconds(interval);
            continue;
        }
        if (code == "access_denied") {
            SetLinkError("Linking was cancelled.");
            return;
        }
        if (code == "expired_token") {
            SetLinkError(ExpiredMessage());
            return;
        }
        if (code == "invalid_grant") {
            SetLinkError("Linking failed. Start over with a new code.");
            return;
        }
        if (http.status >= 500) {
            SetLinkMessage("ShipBingo is having problems, retrying...");
            continue;
        }
        SPDLOG_WARN("[Bingo] ShipBingo linking failed (HTTP {}, {})", http.status, code);
        SetLinkError("Linking failed (HTTP " + std::to_string(http.status) + "). Try again.");
        return;
    }
}

// ---- Unlink -----------------------------------------------------------------------------------------------------

void OAuthClient::RevokeToken(const std::string& token) {
    if (token.empty() || outdated) {
        return;
    }
    const HttpResult http = PostForm("/oauth/revoke", { { "token", token }, { "client_id", kClientId } }, shutdown);
    if (http.status == 0) {
        SPDLOG_WARN("[Bingo] Could not reach ShipBingo to revoke the link: {}", http.error);
    }
}

void OAuthClient::Unlink() {
    CancelLinking();

    std::string tokenToRevoke;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (snapshot.state == LinkState::Linking) {
            return;
        }
        tokenToRevoke = refreshToken;
        ClearTokensLocked();
        const LinkState keep = outdated ? LinkState::Outdated : LinkState::NotLinked;
        const std::string message = snapshot.state == LinkState::Outdated ? snapshot.message : "";
        snapshot = LinkSnapshot{};
        snapshot.state = keep;
        snapshot.message = message;
    }

    if (tokenToRevoke.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(taskMutex);
    if (revokeWorker.joinable()) {
        revokeWorker.join();
    }
    revokeWorker = std::thread([this, tokenToRevoke]() { RevokeToken(tokenToRevoke); });
}

// ---- Tokens and authorized requests -----------------------------------------------------------------------------

AuthResult OAuthClient::GetAccessToken(bool forceRefresh, const std::string& staleToken, std::string& accessTokenOut) {
    std::lock_guard<std::mutex> refreshLock(refreshMutex);

    std::string usedRefreshToken;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (refreshToken.empty()) {
            return AuthResult::Unlinked;
        }
        const bool nearExpiry = NowUnixSeconds() + kRefreshMarginSeconds >= accessExpiresAt;
        if (!accessToken.empty() && !nearExpiry && !forceRefresh) {
            accessTokenOut = accessToken;
            return AuthResult::Ok;
        }
        if (forceRefresh && !accessToken.empty() && accessToken != staleToken) {
            // Another caller already refreshed after the failed request.
            accessTokenOut = accessToken;
            return AuthResult::Ok;
        }
        usedRefreshToken = refreshToken;
    }
    return RefreshLocked(usedRefreshToken, accessTokenOut);
}

// Callers must hold refreshMutex. The old refresh token is never sent again once the server has answered.
AuthResult OAuthClient::RefreshLocked(const std::string& usedRefreshToken, std::string& accessTokenOut) {
    const HttpResult http = PostForm(
        "/oauth/token", { { "grant_type", "refresh_token" }, { "client_id", kClientId }, { "refresh_token", usedRefreshToken } },
        shutdown);
    if (http.status == 0) {
        SPDLOG_WARN("[Bingo] Could not reach ShipBingo to refresh the link: {}", http.error);
        return AuthResult::Network;
    }
    const nlohmann::json body = ParseJson(http.body);
    if (http.status == 426) {
        HandleOutdated(body);
        return AuthResult::Outdated;
    }

    if (http.status == 200) {
        const std::string newAccess = StringField(body, "access_token");
        const std::string newRefresh = StringField(body, "refresh_token");
        auto expiresField = body.find("expires_in");
        const int64_t lifetime =
            expiresField != body.end() && expiresField->is_number_integer() ? expiresField->get<int64_t>() : 3600;
        if (newAccess.empty() || newRefresh.empty()) {
            SPDLOG_WARN("[Bingo] ShipBingo returned an invalid refresh response");
            return AuthResult::Failed;
        }

        bool stillCurrent;
        bool saved = true;
        {
            std::lock_guard<std::mutex> lock(mutex);
            stillCurrent = refreshToken == usedRefreshToken;
            if (stillCurrent) {
                saved = StoreTokensLocked(newAccess, newRefresh, NowUnixSeconds() + lifetime);
            }
        }
        if (!stillCurrent) {
            // The player unlinked while this refresh was in flight; do not leave a live link behind.
            RevokeToken(newRefresh);
            return AuthResult::Unlinked;
        }
        if (!saved) {
            SPDLOG_ERROR("[Bingo] The refreshed ShipBingo link could not be saved; it will be lost on restart");
        }
        accessTokenOut = newAccess;
        return AuthResult::Ok;
    }

    const std::string code = ErrorCode(body);
    if (http.status == 400 && code == "invalid_grant") {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (refreshToken == usedRefreshToken) {
                ClearTokensLocked();
            }
        }
        MarkUnlinked("The ShipBingo link is no longer valid. Link your account again.");
        return AuthResult::Unlinked;
    }

    SPDLOG_WARN("[Bingo] ShipBingo token refresh failed (HTTP {}, {})", http.status, code);
    return AuthResult::Failed;
}

ApiResponse OAuthClient::AuthorizedRequest(const std::string& method, const std::string& path,
                                           const std::string& body, const std::atomic<bool>* cancel) {
    const std::atomic<bool>& cancelFlag = cancel != nullptr ? *cancel : shutdown;
    ApiResponse response;
    if (outdated) {
        response.result = AuthResult::Outdated;
        return response;
    }

    std::string token;
    response.result = GetAccessToken(false, "", token);
    if (response.result != AuthResult::Ok) {
        return response;
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        std::vector<std::string> headers = { "Authorization: Bearer " + token };
        if (!body.empty()) {
            headers.push_back("Content-Type: application/json");
        }
        const HttpResult http = Send(method, path, headers, body, cancelFlag);
        response.status = http.status;
        response.error = http.error;
        response.body = ParseJson(http.body);

        if (http.status == 0) {
            response.result = AuthResult::Network;
            return response;
        }
        if (http.status == 426) {
            HandleOutdated(response.body);
            response.result = AuthResult::Outdated;
            return response;
        }
        if (http.status == 401 && attempt == 0) {
            const AuthResult refreshed = GetAccessToken(true, token, token);
            if (refreshed != AuthResult::Ok) {
                response.result = refreshed;
                return response;
            }
            continue;
        }
        if (http.status == 403) {
            response.result = AuthResult::Forbidden;
            return response;
        }
        response.result = http.status >= 200 && http.status < 300 ? AuthResult::Ok : AuthResult::Failed;
        return response;
    }
    return response;
}

// ---- Identity ---------------------------------------------------------------------------------------------------

void OAuthClient::VerifyLink() {
    SyncSettings();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (snapshot.state != LinkState::Linked || outdated) {
            return;
        }
    }
    StartIdentityFetch();
}

void OAuthClient::StartIdentityFetch() {
    if (taskBusy.exchange(true)) {
        return;
    }
    std::lock_guard<std::mutex> lock(taskMutex);
    if (taskWorker.joinable()) {
        taskWorker.join();
    }
    taskWorker = std::thread([this]() {
        FetchIdentity();
        taskBusy = false;
    });
}

void OAuthClient::FetchIdentity() {
    const ApiResponse response = AuthorizedRequest("GET", "/api/app/v1/me");

    std::lock_guard<std::mutex> lock(mutex);
    if (snapshot.state != LinkState::Linked || refreshToken.empty()) {
        return;
    }
    switch (response.result) {
        case AuthResult::Ok:
            snapshot.username = StringField(response.body, "username");
            snapshot.message.clear();
            break;
        case AuthResult::Network:
            snapshot.message = "Could not reach ShipBingo to confirm the link.";
            break;
        case AuthResult::Forbidden:
        case AuthResult::Failed:
            snapshot.message = "Could not confirm the link (HTTP " + std::to_string(response.status) + ").";
            break;
        case AuthResult::Unlinked:
        case AuthResult::Outdated:
            break;
    }
}

} // namespace Bingo
