#pragma once

#include "App/Application.h"
#include <memory>

#define STRINGIZE(x) #x
#define TOSTRING(x) STRINGIZE(x)

#define ENGINE_ENTRY_POINT(AppClass, Title)                                \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) \
{                                                                   \
    Tsumi::AppDesc desc;                                            \
    desc.hInstance = hInstance;                                     \
    desc.windowTitle = Title;                     \
    desc.windowWidth = 1280;                                        \
    desc.windowHeight = 720;                                        \
                                                                    \
    Tsumi::Application app(desc);                                   \
    auto game = std::make_unique<AppClass>();                       \
    app.SetGameApp(std::move(game));                                \
    app.Run();                                                      \
    return 0;                                                       \
}