#include "png_imager.h"
#include "helpers.h"
#include <iomanip>
#include <math.h>

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

  //Image part;
  //if ( !image.take_part(286, 494, 142, part) )
  //{
  //  std::cout << _T("can't take part of the image") << std::endl;
  //  return -1;
  //}

	std::tstring iname(argv[1]);
	replace_delimiter(iname);
	std::tstring dirname = extract_dirname(iname);

  int side = sqrt( (double)(image.width()*image.width()+image.height()*image.height()) );
  Image rotated(side, side, image.bytes_pp());

  for (int alpha = 0, i = 0; alpha < 360; alpha += 10, ++i)
  {
    TCHAR fname[256];
    _stprintf(fname, _T("%s_%02d.png"), argv[2], i);
		std::tstring outname = dirname + std::tstring(fname);
    image.rotate(3.14*alpha/180.0, 0, 0, rotated);

	  if ( !PngImager::write(outname.c_str(), rotated) )
	  {
		  std::tcout << _T("unable to write target image: ") << argv[2] << std::endl;
		  return -1;
	  }

    std::tcout << outname << std::endl;

    rotated.clear_data();
  }

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}