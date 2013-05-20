#include "png_imager.h"
#include "helpers.h"
#include <iomanip>
#include <math.h>
#include <float.h>

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

	std::vector<Image> images(argc-1);

	for (int i = 1; i < argc; ++i)
	{
		if ( !PngImager::read(argv[i], images[i-1]) )
		{
			std::tcout << _T("unable to read source image: ") << argv[1] << std::endl;
			return -1;
		}
	}

	std::vector<Image> scaled(images.size());
	int scaleTimes = 4;

	for (size_t i = 0; i < images.size(); ++i)
	{
		if ( !images[i].scale_xy(scaleTimes, scaled[i]) )
		{
			std::tcout << _T("Can't scale image ") << scaleTimes << _T(" times in xy directions") << std::endl;
			return -1;
		}
	}

  //Image part;
  //if ( !image.take_part(286, 494, 142, part) )
  //{
  //  std::cout << _T("can't take part of the image") << std::endl;
  //  return -1;
  //}

	std::tstring iname(argv[1]);
	replace_delimiter(iname);
	std::tstring dirname = extract_dirname(iname);

	int deltaA = 5;
	int rotsN = 360/deltaA;
	int num = 0;

	int width = scaled[0].width();
	int height = scaled[0].height();

	int width1 = scaled[1].width();
	int height1 = scaled[1].height();

	int side = width;
	if ( height < side )
		side = height;
	if ( width1 < side )
		side = width1;
	if ( height1 < side )
		side = height1;

	//side--;

	side = 48;

	int dtx = width  / 4; dtx++;
	int dty = height / 4; dty++;

	int best_tx = 0, best_ty = 0;
	int part_x, part_y;
	Image best;
	double deviation = DBL_MAX;
	int alpha_best = 0;

	for (int ty = dty; ty < height; ty += dty)
	{
		for (int tx = dtx; tx < width; tx += dtx, num++)
		{
			std::vector<Image> rotated(rotsN);
			for (size_t i = 0; i < rotated.size(); ++i)
				rotated[i].init(side, side, scaled[0].bytes_pp());

			int tr_x = -width /2 + tx;
			int tr_y = -height/2 + ty;

			for (int alpha = 0, i = 0; alpha < 360 && i < rotsN; alpha += deltaA, ++i)
			{
				TCHAR fname[256];
				_stprintf(fname, _T("result//rot_%02d_%02d.png"), num, i);
				std::tstring outname = dirname + std::tstring(fname);

				scaled[0].rotate(3.14*alpha/180.0, tr_x, tr_y, rotated[i]);

				//if ( !PngImager::write(outname.c_str(), rotated[i]) )
				//{
				//	std::tcout << _T("unable to write target image: ") << outname.c_str() << std::endl;
				//	return -1;
				//}

				std::tcout << outname << std::endl;
			}

			for (int y = 0; y < height1; ++y)
			{
				for (int x = 0; x < width1; ++x)
				{
					Image part;
					if ( !scaled[1].take_part(x, y, side, part) )
						break;

					for (size_t j = 0; j < rotated.size(); ++j)
					{
						double devi = part.calc_deviation(rotated[j]);
						if ( devi < 0 )
							continue;

						if ( devi < deviation )
						{
							deviation = devi;
							best_tx = (x - tx)*scaleTimes;
							best_ty = (y - ty)*scaleTimes;
							part_x = x;
							part_y = y;
							alpha_best = j*deltaA;
							best = rotated[j];
						}
					}
				}
			}
		}
	}

	Image result(images[0].width(), images[0].height(), images[0].bytes_pp());
	images[0].rotate(3.14*alpha_best/180, best_tx, best_ty, result);

	std::tstring result_fname = dirname + std::tstring(_T("result//result.png"));
	PngImager::write(result_fname.c_str(), result);

	Image part;
	if ( scaled[1].take_part(part_x, part_y, side, part) )
	{
		std::tstring part_fname = dirname + std::tstring(_T("result//part.png"));
		PngImager::write(part_fname.c_str(), part);

		std::tstring scaled_fname = dirname + std::tstring(_T("result//scaled.png"));
		PngImager::write(scaled_fname.c_str(), best);
	}

  double dt = tt.getTimeMs();

  std::tcout << _T("time used: ") << std::setprecision(4) << dt << _T(" ms") << std::endl;

	return 0;
}