#include "image_aligner.h"
#include "helpers.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <limits>

ImageAligner::ImageAligner(ImagesUC & images, int scaleFactor, int featuresCount) :
  images_(images), scaleFactor_(scaleFactor), featuresCount_(featuresCount)
{
  scaled_.resize(images_.size());
  for (size_t i = 0; i < images_.size(); ++i)
  {
    if ( !scale_xy<Color3uc, Color3u>(scaleFactor_, images[i], scaled_[i]) )
    {
      std::tcout << _T("Can't scale image ") << std::endl;
      return;
    }
  }
}

bool ImageAligner::findFeatures(int index)
{
  if ( !collectFeatures(index) )
    return false;

  for (int i = 0; i < features_.size(); ++i)
  {
		Color3uc color = features_[i].color();

		std::tcout << _T("Feature(") << i << _T(") have variation ") << features_[i].variation() << _T(" and average color (") << 
			color.r() << _T(", ") << color.g() << _T(", ") << color.b() << _T(")\n");
  }

  TCHAR fname[256];
  _stprintf(fname, _T("..//..//..//data//temp//feature_%02d.png"), index);


  ImageUC image;
  featuresToImage(scaled_[index], image);
  PngImager::write(fname, image);

  return true;
}

bool ImageAligner::collectFeatures(int index)
{
  if ( featuresCount_ < 1 || (size_t)index >= scaled_.size() )
    return false;

	srand(time(0));

  Image<FeatureUC> & img = scaled_[index];

	int offset_x = 2;
	int offset_y = 2;

  int w = img.width()  - offset_x*2 - scaleFactor_;
  int h = img.height() - offset_x*2 - scaleFactor_;

  features_.resize(featuresCount_);

	for (int i = 0; i < featuresCount_; ++i)
	{
		int x = (((double)rand()) / RAND_MAX) * w;
		int y = (((double)rand()) / RAND_MAX) * h;

    int j = x + y * img.width();
    features_[i] = img[j];
	}

	return true;
}

double ImageAligner::align(int index1, int index2)
{
  if ( (size_t)index2 >= scaled_.size()  || !collectFeatures(index1) )
    return -1.0;

  double difference = std::numeric_limits<double>::max();
  int x_best = 0;
  int y_best = 0;
  int angle_best = 0;
  int featuresCoinsides = 0;
  double threshold = 5.0;

  Image<FeatureUC> & image1 = scaled_[index1];
  Image<FeatureUC> & image2 = scaled_[index2];

  for (int y = -image1.height(); y < image2.height(); ++y)
  {
    for (int x = -image1.width(); x < image2.width(); ++x)
    {
      for (int angle = -15; angle <= 15; angle += 1)
      {
        double diff = 0;
        int count = 0;

        double sina = sin(angle*3.1415/180.0);
        double cosa = cos(angle*3.1415/180.0);

        for (size_t i = 0; i < features_.size(); ++i)
        {
          const FeatureUC & feature1 = features_[i];
          int tx = feature1.transform().x() - image1.width()/2;
          int ty = feature1.transform().y() - image1.height()/2;
          double fx = cosa*tx + sina*ty + image1.width()/2;
          double fy =-sina*tx - cosa*ty + image1.height()/2;
          int xx = x + fx;
          int yy = y + fy;

          if ( (unsigned)xx >= image2.width() || (unsigned)yy >= image2.height() )
            continue;

          int j = xx + yy*image2.width();
          const FeatureUC & feature2 = image2[j];
          diff += feature1.difference(feature2);
          count++;
        }

        if ( count < 4 )
          continue;

        diff /= count;

        if ( diff < difference )
        {
          difference = diff;
          x_best = x;
          y_best = y;
          angle_best = angle;
          featuresCoinsides = count;
        }
      }
    }
  }

  int width = images_[index2].width();
  int height = images_[index2].height();

  //if ( width < images_[index2].width() )
  //  width = images_[index2].width();
  //if ( height < images_[index2].height() )
  //  height = images_[index2].height();

  x_best *= scaleFactor_;
  y_best *= scaleFactor_;

  ImageUC image(width, height);
  rotate<Color3uc, Color3u>(0, Vec2i(0, 0), images_[index2], image);
  rotate<Color3uc, Color3u>(angle_best*3.1415/180.0, Vec2i(x_best, y_best), images_[index1], image);

  std::tcout << _T("x = ") << x_best << _T(", y = ") << y_best << _T(", angle = ") << angle_best << std::endl;
  std::tcout << _T("difference = ") << difference <<  _T(", number of features found = ") << featuresCoinsides << std::endl;

  PngImager::write( _T("..\\..\\..\\data\\temp\\rotated.png"), image );

  return difference;
}

void findBoundaries(Image<int> & image)
{
	int * buff = image.buffer();
	int * prev = 0;
	for (int y = 0; y < image.height(); ++y, buff += image.width())
	{
		int * buf = buff;
		int * next = y < image.height()-1 ? buff + image.width() : 0;
		int * nxt = next;
		int * prv = prev;

		for (int x = 0; x < image.width(); ++x, ++buf, ++nxt, ++prv)
		{
			int c = *buf;
			if ( x > 0 && buf[-1] != -1 && c != buf[-1] )
				continue;
			if ( x < image.width()-1 && buf[1] != -1 && c != buf[1] )
				continue;
			if ( next )
			{
				if ( c != *nxt )
					continue;
				if ( x > 0 && c != nxt[-1] )
					continue;
				if ( x < image.width()-1 && c != nxt[1] )
					continue;
			}
			if ( prev )
			{
				if ( *prv != -1 && c != *prv )
					continue;
				if ( x > 0 && prv[-1] != -1 && c != prv[-1] )
					continue;
				if ( x < image.width()-1 && prv[1] != -1 && c != prv[1] )
					continue;
			}
			*buf = -1;
		}

		prev = buff;
	}
}

void writeCountour(Image<int> & image, int x, int y, Contour & contour)
{
	static const int offsets[8][2] = { {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1} };

	int * buffer = image.buffer();
	int n0 = x + image.width()*y;
	int colorIndex = buffer[n0];
	int i = 0;
	bool ok = true;

	for ( ; ok; )
	{
		contour.push_back( Vec2d(x, y) );
		ok = false;

		for (int j = 0; j < 8; ++j, ++i)
		{
			if ( i > 7 )
				i = 0;

			int x1 = x + offsets[i][0];
			int y1 = y + offsets[i][1];

			if ( x1 < 0 || x1 >= image.width() || y1 < 0 || y1 >= image.height() )
				continue;

			int n1 = x1 + y1*image.width();
			if ( n1 == n0 )
				break;

			if ( buffer[n1] == -1 || buffer[n1] != colorIndex )
				continue;

			x = x1;
			y = y1;
			i = (i + 4) % 8;
			i++;
			if ( i > 7 )
				i = 0;

			ok = true;
			break;
		}
	}

	for (size_t i = 0; i < contour.size(); ++i)
	{
		Vec2i v = contour[i];
		int index = image.width()*v.y() + v.x();
		buffer[index] = -1;
	}
}

class CompareContours
{
public:

	bool operator () (const Contour & c1, const Contour & c2) const
	{
		return c1.size() > c2.size();
	}
};

void vectorize(Image<int> & image, Contours & contours, size_t minLength)
{
	int * buffer = image.buffer();

	for (int y = 0; y < image.height(); ++y, buffer += image.width())
	{
		int * buff = buffer;
		for (int x = 0; x < image.width(); ++x, buff++)
		{
			if ( *buff == -1 )
				continue;

			contours.push_back(Contour());

			writeCountour(image, x, y, contours.back());
			if ( contours.back().size() < minLength )
				contours.pop_back();
		}
	}

	std::sort(contours.begin(), contours.end(), CompareContours());
	if ( contours.size() > 20 )
	{
		contours.erase(contours.begin()+20, contours.end());
	}
}

void saveCountours(const TCHAR * fname, Contours & contours)
{
	FileWrapper ff(fname, _T("wt"));

	for (size_t i = 0; i < contours.size(); ++i)
	{
		Contour & contour = contours[i];

		_ftprintf(ff, _T("{\n"));
		for (size_t j = 0; j < contour.size(); ++j)
		{
			Vec2i v = contour[j];
			_ftprintf(ff, _T("{%d, %d}\n"), v.x(), v.y());
		}
		_ftprintf(ff, _T("}\n"));
	}
}
