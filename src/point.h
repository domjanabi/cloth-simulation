#pragma once
#include "vec2.h"

struct point
{
    vec2<float> pos={0,0};
    vec2<float> prev={0,0};
};

struct stick
{
    //doesn't use index_t because a stick shouldn't 
    //be capable of having invalid indices
    uint32_t start;
    uint32_t end;
    float target_length;
    
};