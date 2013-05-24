#include "png_imager.h"
#include "image_aligner.h"
#include "helpers.h"
#include <iomanip>
#include <math.h>
#include <float.h>


int _tmain(int argc, TCHAR ** argv)
{
	setlocale(0, "");

	if ( argc < 3 )
	{
		std::tstring exename(argv[0]);
		replace_backslashes(exename);
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

	//_T("..\\..\\..\\data\\temp\\result.png")

	Params params;
	std::tstring firstname(argv[1]);
	replace_backslashes(firstname);
	params.outname_ = extract_dirname(firstname) + std::tstring(_T("result.png"));

	ImageAligner aligner(images);
	aligner.setParams(params);
  aligner.align();

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}
