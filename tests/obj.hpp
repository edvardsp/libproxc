
#pragma once

#include <algorithm>
#include <iostream>

class Obj
{
private:
    std::size_t value_;

public:
    Obj()
        : value_{}
    {}

    Obj( std::size_t value )
        : value_{ value }
    {}

    Obj( Obj const & ) = default;
    Obj & operator = ( Obj const & ) = default;

    Obj( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
    }
    Obj & operator = ( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
        return *this;
    }

    bool operator == ( Obj const & other )
    { return value_ == other.value_; }

    template<typename CharT, typename TraitsT>
    friend std::basic_ostream< CharT, TraitsT > &
    operator << ( std::basic_ostream< CharT, TraitsT > & out, Obj const & obj )
    { return out << "Obj{" << obj.value_ << "}"; }
};

