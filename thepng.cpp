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

	std::vector<ImageUC> scaled(images.size());
	for (size_t i = 0; i < images.size(); ++i)
		scale_xy<Color3uc, Color3u>(2, images[i], scaled[i]);

	std::vector<Color3uc> palette;
	double threshold = 15.0;
	int maxPaletteSize = 16;
	std::vector<Image<int>> paletteBuffers(images.size());

	for (size_t i = 0; i < images.size(); ++i)
	{
		scaled[i].make_palette(paletteBuffers[i], palette, threshold, maxPaletteSize);
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

	std::vector<Features> features_arr(paletteBuffers.size());

	for (size_t i = 0; i < paletteBuffers.size(); ++i)
	{
		vectorize(paletteBuffers[i], features_arr[i], 100, 30);

		TCHAR fname[256];
		_stprintf(fname, _T("..\\..\\..\\data\\temp\\contours_%d.txt"), i);
		saveContours(fname, features_arr[i]);
	}

	for (size_t i = 0; i < features_arr.size(); ++i)
	{
		Features & features = features_arr[i];
		for (size_t j = 0; j < features.size(); ++j)
			features[j].prepare();
	}

	size_t i0, i1;
	double s = findClosestFeatures(features_arr[0], features_arr[1], i0, i1);

	std::tcout << _T("Similarity = ") << s << std::endl;

	saveContour(_T("..\\..\\..\\data\\temp\\closest_0.txt"), features_arr[0][i0].contour_);
	saveContour(_T("..\\..\\..\\data\\temp\\closest_1.txt"), features_arr[1][i1].contour_);

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}