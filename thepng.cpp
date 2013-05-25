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
  double t0 = tt.getTimeMs();

	std::vector<ImageUC> images(argc-1);

	for (int i = 1; i < argc; ++i)
	{
    std::tstring fname(argv[i]);
    replace_backslashes(fname);
    std::tcout << "Loading " << fname.c_str() << std::endl;
		if ( !PngImager::read(argv[i], images[i-1]) )
		{
			std::tcout << _T("unable to read source image: ") << argv[1] << std::endl;
			return -1;
		}
	}

  double t_load = tt.getTimeMs();
  std::tcout << _T("Images loading took: ") << std::setprecision(2) << t_load*0.001 << _T(" s") << std::endl;

	Params params;
	std::tstring firstname(argv[1]);
	replace_backslashes(firstname);
	params.outname_ = extract_dirname(firstname) + std::tstring(_T("result.png"));

	ImageAligner aligner(images);
	aligner.setParams(params);
  if ( !aligner.align() )
  {
    std::tcout << _T("Images alignment failed") << std::endl;
    return -1;
  }

  double t_align = tt.getTimeMs() - t_load;
  std::tcout << _T("Alignment took: ") << std::setprecision(2) << t_align*0.001 << _T(" s") << std::endl;

  aligner.writeResult();

  double t_write = tt.getTimeMs() - t_align;
  std::tcout << _T("Saving results took: ") << std::setprecision(2) << t_write*0.001 << _T(" s") << std::endl;


  double t_all = tt.getTimeMs();
  std::tcout << _T("Whole time used: ") << std::setprecision(2) << t_all*0.001 << _T(" s") << std::endl;

	return 0;
}
