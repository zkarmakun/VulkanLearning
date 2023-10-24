#pragma once
#define SDL_MAIN_HANDLED
#include "Renderer/RenderWindow.h"
#include "Renderer/Renderer.h"

int main()
{
	FRenderWindow RenderWindow("Rainbow", 1280, 720);
	
	FRenderer Renderer;
	Renderer.Init(&RenderWindow);
	Renderer.RenderLoop();

	Renderer.Shutdown();
	RenderWindow.Shutdown();
	return 0;
}