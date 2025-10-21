#pragma once

#include "App/Application.h"
#include <memory>

#define STRINGIZE(x) #x
#define TOSTRING(x) STRINGIZE(x)

#define ENGINE_ENTRY_POINT(AppClass, Title) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) \
{ \
    Tsumi::AppDesc desc; \
    desc.hInstance = hInstance; \
    desc.windowTitle = Title; \
    desc.windowWidth = 1280; \
    desc.windowHeight = 720; \
    auto game = std::make_unique<AppClass>(); \
    auto app = Tsumi::Application::GetInstance();\
    app->SetAppDesc(desc); \
    app->SetGameApp(std::move(game)); \
    app->Run(); \
    return 0; \
}