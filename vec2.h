#pragma once

class Vec2
{
  int x_, y_;

public:

  Vec2() : x_(0), y_(0) {}
  Vec2(int x, int y) : x_(x), y_(y) {}

  int x() const { return x_; }
  int y() const { return y_; }
};