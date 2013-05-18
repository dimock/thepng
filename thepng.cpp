#include "png_imager.h"
#include "helpers.h"
#include <iomanip>


int _tmain(int argc, TCHAR **argv)
{
	setlocale(0, "");

	if (argc != 3)
	{
		std::tstring exename(argv[0]);
		replace_delimiter(exename);
		std::tstring exefilename = extract_filename(exename);
		std::tcout << _T("usage:\n  ") << exefilename.c_str() << _T(" source.png target.png") << std::endl;
		return -1;
	}

  TestTimer tt;

	Image image;
	if ( !PngImager::read(argv[1], image) )
	{
		std::tcout << _T("unable to read source image: ") << argv[1] << std::endl;
		return -1;
	}

	//Image scaled;
	//int times = 8;
	//if ( !image.scale_xy(times, scaled) )
	//{
	//	std::tcout << _T("Can't scale image ") << times << _T(" times in xy directions") << std::endl;
	//	return -1;
	//}

  Image part;
  if ( !image.take_part(328, 536, 100, part) )
  {
    std::cout << _T("can't take part of the image") << std::endl;
    return -1;
  }

	if ( !PngImager::write(argv[2], part) )
	{
		std::tcout << _T("unable to write target image: ") << argv[2] << std::endl;
		return -1;
	}

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}