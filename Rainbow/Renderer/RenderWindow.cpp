#include "RenderWindow.h"

#include <SDL2/SDL.h>

#include "Core/Assertions.h"

FRenderWindow::FRenderWindow(const string& title, int InWidth, int InHeight)
    : WindowsName(title)
    , Width(InWidth)
    , Height(InHeight)
    , pWindow(nullptr)
    , bInit(false)
{
    init();
}

void FRenderWindow::init()
{
    // Create an SDL window that supports Vulkan rendering.
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        checkf(0, "Could not initialize SDL");
    }
    pWindow= SDL_CreateWindow(WindowsName.c_str(), SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_VULKAN);
    if(pWindow == nullptr)
    {
        checkf(0, "Could not create SDL window.");
    }
    
    bInit = true;
}

void FRenderWindow::Shutdown()
{
    if(!bInit) return;

    SDL_DestroyWindow(pWindow);
    SDL_Quit();
}

SDL_Window* FRenderWindow::GetWindow() const
{
    return pWindow;
}

string FRenderWindow::GetWindowName() const
{
    return WindowsName;
}
