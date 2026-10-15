#include "SohMenu.h"
#include "SohGui.hpp"
#include "soh/Bingo/BingoClient.h"

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
              "The site only accepts logged in users. Log in on the site with Discord, then copy the value of its "
              "\"sb.auth\" cookie (browser DevTools > Application > Cookies > shipbingo.zarashane.dev) into the box "
              "below. Sessions expire after about 24 hours, so you will need to paste a fresh cookie each day.",
              WIDGET_TEXT);
    AddWidget(path, "Game ID & Session", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        ImGui::BeginDisabled(Bingo::Client::Instance().IsActive());
        ImGui::Text("Game ID");
        CVarInputString("##BingoGameId", CVAR_BINGO("GameId"),
                        InputOptions()
                            .Color(THEME_COLOR)
                            .PlaceholderText("276")
                            .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                            .LabelPosition(LabelPositions::None));
        ImGui::Text("Session cookie");
        CVarInputString("##BingoSessionCookie", CVAR_BINGO("SessionCookie"),
                        InputOptions()
                            .Color(THEME_COLOR)
                            .IsSecret(true)
                            .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                            .LabelPosition(LabelPositions::None));
        ImGui::EndDisabled();
    });
    AddWidget(path, "Connect##Bingo", WIDGET_BUTTON)
        .PreFunc([](WidgetInfo& info) {
            info.name = Bingo::Client::Instance().IsActive() ? "Disconnect##Bingo" : "Connect##Bingo";
        })
        .Callback([](WidgetInfo& info) {
            auto& client = Bingo::Client::Instance();
            if (client.IsActive()) {
                client.Disconnect();
                return;
            }
            client.Connect(CVarGetString(CVAR_BINGO("GameId"), ""), CVarGetString(CVAR_BINGO("SessionCookie"), ""));
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        });
    AddWidget(path, "Not connected", WIDGET_TEXT).PreFunc([](WidgetInfo& info) {
        info.name = Bingo::Client::Instance().GetSnapshot().status;
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
