#include "RenderWindow.h"

#include <SDL2/SDL.h>

FRenderWindow::FRenderWindow(const string& title, int InWidth, int InHeight)
    : WindowsName(title)
    , Width(InWidth)
    , Height(InHeight)
    , pWindow(nullptr)
    , bInit(false)
{
    init();
}

bool FRenderWindow::init()
{
    // Create an SDL window that supports Vulkan rendering.
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Could not initialize SDL.");
        return false;
    }
    pWindow= SDL_CreateWindow(WindowsName.c_str(), SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_VULKAN);
    if(pWindow == nullptr)
    {
        printf("Could not create SDL window.");
        return false;
    }
    
    bInit = true;
    return bInit;
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
