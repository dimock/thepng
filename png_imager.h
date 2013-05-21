#pragma once

#include "image.h"
#include <tchar.h>

class PngImager
{
public:

	static bool read(const TCHAR * pngfile, ImageUC & image);
	static bool write(const TCHAR * pngfile, const ImageUC & image);
};
