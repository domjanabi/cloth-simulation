#include "Window.h"

//this project uses the olcPixelGameEngine header
int main()
{
    Window window;
    if(window.Construct(800, 450, 2, 2))
    {
        window.Start();
    }
}