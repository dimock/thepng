#pragma once

#include <vector>
#include <tchar.h>

class Image
{
public:

	Image() :
		width_(0), height_(0), bytes_pp_(0), pitch_(0)
	{}

	Image(int wid, int hei, int bpp) :
		width_(wid), height_(hei), bytes_pp_(bpp)
	{
		pitch_ = width_*bytes_pp_;
		buffer_.resize( width()*height()*bytes_pp() );
	}

	bool init(int wid, int hei, int bpp)
	{
		if ( wid < 1 || hei < 1 || bpp < 1 )
			return false;

		width_ = wid;
		height_ = hei;
		bytes_pp_ = bpp;
		pitch_ = width_*bytes_pp_;

		buffer_.resize( width()*height()*bytes_pp() );

		return true;
	}

	int width() const { return width_; }
	int height() const { return height_; }
	int bytes_pp() const { return bytes_pp_; }

	const unsigned char * buffer() const { return buffer_.empty() ? 0 : &buffer_[0]; }
	unsigned char * buffer() { return buffer_.empty() ? 0 : &buffer_[0]; }

	unsigned char * get_row(int i) { return buffer() + pitch_ * i; }
	const unsigned char * get_row(int i) const { return buffer() + pitch_ * i; }

	bool scale_xy(int times, Image & target_image) const;
  bool take_part(int x, int y, int npixels, Image & target_image) const;
  void rotate(double angle, int translate_x, int translate_y, Image & target_image) const;
  
  void clear_data();

private:

	int width_;
	int height_;
	int bytes_pp_;
	int pitch_;
	std::vector<unsigned char> buffer_;
};

class PngImager
{
public:

	static bool read(const TCHAR * pngfile, Image & image);
	static bool write(const TCHAR * pngfile, const Image & image);
};