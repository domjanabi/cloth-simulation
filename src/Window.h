#include "olcPixelGameEngine.h"
#include "point.h"
#include "vec2.h"

using index_t = int32_t;

struct Window: public olc::PixelGameEngine
{
    //decides what left click does.
    //right click always connects points.
    enum class mode
    {
        handmode, 
        cutmode,
        placemode,
        forcemode
    };
    
    ///////////////////////////////////
                //VARIABLES
    ///////////////////////////////////

    static constexpr bool expensive_point_rendering = false;
    static constexpr index_t invalid_index = -1;
    
    //DATA

    std::vector<bool> point_is_static;
    std::vector<point> points;
    //write buffer
    std::vector<stick> sticks;
    // ^
    // | these two get swapped each iteration(!= frame)
    // v
    //read buffer
    std::vector<stick> stickscopy;
    mode currentmode = mode::placemode;

    // invalid_index (aka -1) is the equivalent of 'nullptr'.
    // pointers would've been
    // invalidated by std::vector reallocating
    // also, indices are cheaper to store
    // because let's be honest, i won't have more
    // than a 10^9 points right? .... right?
    
    //INDICES

    //indices for creating new sticks
    index_t newstickstart = invalid_index;
    index_t newstickend = invalid_index;
    //index to the point closest to the mouse
    index_t closest_point = invalid_index;

    //COLOURS

    static constexpr uint32_t point_colour = expensive_point_rendering ? 0xff286e96 : 0xffb446c8;//olc::Pixel({150,110,40}).n : olc::Pixel(200,70,180).n;
    const olc::Pixel stick_colour = olc::Pixel(50,80,50);
    const olc::Pixel highlight_colour = olc::Pixel(255,200,100);


    //VALUES

    //max ratio between current length and target length of stick
    const float snap_ratio = 3.0f;
    const float change_state_radius = 40.0f;
    const float cut_radius = 1.0f;
    const float point_radius = 2.5f;
    const float highlight_radius = 4.0f;
    const float force = 5.0f;
    vec2<float> previous_mouse_pos = {0,0};
    float smoothdelta = 0.1f;


    /////////////////////////////////////
                //FUNCTIONS
    /////////////////////////////////////
    Window();
    bool OnUserCreate();
    bool OnUserUpdate(float fElapsedTime);
    void simulate(float delta, uint32_t iterations, float gravity);
    void constrainPoints();
    void handleInput();
    void generateGrid(uint32_t dim, float stepsize);
    void snapSticks();
    float distance(stick st, vec2<float> pt);
    bool intersects(vec2<float> start1, vec2<float>end1, vec2<float> start2, vec2<float> end2);
    index_t getClosestPoint(float x, float y);
    void movePoint();
    void cutSticks();
    void spawnPoint();
    void applyForce(); 
    void fillCircle(float x, float y, float radius, olc::Pixel colour);
    void renderPoints();
    void renderSticks();
    void startConnection();
    void endConnection();
    void removeStick(uint32_t index);
    void addStick(const stick& st);
};
