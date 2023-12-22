#pragma once
#define SDL_MAIN_HANDLED
#include "RenderWindow.h"
#include "Renderer.h"

int main()
{
    FRenderWindow RenderWindow("Rainbow", 1920, 1080);
	
    FRenderer Renderer;
    Renderer.Init(&RenderWindow);
    Renderer.RenderLoop();

    Renderer.Shutdown();
    RenderWindow.Shutdown();
    return 0;
}
