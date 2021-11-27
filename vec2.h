#pragma once
#include <cstdint>
#include <cmath>


template <typename elem_type = float>
struct vec2
{
    elem_type x,y;
    vec2 operator+(const vec2<elem_type>& vec)const
    {
        return {x + vec.x, y + vec.y};
    }
    vec2& operator+=(const vec2<elem_type>& vec)
    {
        return *this = *this + vec;
    }
    vec2 operator-(const vec2<elem_type>& vec)const
    {
        return {static_cast<elem_type>(x - vec.x), static_cast<elem_type>(y - vec.y)};
    }
    vec2 operator-()const
    {
        return {-x, -y};
    }
    vec2& operator-=(const vec2<elem_type>& vec)
    {
        return *this = *this - vec;
    }
    vec2 operator*(float factor)const
    {
        return {static_cast<elem_type>(x * factor), static_cast<elem_type>(y * factor)};
    }
    vec2& operator*=(float factor)
    {
        return *this = *this * factor;
    }
    vec2 operator/(float factor)const
    {
        return  {static_cast<elem_type>(x / factor), static_cast<elem_type>(y / factor)};
    }
    vec2& operator/=(float factor)
    {
        return *this = *this / factor;
    }
    bool operator==(const vec2<elem_type>& vec)const
    {
        return x==vec.x&&y==vec.y;
    }

    bool operator!=(const vec2<elem_type>& vec)const
    {
        return !(*this == vec);
    }  

    //returns squared magnitude of vector, faster than mag()
    float mag2()const
    {
        return x * x + y * y;
    }
    //returns magnitude of vector
    float mag()const
    {
        return sqrt(mag2());
    }

    float dot(const vec2<elem_type>& other)const
    {
        return x * other.x + y * other.y;
    }

    vec2 normalised()const
    {
        return *this/mag();
    }
};


