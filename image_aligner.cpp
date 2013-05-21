#include "image_aligner.h"
#include "helpers.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>

ImageAligner::ImageAligner(ImagesUC & images, int scaleFactor, int featureSize, int featuresCount) :
  images_(images), scaleFactor_(scaleFactor), featureSize_(featureSize), featuresCount_(featuresCount)
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
    TCHAR fname[256];
    _stprintf(fname, _T("..//..//..//data//temp//feature_%02d.png"), i);

		features_[i].calcCharacter();
		Color3uc color = features_[i].average_color();

		std::tcout << _T("Feature(") << i << _T(") have variation ") << features_[i].variation() << _T(" and average color (") << 
			color.r() << _T(", ") << color.g() << _T(", ") << color.b() << _T(")\n");

    PngImager::write(fname, features_[i].image());
  }

  return true;
}

bool ImageAligner::collectFeatures(int index)
{
	srand(time(0));

  ImageUC & img = images_[index];

	int offset_x = 2;
	int offset_y = 2;

  int w = img.width() - offset_x*2 - featureSize_;
  int h = img.height() - offset_x*2 - featureSize_;

	for (int i = 0; i < featuresCount_; ++i)
	{
		int x = (((double)rand()) / RAND_MAX) * w;
		int y = (((double)rand()) / RAND_MAX) * h;

		features_.push_back(FeatureUC());
		features_.back().set_transform( Vec2i(x, y) );

		if ( !img.take_part(x, y, featureSize_, features_.back().image()) )
			break;
	}

	return !features_.empty();
}
