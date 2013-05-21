#pragma once

#include <vector>
#include <math.h>
#include <assert.h>
#include "vec2.h"

template <class T>
class Color3
{
	T r_, g_, b_;

public:

	Color3(T r, T g, T b) : r_(r), g_(g), b_(b) {}
	Color3() : r_(0), g_(0), b_(0) {}

	template <class O>
	Color3(const Color3<O> & other)
	{
		r_ = other.r();
		g_ = other.g();
		b_ = other.b();
	}

	template <class O>
	Color3<T> & operator = (const Color3<O> & other)
	{
		r_ = other.r();
		g_ = other.g();
		b_ = other.b();
		return *this;
	}

	operator bool () const
	{
		return r_ != 0 || g_ != 0 || b_ != 0;
	}

	Color3 operator + (const Color3 & other) const
	{
		return Color3(r_+other.r_, g_+other.g_, b_+other.b_);
	}

	Color3 operator - (const Color3 & other) const
	{
		return Color3(r_-other.r_, g_-other.g_, b_-other.b_);
	}

	Color3 operator / (int divider) const
	{
		return Color3(r_/divider, g_/divider, b_/divider);
	}

	Color3 operator * (int multiplier) const
	{
		return Color3(r_*multiplier, g_*multiplier, b_*multiplier);
	}

	Color3 operator * (const Color3 & other) const
	{
		return Color3(r_*other.r_, g_*other.g_, b_*other.b_);
	}

	Color3 & operator += (const Color3 & other)
	{
		r_ += other.r_;
		g_ += other.g_;
		b_ += other.b_;
		return *this;
	}

	template <class O>
	Color3 & operator += (const Color3<O> & other)
	{
		r_ += other.r();
		g_ += other.g();
		b_ += other.b();
		return *this;
	}


	Color3 & operator *= (const int multiplicator)
	{
		r_ *= multiplicator;
		g_ *= multiplicator;
		b_ *= multiplicator;
		return *this;
	}

	Color3 & operator /= (const int divider)
	{
		r_ /= divider;
		g_ /= divider;
		b_ /= divider;
		return *this;
	}

	Color3 operator >> (int shift)
	{
		return Color3(r_>>shift, g_>>shift, b_>>shift);
	}

	T r() const { return r_; }
	T g() const { return g_; }
	T b() const { return b_; }
};

typedef Color3<int> Color3i;
typedef Color3<unsigned char> Color3uc;
typedef Color3<unsigned> Color3u;

template <class C>
class Image
{
public:

  Image() : width_(0), height_(0)
  {}

  Image(int wid, int hei) : width_(wid), height_(hei)
  {
    buffer_.resize( width() * height() );
  }

  bool init(int wid, int hei)
  {
    if ( wid < 1 || hei < 1 )
      return false;

    width_ = wid;
    height_ = hei;

    buffer_.resize( width() * height() );

    return true;
  }

  int width() const { return width_; }
  int height() const { return height_; }

  const C * buffer() const { return buffer_.empty() ? 0 : &buffer_[0]; }
  C * buffer() { return buffer_.empty() ? 0 : &buffer_[0]; }

  const C & operator [] (size_t i) const { return buffer_.at(i); }
  C & operator [] (size_t i) { return buffer_.at(i); }

  C * get_row(int i) { return buffer() + width_ * i; }
  const C * get_row(int i) const { return buffer() + width_ * i; }

  bool take_part(int x, int y, int npixels, Image & target_image) const;

	template <class C, class A>
  friend void rotate(double angle, const Vec2i & translate, const Image<C> & source_image, Image<C> & target_image);

  // it is supposed that images have equal size
  double calc_deviation(const Image & other) const;

	template <class C, class A>
	friend double calc_variation(const Image<C> & image, C & average);

	void	 clear_data();

private:

  int width_;
  int height_;
  std::vector<C> buffer_;
};


/// extract part of image starting from point (x, y) with size npixels in both directions
template <class C>
bool Image<C>::take_part(int x, int y, int npixels, Image & target_image) const
{
  if ( x < 0 || y < 0 || x+npixels > width_ || y+npixels > height_ )
    return false;

  target_image.init(npixels, npixels);

  const C * src_buffer = buffer() + width_*y;
  C * dst_buffer = target_image.buffer();

  for (int i = 0; i < target_image.height_; ++i, src_buffer += width_, dst_buffer += target_image.width_)
  {
    const C * src_buf = src_buffer + x;
    C * dst_buf = dst_buffer;

    for (int j = 0; j < target_image.width_; ++j, src_buf++, dst_buf++)
      *dst_buf = *src_buf;
  }

  return true;
}

/**
 * take rotated part of this image and copy it to target. also translate in by XY
 * we scan each line of target image pixel by pixel and calculate corresponding source pixels coords
 * than we take 4 neighbors and calculate average color of result pixel
 * while calculating color we take in account areas of this pixel which it eats from source pixels
 */

template <class C, class A>
void rotate(double angle, const Vec2i & translate, const Image<C> & source_image, Image<C> & target_image)
{
	const int SHIFT = 10;
	const int SCALE = 1<<SHIFT;
	const int MASK = SCALE-1;

	const C * src_buffer = source_image.buffer();
  C * dst_buffer = target_image.buffer();
  double sina = sin(angle);
  double cosa = cos(angle);
  int dst_xcenter = target_image.width_/2;
  int dst_ycenter = target_image.height_/2;
  int src_xcenter = source_image.width_/2;
  int src_ycenter = source_image.height_/2;

	int dst_x0 = -dst_xcenter;
	int dst_y0 = -dst_ycenter;

	int src_x0 = (dst_x0*cosa - dst_y0*sina + src_xcenter - translate.x()) * SCALE;
	int src_y0 = (dst_x0*sina + dst_y0*cosa + src_ycenter - translate.y()) * SCALE;

	int dxx = cosa*SCALE;
	int dyx = sina*SCALE;
	int dxy = -sina*SCALE;
	int dyy = cosa*SCALE;


  for (int dst_y = 0; dst_y < target_image.height_; ++dst_y, src_x0 += dxy, src_y0 += dyy, dst_buffer += target_image.width_)
  {
    C * dst_buf = dst_buffer;

		int src_x = src_x0;
		int src_y = src_y0;

    for (int dst_x = 0; dst_x < target_image.width_; ++dst_x, src_x += dxx, src_y += dyx, dst_buf++)
    {
      int xr = src_x >> SHIFT;
			int yr = src_y >> SHIFT;

      if ( xr < 0 || xr+1 >= source_image.width_ || yr < 0 || yr+1 >= source_image.height_ )
        continue;

			int dx = src_x & MASK;
			int dy = src_y & MASK;

      const C * src_buf0 = src_buffer + yr*source_image.width_ + xr;
      const C * src_buf1 = src_buffer + yr*source_image.width_ + (xr+1);
      const C * src_buf2 = src_buffer + (yr+1)*source_image.width_ + xr;
      const C * src_buf3 = src_buffer + (yr+1)*source_image.width_ + (xr+1);

			A c0 = *src_buf0;
			A c1 = *src_buf1;
			A c2 = *src_buf2;
			A c3 = *src_buf3;

			A color = c0*(SCALE-dx)*(SCALE-dy) + c1*dx*(SCALE-dy) +	c2*(SCALE-dx)*dy + c3*dx*dy;

			*dst_buf = color >> 2*SHIFT;
    }
  }
}

template <class C>
double Image<C>::calc_deviation(const Image<C> & other) const
{
	assert(other.width_ == width_ && other.height_ == height_ && other.bytes_pp_ == bytes_pp_);

	const C * this_buffer = buffer();
	const C * other_buffer = other.buffer();

	unsigned long long sum = 0;
	int pixelsN = 0;

	for (int y = 0; y < height_; ++y, this_buffer += width_, other_buffer += width_)
	{
		const unsigned char * this_buf = this_buffer;
		const unsigned char * other_buf = other_buffer;

		for (int x = 0; x < width_; ++x, this_buf++, other_buf++)
		{
			// if color is NULL we suppose that pixel unused
			if ( !*this_buf || !*other_buf )
				continue;

			Color3<unsigned long long> this_color = *this_buf;
			Color3<unsigned long long> other_color = *other_buf;
			Color3<unsigned long long> color = this_color - other_color;
			color = color * color;

			sum += color.r() + color.g() + color.b();
			pixelsN++;
		}
	}

	if ( 0 == pixelsN )
		return -1.0;

	double deviation = sqrt((double)sum)/pixelsN;
	if ( deviation == 0 )
		return 0;

	return deviation;
}


template <class C, class A>
double calc_variation(const Image<C> & image, C & average)
{
  A sum, sum2;
  int count = 0;

	const C * buffer = image.buffer();
	for (size_t i = 0; i < image.buffer_.size(); ++i, buffer++)
  {
    if ( !*buffer )
      continue;

		A a = *buffer;
    sum  += a;
    sum2 += a * a;

    count++;
  }

  if ( !count )
    return -1.0;

	A result = sum2 - sum*sum/count;
	average = sum / count;

	/// HACK. it is supposed that A is color type !
	double var = ( sqrt((double)result.r()) + sqrt((double)result.g()) + sqrt((double)result.b()) ) / count;

  return var;
}

template <class C>
void Image<C>::clear_data()
{
  for (size_t i = 0; i < buffer_.size(); ++i)
    buffer_[i] = C();
}


typedef Image<Color3uc> ImageUC;
typedef std::vector<ImageUC> ImagesUC;