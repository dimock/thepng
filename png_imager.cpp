#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

//#define PNG_DEBUG 3
#include <png.h>

#include "png_imager.h"
#include "helpers.h"

/**
 * read/write PNG to Image
 */

bool PngImager::read(const TCHAR * pngfile, ImageUC & image)
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

	// we don't support other formats yet (
	if ( color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA )
		return false;

	int number_of_passes = png_set_interlace_handling(png_ptr); // ???
	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	/// we suppose that all rows in PNG have equal length!!!
	if ( !image.init(width, height) )
		return false;

	std::vector<png_bytep> rows(image.height());

	if ( color_type == PNG_COLOR_TYPE_RGB )
	{
		for (int i = 0; i < image.height(); ++i)
			rows[i] = reinterpret_cast<png_bytep>(image.get_row(i));
		png_read_image(png_ptr, &rows[0]);
	}
	else if ( color_type == PNG_COLOR_TYPE_RGBA )
	{
		Image<unsigned> image_rgba(width, height);

		/// read in rgba format
		for (int i = 0; i < image.height(); ++i)
			rows[i] = reinterpret_cast<png_bytep>(image_rgba.get_row(i));

		png_read_image(png_ptr, &rows[0]);

		/// convert to rgb format
		for (int i = 0; i < image.height(); ++i)
		{
			unsigned char * src_buffer = reinterpret_cast<unsigned char *>(image_rgba.get_row(i));
			Color3uc * dst_buffer = image.get_row(i);

			for (int j = 0; j < width; ++j, dst_buffer++, src_buffer += 4)
				*dst_buffer = Color3uc(src_buffer[0], src_buffer[1], src_buffer[2]);
		}
	}

	return true;
}

bool PngImager::write(const TCHAR * pngfile, const ImageUC & image)
{
	int color_type = PNG_COLOR_TYPE_RGB;

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
		rows[i] = reinterpret_cast<const unsigned char *>(image.get_row(i));

	png_write_image(png_ptr, (png_bytepp)&rows[0]);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	png_write_end(png_ptr, NULL);

	return true;
}

