#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//#define PNG_DEBUG 3
#include <png.h>

#include "png_imager.h"

class FileWrapper
{
	FILE * fp_;

public:

	FileWrapper(const TCHAR * filename, const TCHAR * mode) :
		fp_(NULL)
	{
		_tfopen_s(&fp_, filename, mode);
	}

	operator FILE * ()
	{
		return fp_;
	}

	~FileWrapper()
	{
		if ( fp_ != NULL )
			fclose(fp_);
	}
};

/**
 * read/write PNG to Image
 */

bool PngImager::read(const TCHAR * pngfile, Image & image)
{
	png_byte header[8];
	FileWrapper fp(pngfile, _T("rb"));
	if (!fp)
		return false;

	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		return false;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		return false;

	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if ( bit_depth != 8 ) // in this simple example we don't consider PNG depth to be not equal to 8 bit )
		return false;

	int bytes_pp = 0;
	switch ( color_type )
	{
	case PNG_COLOR_TYPE_RGB:
		bytes_pp = 3;
		break;

	case PNG_COLOR_TYPE_RGBA:
		bytes_pp = 4;
		break;

	// we don't support other formats yet (
	default:
		return false;
	}

	int number_of_passes = png_set_interlace_handling(png_ptr); // ???
	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	/// we suppose that all rows in PNG have equal length!!!
	if ( !image.init(width, height, bytes_pp) )
		return false;

	std::vector<png_bytep> rows(image.height());
	for (int i = 0; i < image.height(); ++i)
		rows[i] = image.get_row(i);

	png_read_image(png_ptr, &rows[0]);

	return true;
}

bool PngImager::write(const TCHAR * pngfile, const Image & image)
{
	int color_type = 0;
	switch ( image.bytes_pp() )
	{
	case 3:
		color_type = PNG_COLOR_TYPE_RGB;
		break;

	case 4:
		color_type = PNG_COLOR_TYPE_RGBA;
		break;

	// don't support other types
	default:
		return false;
	}

	/* create file */
	FileWrapper fp(pngfile, _T("wb"));
	if (!fp)
		return false;


	/* initialize stuff */
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		return false;

	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	png_init_io(png_ptr, fp);

	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	// suppose it is always = 8
	int bit_depth = 8;

	png_set_IHDR(png_ptr, info_ptr, image.width(), image.height(),
		bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	std::vector<const unsigned char *> rows(image.height());
	for (int i = 0; i < image.height(); ++i)
		rows[i] = image.get_row(i);

	png_write_image(png_ptr, (png_bytepp)&rows[0]);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	png_write_end(png_ptr, NULL);

	return true;
}


/**
 * scale image proportionally in both directions
 * destination image pixels have average color of the corresponding source pixels
 */
bool Image::scale_xy(int times, Image & target_image) const
{
	int wid = width_/times;
	int hei = height_/times;

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
		if ( y_counter >= times )
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
			if ( x_counter >= times )
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
	int times2 = times*times;

	for (size_t i = 0; i < temp_buffer.size(); ++i)
		target_buffer[i] = temp_buffer[i]/times2;

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

bool Image::rotate(double angle)
{
  return false;
}
