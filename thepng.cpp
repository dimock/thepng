#include "png_imager.h"
#include "image_aligner.h"
#include "helpers.h"
#include <iomanip>
#include <math.h>
#include <float.h>
#include "vec2.h"

int _tmain(int argc, TCHAR **argv)
{
	setlocale(0, "");

	if (argc < 3)
	{
		std::tstring exename(argv[0]);
		replace_delimiter(exename);
		std::tstring exefilename = extract_filename(exename);
		std::tcout << _T("usage:\n  ") << exefilename.c_str() << _T(" source1.png source2.png ... [need at least 2 files]") << std::endl;
		return -1;
	}

  TestTimer tt;

	std::vector<ImageUC> images(argc-1);

	for (int i = 1; i < argc; ++i)
	{
		if ( !PngImager::read(argv[i], images[i-1]) )
		{
			std::tcout << _T("unable to read source image: ") << argv[1] << std::endl;
			return -1;
		}
	}

	//std::vector<ImageUC> scaled(images.size());
	//for (size_t i = 0; i < images.size(); ++i)
	//	scale_xy<Color3uc, Color3u>(2, images[i], scaled[i]);

	std::vector<Color3uc> palette;
	double threshold = 15.0;
	int maxPaletteSize = 16;
	std::vector<Image<int>> paletteBuffers(images.size());

	for (size_t i = 0; i < images.size(); ++i)
	{
		images[i].make_palette(paletteBuffers[i], palette, threshold, maxPaletteSize);
	}

	normalize_palette(palette);

	std::vector<ImageUC> paletteImages(images.size());
	for (size_t i = 0; i < images.size(); ++i)
	{
		findBoundaries(paletteBuffers[i]);
		imageToPalette(paletteBuffers[i], paletteImages[i], palette);

		TCHAR fname[256];
		_stprintf(fname, _T("..\\..\\..\\data\\temp\\palette_%d.png"), i);
		PngImager::write(fname, paletteImages[i]);
	}

	for (size_t i = 0; i < paletteBuffers.size(); ++i)
	{
		Contours contours;
		vectorize(paletteBuffers[i], contours, 20);

		TCHAR fname[256];
		_stprintf(fname, _T("..\\..\\..\\data\\temp\\contours_%d.png"), i);
		saveCountours(fname, contours);
	}


  //ImageAligner aligner(images, 8, 16);

  //double diff = aligner.align(0, 1);
  //if ( diff < 0 )
  //{
  //  std::tcout << _T("can't align images\n");
  //  return -1;
  //}

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}