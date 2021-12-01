#include "Window.cpp"


//this project uses the olcPixelGameEngine header
int main()
{
    Window window;
    if(window.Construct(400, 300, 3, 3))
    {
        window.Start();
    }
}