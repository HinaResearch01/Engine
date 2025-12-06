#pragma once

#include "App/Application.h"
#include "Win/Win32Window.h"
#include <memory>

#define STRINGIZE(x) #x
#define TOSTRING(x) STRINGIZE(x)

#define ENGINE_ENTRY_POINT(AppClass, Title) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) \
{ \
    Tsumi::Win32::Win32Desc desc; \
    desc.hInstance = hInstance; \
    desc.windowTitle = Title; \
    desc.windowWidth = 1280; \
    desc.windowHeight = 720; \
    auto app = Tsumi::Application::GetInstance();\
    app->Init(desc); \
    auto game = std::make_unique<AppClass>(); \
    app->SetGameApp(std::move(game)); \
    app->Run(); \
    return 0; \
}