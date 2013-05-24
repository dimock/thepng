#pragma once

#include <vector>
#include <math.h>
#include <assert.h>
#include "transf.h"
#include <limits>

//////////////////////////////////////////////////////////////////////////

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

	bool operator == (const Color3 & other) const
	{
		return r_ == other.r_ && g_ == other.g_ && b_ == other.b_;
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

	double module_diff(const Color3 & other) const
	{
		double dr = r_ - other.r_;
		double dg = g_ - other.g_;
		double db = b_ - other.b_;
		return sqrt(dr*dr + dg*dg + db*db);
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

//////////////////////////////////////////////////////////////////////////

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

	template <class C, class A>
	friend bool scale_xy(int factor, const Image<C> & source_image, Image<C> & target_image);

	void make_palette(Image<int> & target_buffer, std::vector<C> & palette, double threshold, size_t maxPaletteSize) const;

	template <class C, class A>
  friend void transform(const Transformd & tr, const Image<C> & source_image, Image<C> & target_image);

	void clear_data()
	{
		for (size_t i = 0; i < buffer_.size(); ++i)
			buffer_[i] = C();
	}

private:

  int width_;
  int height_;
  std::vector<C> buffer_;
};

/**
 * first rotate image around its center,than translate it
 * we scan each line of target image pixel by pixel and calculate corresponding source pixels coords
 * than we take 4 neighbors and calculate average color of result pixel
 * while calculating color we take in account areas of this pixel which it eats from source pixels
 */

template <class C, class A>
void transform(const Transformd & tr, const Image<C> & source_image, Image<C> & target_image)
{
	const int SHIFT = 10;
	const int SCALE = 1<<SHIFT;
	const int MASK = SCALE-1;

  Transformd trI = ~tr;

	const C * src_buffer = source_image.buffer();
  C * dst_buffer = target_image.buffer();
  int dst_xcenter = target_image.width_/2;
  int dst_ycenter = target_image.height_/2;
  int src_xcenter = source_image.width_/2;
  int src_ycenter = source_image.height_/2;

  Vec2d src0 = trI(Vec2d(0, 0));

	int src_x0 = src0.x() * SCALE;
	int src_y0 = src0.y() * SCALE;

	int dxx = trI.rx().x()*SCALE;
	int dyx = trI.ry().x()*SCALE;
  int dxy = trI.rx().y()*SCALE;
	int dyy = trI.ry().y()*SCALE;


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
void Image<C>::make_palette(Image<int> & target_buffer, std::vector<C> & palette, double threshold, size_t maxPaletteSize) const
{
	target_buffer.init(width(), height());

	for (size_t i = 0; i < buffer_.size(); ++i)
	{
		double min_diff = std::numeric_limits<double>::max();
		int nearest_index = -1;
		for (size_t j = 0; j < palette.size(); ++j)
		{
			double diff = buffer_[i].module_diff(palette[j]);
			if ( diff < min_diff )
			{
				nearest_index = j;
				min_diff = diff;
			}
		}

		int paletteIndex = -1;

		// palette isn't initialized yet
		if ( (nearest_index < 0) || (min_diff > threshold && palette.size() < maxPaletteSize) )
		{
			paletteIndex = palette.size();
			palette.push_back(buffer_[i]);
		}
		else
			paletteIndex = nearest_index;

		if ( paletteIndex < 0 )
			continue;

		target_buffer.buffer()[i] = paletteIndex;
	}
}

//////////////////////////////////////////////////////////////////////////

template <class C>
void imageToPalette(const Image<int> & palette_buffer, Image<C> & target_image, std::vector<C> & palette)
{
	target_image.init(palette_buffer.width(), palette_buffer.height());
	size_t size = palette_buffer.width() * palette_buffer.height();

	for (size_t i = 0; i < size; ++i)
	{
		int index = palette_buffer.buffer()[i];
		if ( index < 0 )
			continue;
		target_image.buffer()[i] = palette[index];
	}
}

template <class C>
void normalize_palette(std::vector<C> & palette)
{
	int n = palette.size() / 3;
	if ( n > 0 )
	{
		int deltaC = 255 / n;
		int rgb[3] = { 0, 0, 0 };

		for (size_t i = 0; i < palette.size(); ++i)
		{
			rgb[0] += deltaC;
			for (int j = 0; j < 3; ++j)
			{
				if ( rgb[j] < 256 )
					break;

				rgb[j] = 0;
				rgb[(j+1) % 3] += deltaC;
			}
			if ( rgb[0] > 255 )
				rgb[0] = 0;
			palette[i] = C(rgb[0], rgb[1], rgb[2]);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

typedef Image<Color3uc> ImageUC;
typedef std::vector<ImageUC> ImagesUC;
