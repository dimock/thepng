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