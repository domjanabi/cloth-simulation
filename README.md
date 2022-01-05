# cloth-simulation
A cloth simulation based on verlet integration.

This video series helped a lot:
https://youtu.be/3HjO_RGIjCU

## Controls:

Hold LMB to do one of three things:
* Cut sticks
* Move points
* Place points

You can choose what the LMB should do by toggling the correct mode:
* Press [C] to choose cut-mode
* Press [H] to choose hand-mode (moving points)
* Press [P] to choose place-mode

The point closest to the mouse will be highlighted.
When you use hand-mode, that point will start following your cursor (along with all that are connected to it).

You can use the RMB to connect points together
by pressing down at one point and releasing at the other.

Additional keys:
* [Q] for debug information (if you feel so inclined)
* [A] to delete all sticks
* [Y] to delete all points _and_ sticks
* [S] to make a point static
* [D] to reverse [S]
* and most importantly, **[G]** to generate a grid of connected points which forms a cloth


Compiling shouldn't be too hard.
For clang on Windows:
`clang++ -O3 Window.cpp main.cpp -o cloth_sim.exe`
or
`clang-cl -O2 Window.cpp main.cpp -o cloth_sim.exe`

*LMB=Left Mouse Button,
*RMB=Right Mouse Button