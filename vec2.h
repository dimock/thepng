#pragma once

template <class T>
class Vec2
{
  T x_, y_;

public:

  Vec2() : x_(0), y_(0) {}
  Vec2(T x, T y) : x_(x), y_(y) {}

  T x() const { return x_; }
  T y() const { return y_; }
};

typedef Vec2<int> Vec2i;