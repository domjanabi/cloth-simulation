#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "point.h"
#include "vec2.h"

class Window: public olc::PixelGameEngine
{
    public:
    //decides what left click does.
    //right click always connects points.
    enum class mode
    {
        handmode, 
        cutmode,
        placemode
    };

    std::vector<bool> point_is_static;
    std::vector<point> points;
    //write buffer
    std::vector<stick> sticks;
    // ^
    // | these two get swapped each iteration(!= frame)
    // v
    //read buffer
    std::vector<stick> stickscopy;

    // -1 is the equivalent of 'nullptr'.
    // i needed to use indices,
    // because pointers would've been
    // invalidated by std::vector reallocating
    // also, indices are cheaper to store
    // because let's be honest, i won't have more
    // than a billion points right? .... right?
    
    //indices for creating new sticks
    int32_t newstickstart = -1;
    int32_t newstickend = -1;
    //index to the point closest to the mouse
    int32_t highlight = -1;

    const olc::Pixel pointcolour = olc::Pixel(150,110,40);
    const olc::Pixel stickcolour = olc::Pixel(50,80,50);
    mode currentmode = mode::placemode;
    
    //max ratio between current length and target length of stick
    float snap_ratio = 3.0f;
    float change_state_radius = 40.0f;
    float cut_radius = 1.0f;
    float point_radius = 2.5f;
    float highlight_radius = 4.0f;
    vec2<float> previous_mouse_pos = {0,0};


    Window()
    {
        sAppName = "cloth simulation";
    }
    bool OnUserCreate()
    {
        Clear(olc::Pixel(40,45,35));
        points.reserve(400);
        sticks.reserve(400);
        stickscopy.reserve(400);
        point_is_static.reserve(400);
        return true;
    }
    bool OnUserUpdate(float fElapsedTime)
    {
        //fade (significantly slower)
        //comment out Clear() if you want to use this
        // SetPixelMode(olc::Pixel::ALPHA);
        // FillRect(0,0,ScreenWidth(), ScreenHeight(), olc::Pixel(30,15,30, 40));//40,45,35
        // SetPixelMode(olc::Pixel::NORMAL);
        
        Clear(olc::Pixel(33, 36, 30));

        if( !(currentmode == mode::handmode && GetMouse(0).bHeld))
        {
            highlight = getClosestPoint(GetMouseX(), GetMouseY());
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
            newstickstart = -1;
            newstickend = -1;
        }

        if(newstickstart!=-1)
        {
            int temp = getClosestPoint(GetMouseX(), GetMouseY());
            if(newstickstart!=temp && temp != -1)
            {
                DrawLine(points[newstickstart].pos.x,points[newstickstart].pos.y, points[temp].pos.x, points[temp].pos.y, olc::Pixel(255,200,100));
            }
        }

        simulate(fElapsedTime, 4, 200.0f);
        snapSticks();
        renderPoints();
        renderSticks();

        //caps to 60fps
        //std::this_thread::sleep_for(std::chrono::duration<float>(0.0166666f- fElapsedTime));

        previous_mouse_pos = vec2<float>{(float)GetMouseX(), (float)GetMouseY()};
        return true;
    }


    //used verlet integration for this
    //https://youtu.be/3HjO_RGIjCU
    // ^ very useful link
    void simulate(float delta, uint32_t iterations = 10, float gravity = 50.0f)
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


                //keeps points at constant distance from eachother
                //at least in theory, with less iterations it's bouncier
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


    void handleInput()
    {
        if(GetKey(olc::Key::S).bPressed)
        {
            if(highlight != -1)
            {
                bool nearmouse = (points[highlight].pos - vec2<float>{(float)GetMouseX(), (float)GetMouseY()}).mag2() < change_state_radius;
                point_is_static[highlight] = nearmouse?true:point_is_static[highlight];
            }
        }
        else if(GetKey(olc::Key::D).bPressed)
        {
            if(highlight != -1)
            {
                bool nearmouse = (points[highlight].pos - vec2<float>{(float)GetMouseX(), (float)GetMouseY()}).mag2() < change_state_radius;
                point_is_static[highlight] = nearmouse?false:point_is_static[highlight];
            }
        }
        if(GetKey(olc::Key::H).bPressed)
        {
            currentmode = mode::handmode;
        }
        else if(GetKey(olc::Key::C).bPressed)
        {
            currentmode = mode::cutmode;
        }
        else if(GetKey(olc::Key::P).bPressed)
        {
            currentmode = mode::placemode;
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
            std::cout<<"value of highlight: "<< highlight<<'\n';
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
    void generateGrid(uint32_t dim, float stepsize = 20.0f)
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
    void snapSticks()
    {
        for(uint32_t i = 0;i!=sticks.size();++i)
        {
            if((points[sticks[i].start].pos - points[sticks[i].end].pos).mag() / sticks[i].target_length > snap_ratio)
            {
                removeStick(i);
                --i;
            }
        }
    }

    //bounces the points off the window borders
    void constrainPoints()
    {
        for(auto&p : points)
        {
            const vec2<float> vel = p.pos - p.prev;
            const float sw =(float)ScreenWidth(); 
            const float sh = (float) ScreenHeight();
            if(p.pos.x > sw)
            {
                p.pos.x = sw;
                p.prev.x = p.pos.x + abs(vel.x);
            }
            else if(p.pos.x < 0.0f)
            {
                p.pos.x = 0.0f;
                p.prev.x = p.pos.x - abs(vel.x);
            }
            if(p.pos.y > sh)
            {
                p.pos.y = sh;
                p.prev.y = p.pos.y + abs(vel.y);
            }
            else if(p.pos.y < 0.0f)
            {
                p.pos.y = 0.0f;
                p.prev.y = p.pos.y - abs(vel.y);
            }
        }
    }

    //gets the distance between a line segment (aka stick), and a point
    float distance(stick st, vec2<float> pt)
    {
        const vec2<float> a = points[st.start].pos;
        const vec2<float> b = points[st.end].pos;

        //length squared
        const float l2 = (a-b).mag2();
        if(l2 < 0.001f)
        {
            return (a-pt).mag();
        }
        const float t = max(0.0f, min(1.0f, (pt-a).dot(b-a) /l2 ));
        const vec2<float> projection = a + (b-a) * t;
        return (pt - projection).mag();
    }

    //returns whether two line segments intersect or not
    bool intersects(vec2<float> start1, vec2<float>end1, vec2<float> start2, vec2<float> end2)
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
    int getClosestPoint(float x, float y)
    {
        if(points.size()==0)
        {
            return -1;
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

    void movePoint()
    {
        if(GetMouse(0).bHeld)
        {
            if(highlight != -1)
            {
                points[highlight].pos = vec2<float>{(float)GetMouseX(), (float)GetMouseY()};
            }
        }
    }
    //can leave orphan points that bounce around
    void cutSticks()
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
                removeStick(i);
                --i;
            }
        }
    }
    void spawnPoint()
    {
        if(GetMouse(0).bPressed)
        {
            vec2<float> temp = {(float)GetMouseX(), (float)GetMouseY()}; 
            points.emplace_back(point{temp,temp});
            point_is_static.emplace_back(true);
        }
    }
    
    //there is a function for doing this in olc::PGE,
    //but it takes in integers for position,
    //which means that it's not actually a circle rendered in low res,
    //but more of a "texture", as there is only one shape the circle can have.
    //replace fillCircle() with FillCircle() calls,
    //and you might see the difference yourself 
    void fillCircle(float x, float y, float radius, olc::Pixel colour)
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

    void renderPoints()
    {
        
        for(int i = 0; i!= points.size();++i)
        {
            point& p = points[i];
            if(i != highlight)
            {
                fillCircle(p.pos.x, p.pos.y,point_radius, pointcolour);   
            }
            else
            {
                //renders closest point to mouse highlighted
                fillCircle(p.pos.x, p.pos.y, highlight_radius, olc::Pixel(255,200,100));
                fillCircle(p.pos.x, p.pos.y, point_radius, pointcolour);
            }
        }
    }
    void renderSticks()
    {
        for(const auto& stick : sticks)
        {
            DrawLine(points[stick.start].pos.x, points[stick.start].pos.y, points[stick.end].pos.x, points[stick.end].pos.y, stickcolour);
        }
    }
    
    
    //for connecting points using sticks
    void startConnection()
    {
        newstickstart = getClosestPoint(GetMouseX(), GetMouseY());
    }

    void endConnection()
    {
        if(newstickstart != -1)
        {
            newstickend = getClosestPoint(GetMouseX(), GetMouseY());
            if(newstickstart != newstickend)
            {
                float len = (points[newstickstart].pos - points[newstickend].pos).mag();
                addStick(stick{(uint32_t)newstickstart, (uint32_t)newstickend, len});
                newstickstart = -1;
                newstickend = -1;
            }
        }
    }

    void removeStick(uint32_t index)
    {
        sticks.erase(sticks.begin()+index);
        stickscopy.erase(stickscopy.begin()+index);
    }
    void addStick(const stick& st)
    {
        sticks.emplace_back(st);
        stickscopy.emplace_back(st);
    }
};