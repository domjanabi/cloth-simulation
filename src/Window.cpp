#include "Window.h"

Window::Window()
{
    sAppName = "cloth simulation";
}
bool Window::OnUserCreate()
{
    Clear(olc::Pixel(40,45,35));
    points.reserve(400);
    sticks.reserve(400);
    stickscopy.reserve(400);
    point_is_static.reserve(400);

    return true;
}
bool Window::OnUserUpdate(float fElapsedTime)
{
    //      fade (significantly slower)
    //      comment out Clear() if you want to use this
    // SetPixelMode(olc::Pixel::ALPHA);
    // FillRect(0,0,ScreenWidth(), ScreenHeight(), olc::Pixel(30,15,30, 40));//40,45,35
    // SetPixelMode(olc::Pixel::NORMAL);

    smoothdelta = (smoothdelta+fElapsedTime*0.1f)/1.1f;


    Clear(olc::Pixel(33, 36, 30));
    
    if((currentmode == mode::handmode && GetMouse(0).bHeld) != true)
    {
        closest_point = getClosestPoint(GetMouseX(), GetMouseY());
    }

    handleInput();
    
    switch(currentmode)
    {
        case mode::handmode:
        movePoint();
        break;
        case mode::cutmode:
        cutSticks();
        break;
        case mode::placemode:
        spawnPoint();
        break;
        case mode::forcemode:
        applyForce();
    }

    if(GetMouse(1).bPressed)
    {
        startConnection();
    }
    else if(GetMouse(1).bReleased)
    {
        endConnection();
    }
    if(GetMouse(1).bHeld != true)
    {
        //resets
        newstickstart = invalid_index;
        newstickend = invalid_index;
    }

    if(newstickstart != invalid_index)
    {
        int temp = getClosestPoint(GetMouseX(), GetMouseY());
        if(newstickstart != temp && temp != invalid_index)
        {
            DrawLine(points[newstickstart].pos.x,points[newstickstart].pos.y, points[temp].pos.x, points[temp].pos.y, olc::Pixel(255,200,100));
        }
    }

    simulate(1/100.0f, 8, 200.0f);
    snapSticks();
    if(expensive_point_rendering)
    {
        renderPoints();
    }
    renderSticks();
    if(expensive_point_rendering != true)
    {
        renderPoints();
    }
    previous_mouse_pos = vec2<float>{(float)GetMouseX(), (float)GetMouseY()};

    //std::this_thread::sleep_for(std::chrono::duration<float, std::ratio<1,1>>(1/60.0f - fElapsedTime));

    return true;
}


//used verlet integration for this
//https://youtu.be/3HjO_RGIjCU
// ^ very useful link
void Window::simulate(float delta, uint32_t iterations = 10, float gravity = 50.0f)
{
    //moves points one time step further
    for(size_t i = 0;i != points.size();++i)
    {
        if(!point_is_static[i])
        {
            point& p = points[i]; 
            const vec2<float> temp = p.pos;
            p.pos += p.pos - p.prev;
            p.pos += vec2<float>{0.0f,1.0f} * gravity * delta * delta;
            p.prev = temp;
            p.prev = p.pos + (p.prev - p.pos) * 0.999f;
        }
    }

    while(iterations--)
    {
        for(int i = 0;i!=sticks.size();++i)
        {
            stick& s = sticks[i];
            stick& scopy = stickscopy[i];
            const vec2<float> stickcentre =(points[s.start].pos + points[s.end].pos)/2.0f;
            const vec2<float> stickdir = (points[s.start].pos - points[s.end].pos).normalised();


            //keeps points at constant distance from eachother,
            //at least in theory. with less iterations it's bouncier
            //with more iterations it becomes rigid
            if(!point_is_static[s.start])
                points[scopy.start].pos = stickcentre + stickdir * s.target_length / 2.0f;

            if(!point_is_static[s.end])
                points[scopy.end].pos = stickcentre - stickdir * s.target_length / 2.0f;
        }
        //swaps read and write buffers
        sticks.swap(stickscopy);
        constrainPoints();
    }
}


void Window::handleInput()
{
    if(GetKey(olc::S).bPressed)
    {
        if(closest_point != invalid_index)
        {
            bool nearmouse = (points[closest_point].pos - vec2<float>{(float)GetMouseX(), (float)GetMouseY()}).mag2() < change_state_radius;
            point_is_static[closest_point] = nearmouse?true:point_is_static[closest_point];
        }
    }
    else if(GetKey(olc::D).bPressed)
    {
        if(closest_point != invalid_index)
        {
            bool nearmouse = (points[closest_point].pos - vec2<float>{(float)GetMouseX(), (float)GetMouseY()}).mag2() < change_state_radius;
            point_is_static[closest_point] = nearmouse?false:point_is_static[closest_point];
        }
    }
    if(GetKey(olc::H).bPressed)
    {
        currentmode = mode::handmode;
    }
    else if(GetKey(olc::C).bPressed)
    {
        currentmode = mode::cutmode;
    }
    else if(GetKey(olc::P).bPressed)
    {
        currentmode = mode::placemode;
    }
    else if(GetKey(olc::F).bPressed)
    {
        currentmode = mode::forcemode;
    }
    if(GetKey(olc::Key::G).bPressed)
    {
        generateGrid(64,5.0f);
    }

    if(GetKey(olc::Key::Q).bPressed)
    {
        //several std::cout<< calls are not optimal
        //but more readable imo
        //for debugging purposes so not a big deal
        std::cout<<"DEBUG INFORMATION:\n";
        std::cout<<"amount of points: "<<points.size()<<'\n';
        std::cout<<"amount of sticks: "<<sticks.size()<<'\n';
        std::cout<<"value of newstickstart: "<<newstickstart<<'\n';
        std::cout<<"value of newstickend: "<< newstickend<<'\n';
        std::cout<<"value of highlight: "<< closest_point<<'\n';
    }
    if(GetKey(olc::Key::A).bPressed)
    {
        //disconnects all points
        std::cout<<"deleted all sticks.\n";
        sticks.clear();
        stickscopy.clear();
    }
    if(GetKey(olc::Key::Y).bPressed)
    {
        //erases everything.
        std::cout<<"deleted all points.\n";
        sticks.clear();
        stickscopy.clear();
        points.clear();
        point_is_static.clear();
    }
}
//generates a grid that acts like cloth
void Window::generateGrid(uint32_t dim, float stepsize = 20.0f)
{
    uint32_t initial_size = points.size();
    for(auto i = 0;i<dim;++i)
    {
        for(auto j = 0;j<dim;++j)
        {
            vec2<float> temp{20 + j*stepsize,20 + i*stepsize};
            points.emplace_back(point{temp, temp});

            bool should_be_static = i==0 && (j%4==0 || j == dim-1);
            point_is_static.emplace_back(should_be_static);
        }
    }
    for(auto i = 0;i<dim-1;++i)
    {
        for(auto j = 0;j<dim-1;++j)
        {
            uint32_t s  = initial_size + i*dim + j;
            uint32_t e1 = initial_size + (i)*dim + j + 1;
            
            addStick(stick{s, e1, (points[s].pos - points[e1].pos).mag() });
            uint32_t e2 = initial_size + (i+1)*dim + j;
            addStick(stick{s, e2, (points[s].pos - points[e2].pos).mag()});
        }
    }
    for(auto i = 0;i!=dim-1;++i)
    {
        uint32_t s = initial_size + (i+1)*dim + dim - 1;
        uint32_t e = initial_size + i*dim + dim-1;
        addStick(stick{s, e, (points[s].pos - points[e].pos).mag()});
        s = initial_size + dim*(dim-1) + i;
        e = initial_size + dim*(dim-1) + i + 1;
        addStick(stick{s, e, (points[s].pos - points[e].pos).mag()});
    }
}

//snaps all sticks that become too long
void Window::snapSticks()
{
    for(uint32_t i = 0;i!=sticks.size();++i)
    {
        float current_length = (points[sticks[i].start].pos - points[sticks[i].end].pos).mag(); 
        if(current_length / sticks[i].target_length > snap_ratio)
        {
            removeStick(i);
            --i;
        }
    }
}

//bounces the points off the window borders
void Window::constrainPoints()
{
    for(size_t i = 0;i!=points.size();++i)
    {
        point p = points[i];
        const vec2<float> vel = p.pos - p.prev;
        const float sw =(float)ScreenWidth(); 
        const float sh = (float) ScreenHeight();
        if(p.pos.x > sw - point_radius)
        {
            p.pos.x = sw - point_radius;
            p.prev.x = p.pos.x + abs(vel.x);
        }
        else if(p.pos.x < point_radius)
        {
            p.pos.x = point_radius;
            p.prev.x = p.pos.x - abs(vel.x);
        }
        if(p.pos.y > sh - point_radius)
        {
            p.pos.y = sh - point_radius;
            p.prev.y = p.pos.y + abs(vel.y);
        }
        else if(p.pos.y < point_radius)
        {
            p.pos.y = point_radius;
            p.prev.y = p.pos.y - abs(vel.y);
        }
        points[i] = p;
    }
}

//gets the distance between a line segment (aka stick), and a point
float Window::distance(stick st, vec2<float> pt)
{
    const vec2<float> a = points[st.start].pos;
    const vec2<float> b = points[st.end].pos;

    //length squared
    const float l2 = (a-b).mag2();
    if(l2 < 0.001f)
    {
        return (a-pt).mag();
    }
    const float t = fmax(0.0f, fmin(1.0f, (pt-a).dot(b-a) /l2 ));
    const vec2<float> projection = a + (b-a) * t;
    return (pt - projection).mag();
}

//returns whether two line segments intersect or not
bool Window::intersects(vec2<float> start1, vec2<float>end1, vec2<float> start2, vec2<float> end2)
{
    //t=
    //(x1-x3)(y3-y4) - (y1-y3)(x3-x4)
    //______________________________
    //(x1-x2)(y3-y4) - (y1-y2)(x3-x4)
    
    //u=
    //(x1-x3)(y1-y2) - (y1-y3)(x1-x2)
    //______________________________
    //(x1-x2)(y3-y4) - (y1-y2)(x3-x4)
    
    const vec2<float> a = start1;
    const vec2<float> b = end1;
    const vec2<float> c = start2;
    const vec2<float> d = end2;

    const float x12 = a.x - b.x;
    const float y12 = a.y - b.y;
    const float x13 = a.x - c.x;
    const float y13 = a.y - c.y;
    const float y34 = c.y - d.y;
    const float x34 = c.x - d.x;

    float t = x13*y34 - y13*x34;
    t/=x12*y34 - y12*x34;
    if(t<0.0f||t>1.0f)
    {
        return false;
    }
    float u = x13*y12 - y13*x12;
    u/=x12*y34 - y12*x34;
    if(u<0.0f||u>1.0f)
    {
        return false;
    }
    return true;
}

//-1 when no points exist
index_t Window::getClosestPoint(float x, float y)
{
    if(points.size()==0)
    {
        return invalid_index;
    }
    int out = 0;
    vec2<float> pos{x,y};
    for(int i = 0;i!=points.size();++i)
    {
        if((pos - points[i].pos).mag2() < (pos - points[out].pos).mag2())
        {
            out = i;
        }
    }
    return out;
}

void Window::movePoint()
{
    if(GetMouse(0).bHeld)
    {
        if(closest_point != invalid_index)
        {
            points[closest_point].pos = vec2<float>{(float)GetMouseX(), (float)GetMouseY()};
        }
    }
}
//can leave orphan points that bounce around
void Window::cutSticks()
{
    if(GetMouse(0).bHeld)
    {
        const vec2<float> current_mouse_pos = {(float)GetMouseX(), (float)GetMouseY()}; 
        const vec2<float> mouse_delta = current_mouse_pos - previous_mouse_pos;
        for(uint32_t i = 0;i != sticks.size();++i)
        {
            //in most cases, the mouse will probably be moving faster than a distance of two pixels per frame.
            //so checking whether a stick should be removed happens by checking for intersection between
            //two line segments.
            //one line segment is the path that the mouse took from previous to current frame
            //the other line segment is the stick itself
            //in the case that the user is not moving their mouse, however,
            //they should not be disappointed by the mouse not cutting through any cloth that passes it by.
            //so we switch to distance mode, and the mouse will cut through any sticks
            //that are close enough to it.
            //distance mode is not used for higher velocities because it wouldn't create
            //clean cuts, but rather, scattered holes. 
            if(mouse_delta.mag()<2.0f)
            {
                if(!(distance(sticks[i], vec2<float>{(float)GetMouseX(),(float)GetMouseY()}) < 1.0f))
                {
                    continue;
                }
            }
            else if(!(intersects(current_mouse_pos, previous_mouse_pos, points[sticks[i].start].pos, points[sticks[i].end].pos)))
            {
                continue;
            }
            //moves points a bit when cutting them apart
            points[sticks[i].start].pos+=mouse_delta/10.0f;
            points[sticks[i].end].pos+=mouse_delta/10.0f;
            removeStick(i);
            --i;
        }
    }
}
void Window::spawnPoint()
{
    if(GetMouse(0).bPressed)
    {
        vec2<float> temp = {(float)GetMouseX(), (float)GetMouseY()}; 
        points.emplace_back(point{temp,temp});
        point_is_static.emplace_back(true);
    }
}
void Window::applyForce()
{
    if(GetMouse(0).bHeld)
    {
        vec2<float> mp = {float(GetMouseX()), float(GetMouseY())};
        for(size_t i = 0;i!= points.size();++i)
        {
            if(point_is_static[i] != true)
            {
                point& p = points[i];
                vec2<float> dir = mp - p.pos;
                p.pos += dir.normalised()/(dir.mag()+0.5) * force;
            }
        }
    }
}


//there is a function for doing this in olc::PGE,
//but it takes in integers for position,
//which means that it's not actually a circle rendered in low res,
//but more of a "texture", as there is only one shape the circle can have.
//replace fillCircle() with FillCircle() calls,
//and you might see the difference yourself 
void Window::fillCircle(float x, float y, float radius, olc::Pixel colour)
{
    int startx = int(x-radius);
    int starty = int(y-radius);
    int endx = ceil(x+radius);
    int endy = ceil(y+radius);
    for(int i = starty;i<endy;++i)
    {
        for(int j = startx;j<endx;++j)
        {
            float deltax = j-x;
            float deltay = i-y;
            float dist2 = deltax*deltax+deltay*deltay;
            if(dist2<=radius*radius)
            {
                Draw(j,i,colour);
            }
        }
    }
}

void Window::renderPoints()
{
    if(expensive_point_rendering)
    {
        for(int i = 0; i!= points.size();++i)
        {

            const point& p = points[i];
            const olc::vf2d pos = reinterpret_cast<const olc::vf2d&>(p.pos);
            const olc::vf2d pr = olc::vf2d(point_radius, point_radius);
            if(i != closest_point)
            {
                FillCircle(p.pos.x, p.pos.y,point_radius, point_colour);
            }
            else
            {
                //renders closest point to mouse highlighted
                FillCircle(p.pos.x, p.pos.y, highlight_radius, highlight_colour);
                FillCircle(p.pos.x, p.pos.y, point_radius, point_colour);
            }
        }
    }
    else
    {
        for(int i = 0; i!= points.size();++i)
        {
            const point& p = points[i];
            if(i != closest_point)
            {
                Draw(p.pos.x, p.pos.y, point_colour);
            }
            else
            {
                //Draw(p.pos.x, p.pos.y, highlight_colour);
                fillCircle(p.pos.x, p.pos.y, 2.0f, highlight_colour);
            }
        }
    }
}
void Window::renderSticks()
{
    for(const auto& stick : sticks)
    {
        DrawLine(points[stick.start].pos.x, points[stick.start].pos.y, points[stick.end].pos.x, points[stick.end].pos.y, stick_colour);
    }
}


//for connecting points using sticks
void Window::startConnection()
{
    newstickstart = getClosestPoint(GetMouseX(), GetMouseY());
}

void Window::endConnection()
{
    if(newstickstart != invalid_index)
    {
        newstickend = getClosestPoint(GetMouseX(), GetMouseY());
        if(newstickstart != newstickend)
        {
            float len = (points[newstickstart].pos - points[newstickend].pos).mag();
            addStick(stick{(uint32_t)newstickstart, (uint32_t)newstickend, len});
            newstickstart = invalid_index;
            newstickend = invalid_index;
        }
    }
}

void Window::removeStick(uint32_t index)
{
    sticks.erase(sticks.begin()+index);
    stickscopy.erase(stickscopy.begin()+index);
}
void Window::addStick(const stick& st)
{
    sticks.emplace_back(st);
    stickscopy.emplace_back(st);
}
