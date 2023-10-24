#pragma once
#include <SDL2/SDL_video.h>

#include "string"

using namespace std;

class FRenderWindow
{
public:
    FRenderWindow(const string& title, int InWidth, int InHeight);
    void init();
    void Shutdown();

    SDL_Window* GetWindow() const;
    string GetWindowName() const;

private:
    string WindowsName;
    int Width;
    int Height;
    SDL_Window* pWindow;
    bool bInit;
};
