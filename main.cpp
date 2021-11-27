#include "Window.cpp"


//this project uses the olcPixelGameEngine header
int main()
{
    Window window;
    if(window.Construct(600, 400, 2, 2))
    {
        window.Start();
    }
}