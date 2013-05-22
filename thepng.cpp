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

  double threshold = 15.0;
  size_t maxPaletteSize = 16;
  size_t minContourSize = 100;
  size_t featuresMax = 20;
  size_t similarMax = 5;
  size_t correlatedNum = 5;

  align(images, threshold, maxPaletteSize, minContourSize, featuresMax, similarMax, correlatedNum);

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}