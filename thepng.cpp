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

  ImageAligner aligner(images, 1, 50, 8);

  if ( !aligner.findFeatures(0) )
  {
    std::tcout << _T("can't find features\n");
    return -1;
  }

	ImageUC rotated(images[0].width()*1.5, images[0].height()*1.5);
	rotate<Color3uc, Color3u>(3.14*30/180, Vec2i(0, 0), images[0], rotated);

	PngImager::write( _T("..\\..\\..\\data\\temp\\rotated.png"), rotated);

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}