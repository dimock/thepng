#include <math.h>
#include <assert.h>
#include "image.h"

/**
 * scale image proportionally in both directions
 * destination image pixels have average color of the corresponding source pixels
 */
bool Image::scale_xy(int factor, Image & target_image) const
{
	int wid = width_/factor;
	int hei = height_/factor;

	if ( wid < 1 || hei < 1 )
		return false;

	// allocate target buffer
	// pixels are unsigned, because we have to store sum of source pixels colors
	std::vector<unsigned> temp_buffer(wid*hei*bytes_pp());

	const unsigned char * src_buffer = buffer();
	unsigned * dst_buffer = &temp_buffer[0];
	int dst_pitch = bytes_pp()*wid;

	// sequentially scan source image row by row
	for (int y = 0, y_dst = 0, y_counter = 0; y < height_; ++y, y_counter++, src_buffer += pitch_)
	{
		// we need to go to the next line of target image
		if ( y_counter >= factor )
		{
			y_counter = 0;
			if ( ++y_dst >= hei )
				break;
			dst_buffer += dst_pitch;
		}

		const unsigned char * src_row = src_buffer;
		unsigned * dst_row = dst_buffer;

		for (int x = 0, x_dst = 0, x_counter = 0; x < width_; ++x, x_counter++, src_row += bytes_pp_)
		{
			// take the next taget pixel
			if ( x_counter >= factor )
			{
				x_counter = 0;
				if ( ++x_dst >= wid )
					break;
				dst_row += bytes_pp_;
			}

			dst_row[0] += src_row[0];
			dst_row[1] += src_row[1];
			dst_row[2] += src_row[2];
		}
	}

	// copy average values of the result to the target
	target_image.init(wid, hei, bytes_pp());
	unsigned char * target_buffer = target_image.buffer();
	int factor2 = factor*factor;

	for (size_t i = 0; i < temp_buffer.size(); ++i)
		target_buffer[i] = temp_buffer[i]/factor2;

	return true;
}

/// extract part of image starting from point (x, y) with size npixels in both directions
bool Image::take_part(int x, int y, int npixels, Image & target_image) const
{
  if ( x < 0 || y < 0 || x+npixels > width_ || y+npixels > height_ )
    return false;

  target_image.init(npixels, npixels, bytes_pp_);

  int tpitch = target_image.pitch_;
  const unsigned char * src_buffer = buffer() + pitch_*y;
  unsigned char * dst_buffer = target_image.buffer();

  for (int i = 0; i < npixels; ++i, src_buffer += pitch_, dst_buffer += tpitch)
  {
    const unsigned char * src_buf = src_buffer + x*bytes_pp_;
    unsigned char * dst_buf = dst_buffer;

    for (int j = 0; j < npixels; ++j, src_buf += bytes_pp_, dst_buf += bytes_pp_)
    {
      dst_buf[0] = src_buf[0];
      dst_buf[1] = src_buf[1];
      dst_buf[2] = src_buf[2];
    }
  }

  return true;
}

/**
 * take rotated part of this image and copy it to target. also translate in by XY
 * we scan each line of target image pixel by pixel and calculate corresponding source pixels coords
 * than we take 4 neighbours and calculate average color of result pixel
 * while calculating color we take in account areas of this pixel which it eats from source pixels
 */

void Image::rotate(double angle, int translate_x, int translate_y, Image & target_image) const
{
	const int SHIFT = 10;
	const int SCALE = 1<<SHIFT;
	const int MASK = SCALE-1;

	const unsigned char * src_buffer = buffer();
  unsigned char * dst_buffer = target_image.buffer();
  double sina = sin(angle);
  double cosa = cos(angle);
  int dst_xcenter = target_image.width_/2;
  int dst_ycenter = target_image.height_/2;
  int src_xcenter = width_/2;
  int src_ycenter = height_/2;

	int dst_x0 = -dst_xcenter;
	int dst_y0 = -dst_ycenter;

	int src_x0 = (dst_x0*cosa - dst_y0*sina + src_xcenter - translate_x) * SCALE;
	int src_y0 = (dst_x0*sina + dst_y0*cosa + src_ycenter - translate_y) * SCALE;

	int dxx = cosa*SCALE;
	int dyx = sina*SCALE;
	int dxy = -sina*SCALE;
	int dyy = cosa*SCALE;


  for (int dst_y = 0; dst_y < target_image.height_; ++dst_y, src_x0 += dxy, src_y0 += dyy, dst_buffer += target_image.pitch_)
  {
    unsigned char * dst_buf = dst_buffer;

		int src_x = src_x0;
		int src_y = src_y0;

    for (int dst_x = 0; dst_x < target_image.width_; ++dst_x, src_x += dxx, src_y += dyx, dst_buf += bytes_pp_)
    {
      int xr = src_x >> SHIFT;
			int yr = src_y >> SHIFT;

      if ( xr < 0 || xr+1 >= width_ || yr < 0 || yr+1 >= height_ )
        continue;

			int dx = src_x & MASK;
			int dy = src_y & MASK;

      const unsigned char * src_buf0 = src_buffer + yr*pitch_ + xr*bytes_pp_;
      const unsigned char * src_buf1 = src_buffer + yr*pitch_ + (xr+1)*bytes_pp_;
      const unsigned char * src_buf2 = src_buffer + (yr+1)*pitch_ + xr*bytes_pp_;
      const unsigned char * src_buf3 = src_buffer + (yr+1)*pitch_ + (xr+1)*bytes_pp_;

      for (int i = 0; i < 3; ++i)
      {
        unsigned color = src_buf0[i]*(SCALE-dx)*(SCALE-dy) +
          src_buf1[i]*dx*(SCALE-dy) +
          src_buf2[i]*(SCALE-dx)*dy +
          src_buf3[i]*dx*dy;

				dst_buf[i] = (color >> SHIFT) >> SHIFT;
      }
    }
  }
}

double Image::calc_deviation(const Image & other) const
{
	assert(other.width_ == width_ && other.height_ == height_ && other.bytes_pp_ == bytes_pp_);

	const unsigned char * this_buffer = buffer();
	const unsigned char * other_buffer = other.buffer();

	long long sum = 0;
	int pixelsN = 0;

	for (int y = 0; y < height_; ++y, this_buffer += pitch_, other_buffer += pitch_)
	{
		const unsigned char * this_buf = this_buffer;
		const unsigned char * other_buf = other_buffer;

		for (int x = 0; x < width_; ++x, this_buf += bytes_pp_, other_buf += bytes_pp_)
		{
			unsigned this_color = *this_buf & 0xffffff;
			unsigned other_color = *other_buf & 0xffffff;

			// if color is NULL we suppose that pixel unused
			if ( !this_color || !other_color )
				continue;

			int dr = this_buf[0] - other_buf[0];
			int dg = this_buf[1] - other_buf[1];
			int db = this_buf[2] - other_buf[2];

			sum += dr*dr + dg*dg + db*db;
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

double Image::take_variation() const
{
  const unsigned char * buff = buffer();

  long long rsum  =0;
  long long gsum = 0;
  long long bsum = 0;
  long long rsum2 = 0;
  long long gsum2 = 0;
  long long bsum2 = 0;
  int count = 0;
  size_t num = width_ * height_;

  for (size_t i = 0; i < num; ++i, buff += bytes_pp_)
  {
    if ( !buff[0] && !buff[1] && !buff[2] )
      continue;

    rsum += buff[0];
    gsum += buff[1];
    bsum += buff[2];

    rsum2 += buff[0]*buff[0];
    gsum2 += buff[1]*buff[1];
    bsum2 += buff[2]*buff[2];

    count++;
  }

  if ( !count )
    return -1.0;

  double rvar = sqrt((double)rsum2 - rsum*rsum/count) / count;
  double gvar = sqrt((double)gsum2 - gsum*gsum/count) / count;
  double bvar = sqrt((double)bsum2 - bsum*bsum/count) / count;

  double var = rvar + gvar + bvar;

  return var;
}

void Image::clear_data()
{
  for (size_t i = 0; i < buffer_.size(); ++i)
    buffer_[i] = 0;
}
