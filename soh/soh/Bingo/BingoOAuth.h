#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace Bingo {

enum class LinkState {
    NotLinked,
    Linking,
    Linked,
    Error,
    Outdated, // the server rejected this build with HTTP 426; tokens are kept
};

struct LinkSnapshot {
    LinkState state = LinkState::NotLinked;
    std::string message;
    std::string username;
    std::string userCode;
    std::string verificationUri;
    std::string verificationUriComplete;
    int secondsRemaining = 0;
};

enum class AuthResult {
    Ok,
    Network,   // could not reach the server; tokens are kept
    Unlinked,  // no tokens, or the server says the link is gone
    Outdated,  // HTTP 426
    Forbidden, // HTTP 403, never retried
    Failed,    // any other error response
};

struct ApiResponse {
    AuthResult result = AuthResult::Failed;
    long status = 0;
    nlohmann::json body;
    std::string error;
};

struct HttpResult {
    long status = 0; // 0 means the request itself failed
    std::string body;
    std::string error;
};

// ShipBingo account linking (OAuth 2.0 Device Authorization Grant, RFC 8628) and the authorized request helper
// that future feed/autotracker endpoints should use. All public methods are safe to call from any thread and never
// block on the network, except AuthorizedRequest which must be called from a worker thread.
class OAuthClient {
  public:
    static OAuthClient& Instance();

    // Re-reads the base URL setting. Call from the game thread (CVars are not thread safe).
    void SyncSettings();

    void StartLinking();
    void CancelLinking();
    void Unlink();
    // Confirms the stored link with GET /api/app/v1/me and updates the username.
    void VerifyLink();

    LinkSnapshot GetSnapshot();

    // Adds the auth and client-version headers, refreshes the access token when needed and retries once after a 401.
    // `cancel` aborts the in-flight request when set; it defaults to the client's own shutdown flag.
    ApiResponse AuthorizedRequest(const std::string& method, const std::string& path, const std::string& body = "",
                                  const std::atomic<bool>* cancel = nullptr);

    bool IsLinked();

  private:
    OAuthClient();
    ~OAuthClient();

    HttpResult Send(const std::string& method, const std::string& path, const std::vector<std::string>& headers,
                    const std::string& body, const std::atomic<bool>& cancel);
    HttpResult PostForm(const std::string& path, const std::vector<std::pair<std::string, std::string>>& fields,
                        const std::atomic<bool>& cancel);

    AuthResult GetAccessToken(bool forceRefresh, const std::string& staleToken, std::string& accessTokenOut);
    AuthResult RefreshLocked(const std::string& usedRefreshToken, std::string& accessTokenOut);
    void RevokeToken(const std::string& refreshToken);

    void RunLinking();
    void StartIdentityFetch();
    void FetchIdentity();

    void SetLinkError(const std::string& message);
    void SetLinkMessage(const std::string& message);
    void HandleOutdated(const nlohmann::json& body);
    void MarkUnlinked(const std::string& message);

    bool LoadTokens();
    bool StoreTokensLocked(const std::string& access, const std::string& refresh, int64_t expiresAt);
    void ClearTokensLocked();

    std::mutex mutex; // guards everything below except the workers
    std::mutex refreshMutex; // only one token refresh may run at a time
    std::condition_variable wake;

    LinkSnapshot snapshot;
    std::chrono::steady_clock::time_point codeExpiresAt;
    std::string baseUrl;
    std::string accessToken;
    std::string refreshToken;
    int64_t accessExpiresAt = 0; // unix seconds
    std::string minVersion;

    std::atomic<bool> outdated = false;
    std::atomic<bool> linkCancel = false;
    std::atomic<bool> shutdown = false;
    std::atomic<bool> taskBusy = false;

    std::mutex linkMutex;
    std::mutex taskMutex;
    std::thread linkWorker;
    std::thread taskWorker;
    std::thread revokeWorker;
};

} // namespace Bingo
