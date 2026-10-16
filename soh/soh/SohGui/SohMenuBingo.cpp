#include "SohMenu.h"
#include "SohGui.hpp"
#include "soh/Bingo/BingoClient.h"

#include <cstdio>
#include <string>
#include <charconv>
#include <string_view>

extern "C" {
extern PlayState* gPlayState;
}

namespace SohGui {

extern std::shared_ptr<SohMenu> mSohMenu;
using namespace UIWidgets;

// #ifdef _DEBUG
// DebugLogOption defaultLogLevel = DEBUG_LOG_TRACE;
// #else
// DebugLogOption defaultLogLevel = DEBUG_LOG_INFO;
// #endif
// 
// static const std::map<int32_t, const char*> debugSaveFileModes = {
//     { 0, "Off" },
//     { 1, "Vanilla" },
//     { 2, "Maxed" },
// };

std::string Rgba8ToHex(Color_RGBA8 color) {
    char hex[8]; // '#' + 6 digits + null terminator
    std::snprintf(hex, sizeof(hex), "%02X%02X%02X",
                  static_cast<unsigned>(color.r),
                  static_cast<unsigned>(color.g),
                  static_cast<unsigned>(color.b));
    return hex;
}

Color_RGBA8 HexToRgba8(std::string_view hex) {
    const Color_RGBA8 defaultColor = { 255, 255, 255, 255 };
    if (hex.empty()) {
        return defaultColor;
    }

    if (hex.front() == '#') {
        hex.remove_prefix(1);
    }
    if (hex.size() != 6) {
        SPDLOG_WARN("[Bingo] Invalid user color '{}'; using white", std::string(hex));
        return defaultColor;
    }

    unsigned int rgb = 0;
    const auto result = std::from_chars(hex.data(), hex.data() + hex.size(), rgb, 16);
    if (result.ec != std::errc{} || result.ptr != hex.data() + hex.size()) {
        SPDLOG_WARN("[Bingo] Invalid user color '{}'; using white", std::string(hex));
        return defaultColor;
    }

    return {
        static_cast<u8>((rgb >> 16) & 0xFF),
        static_cast<u8>((rgb >> 8) & 0xFF),
        static_cast<u8>(rgb & 0xFF),
        255
    };
}

void DrawRoomsWindow(WidgetInfo& info) {
    auto& client = Bingo::Client::Instance();
    const std::string sessionCookie = CVarGetString(CVAR_BINGO("SessionCookie"), "");
    Bingo::RoomsSnapshot rooms = client.GetRoomsSnapshot();
    if (!rooms.hasRequested) {
        client.ReadRoomsList(sessionCookie);
        rooms = client.GetRoomsSnapshot();
    }

    static ImVec2 cellPadding(8.0f, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);

    ImGui::BeginDisabled(rooms.loading);
    if (ImGui::Button("Refresh Rooms")) {
        client.ReadRoomsList(sessionCookie);
    }
    ImGui::EndDisabled();
    ImGui::TextUnformatted(rooms.status.c_str());

    if (ImGui::BeginTable("tableRoomsAvailable", 4, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Host", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Game Code", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("##Enter as", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();

        for (const Bingo::GameRoom& room : rooms.rooms) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(room.host.empty() ? "-" : room.host.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(room.gameCode.empty() ? "-" : room.gameCode.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(room.description.empty() ? "-" : room.description.c_str());
            ImGui::TableNextColumn();

            if (client.IsActive()) {
                if (ImGui::Button("Disconnect")) {
                    client.Disconnect();
                }
            } else {
                if (ImGui::Button("Join")) {
                    client.Connect(room.gameCode, CVarGetString(CVAR_BINGO("SessionCookie"), ""));
                }
            }
        }
        ImGui::EndTable();
    }

    ImGui::PopStyleVar(1);
}

void SohMenu::AddMenuBingo() {
    // Add Dev Tools Menu
    AddMenuEntry("Bingo", CVAR_SETTING("Menu.BingoSidebarSection"));

    // General
    AddSidebarEntry("Bingo", "Login", 1);
    WidgetPath path = { "Bingo", "Login", SECTION_COLUMN_1 };

    AddWidget(path, "Connect to ShipBingo", WIDGET_SEPARATOR_TEXT);
    AddWidget(path,
              "Enter the Game ID of the room you are playing in on shipbingo.zarashane.dev (the number at the end of "
              "the /play/ URL). The board is read from the site and squares are marked automatically as you "
              "complete them.\n"
              "\n"
              "Marking squares only works for logged in users. Log in on the site with Discord, then copy the value of its "
              "\"sb.auth\" cookie (browser DevTools > Application > Cookies > shipbingo.zarashane.dev) into the box below.",
              WIDGET_TEXT);
    AddWidget(path, "Game ID & Session", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        ImGui::BeginDisabled(Bingo::Client::Instance().IsLoggedIn() || Bingo::Client::Instance().IsActive());
        ImGui::Text("Session cookie");
        CVarInputString("##BingoSessionCookie", CVAR_BINGO("SessionCookie"),
                        InputOptions()
                            .Color(THEME_COLOR)
                            .IsSecret(true)
                            .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                            .LabelPosition(LabelPositions::None));
        ImGui::EndDisabled();
    });
    AddWidget(path, "Login##Bingo", WIDGET_BUTTON)
        .PreFunc([](WidgetInfo& info) {
            info.name = (Bingo::Client::Instance().IsLoggedIn() || Bingo::Client::Instance().IsActive()) ? "Logout##Bingo" : "Login##Bingo";
        })
        .Callback([](WidgetInfo& info) {
            auto& client = Bingo::Client::Instance();
            if (client.IsLoggedIn()) {
                client.Logout();
                return;
            }
            client.Login(CVarGetString(CVAR_BINGO("SessionCookie"), ""));
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        });
    AddWidget(path, "Not connected", WIDGET_TEXT).PreFunc([](WidgetInfo& info) {
        info.name = Bingo::Client::Instance().GetSnapshot().status;
    });

    // Rooms
    path.sidebarName = "Rooms";
    AddSidebarEntry("Bingo", path.sidebarName, 1);
    AddWidget(path, "Bingo Rooms", WIDGET_CUSTOM).CustomFunction(DrawRoomsWindow)
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = !(Bingo::Client::Instance().IsLoggedIn() || Bingo::Client::Instance().IsActive());
        });

    // Bingo Board
    path.sidebarName = "Bingo Board";
    AddSidebarEntry("Bingo", path.sidebarName, 1);
    AddWidget(path, "Bingo Board Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Bingo Board Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("BingoBoardSettings"))
        .WindowName("Bingo Board Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Bingo Board Settings Window."));
        
    AddWidget(path, "Popout Bingo Board", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("BingoBoard"))
        .WindowName("Bingo Board")
        .HideInSearch(true)
        .Options(WindowButtonOptions()
                     .Tooltip("Enables the separate Bingo Board Window.")
                     .EmbedWindow(false));
    AddWidget(path, "Pick your color", WIDGET_CVAR_COLOR_PICKER)
        .HideInSearch(true)
        .CVar(CVAR_BINGO("UserColor"))
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = !Bingo::Client::Instance().IsActive();
        })
        .Options(
            ColorPickerOptions().Color(THEME_COLOR).DefaultValue(HexToRgba8(Bingo::Client::Instance().GetSnapshot().myColor)).ShowReset().ShowRandom());
    AddWidget(path, "Update Color", WIDGET_BUTTON)
        .HideInSearch(true)
        .SameLine(true)
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = !Bingo::Client::Instance().IsActive();
        })
        .Options(ButtonOptions()
            .Size(ImVec2(0, 0)))
        .Callback([](WidgetInfo& info) {
            auto& client = Bingo::Client::Instance();
            client.UpdateUserColor(CVarGetString(CVAR_BINGO("GameId"), ""), CVarGetString(CVAR_BINGO("SessionCookie"), ""),
            Rgba8ToHex(CVarGetColor(CVAR_BINGO("UserColor.Value"), HexToRgba8(Bingo::Client::Instance().GetSnapshot().myColor))));
        });
    AddWidget(path, "Renew Bingo Card", WIDGET_BUTTON)
        .HideInSearch(true)
        .Options(ButtonOptions()
                     .Tooltip("Generates a new Bingo Card."))
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled = !Bingo::Client::Instance().IsActive();
        })
        .Callback([](WidgetInfo& info) {
            auto& client = Bingo::Client::Instance();
            client.RenewCard(CVarGetString(CVAR_BINGO("GameId"), ""), CVarGetString(CVAR_BINGO("SessionCookie"), ""), "-1");
        });
    AddWidget(path, "Seed ID", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        ImGui::Text("Seed ID");
        CVarInputString("##BingoSeedId", CVAR_BINGO("SeedId"),
                        InputOptions()
                            .Color(THEME_COLOR)
                            .PlaceholderText("665274846")
                            .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                            .LabelPosition(LabelPositions::None));
    });
    AddWidget(path, "New Seeded Card", WIDGET_BUTTON)
        .HideInSearch(true)
        .SameLine(true)
        .Options(ButtonOptions()
                     .Tooltip("Generates a new Bingo Card using a seed value."))
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled = !Bingo::Client::Instance().IsActive();
        })
        .Callback([](WidgetInfo& info) {
            auto& client = Bingo::Client::Instance();
            client.RenewCard(CVarGetString(CVAR_BINGO("GameId"), ""), CVarGetString(CVAR_BINGO("SessionCookie"), ""),
                             CVarGetString(CVAR_BINGO("SeedId"), ""));
        });


    //// Console
    //path.sidebarName = "Console";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    //AddWidget(path, "Popout Console", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("SohConsole"))
    //    .WindowName("Console##SoH")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Console Window."));

    //// Save Editor
    //path.sidebarName = "Save Editor";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    //AddWidget(path, "Popout Save Editor", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("SaveEditor"))
    //    .WindowName("Save Editor")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Save Editor Window."));

    //// Hook Debugger
    //path.sidebarName = "Hook Debugger";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    //AddWidget(path, "Popout Hook Debugger", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("HookDebugger"))
    //    .WindowName("Hook Debugger")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Hook Debugger Window."));

    //// Collision Viewer
    //path.sidebarName = "Collision Viewer";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 2);
    //AddWidget(path, "Popout Collision Viewer", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("CollisionViewer"))
    //    .WindowName("Collision Viewer")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Collision Viewer Window."));

    //// Actor Viewer
    //path.sidebarName = "Actor Viewer";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 2);
    //AddWidget(path, "Popout Actor Viewer", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("ActorViewer"))
    //    .WindowName("Actor Viewer")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Actor Viewer Window."));

    //// Display List Viewer
    //path.sidebarName = "DList Viewer";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 2);
    //AddWidget(path, "Popout Display List Viewer", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("DisplayListViewer"))
    //    .WindowName("Display List Viewer")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Display List Viewer Window."));

    //// Value Viewer
    //path.sidebarName = "Value Viewer";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 2);
    //AddWidget(path, "Popout Value Viewer", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("ValueViewer"))
    //    .WindowName("Value Viewer")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Value Viewer Window."));

    //// Message Viewer
    //path.sidebarName = "Message Viewer";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 2);
    //AddWidget(path, "Popout Message Viewer", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("MessageViewer"))
    //    .WindowName("Message Viewer")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Message Viewer Window."));

    //// Gfx Debugger
    //path.sidebarName = "Gfx Debugger";
    //AddSidebarEntry("Dev Tools", path.sidebarName, 1);
    //AddWidget(path, "Popout Gfx Debugger", WIDGET_WINDOW_BUTTON)
    //    .CVar(CVAR_WINDOW("SohGfxDebugger"))
    //    .WindowName("GfxDebugger##SoH")
    //    .HideInSearch(true)
    //    .Options(WindowButtonOptions().Tooltip("Enables the separate Gfx Debugger Window."));
}

} // namespace SohGui
