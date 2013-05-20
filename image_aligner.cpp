#include "image_aligner.h"
#include "helpers.h"
#include "png_imager.h"
#include <algorithm>

ImageAligner::ImageAligner(Images & images, int scaleFactor, int featureSize, int featuresCount) :
  images_(images), scaleFactor_(scaleFactor), featureSize_(featureSize), featuresCount_(featuresCount)
{
  scaled_.resize(images_.size());
  for (size_t i = 0; i < images_.size(); ++i)
  {
    scaled_[i].init(images_[i].width(), images_[i].height(), images_[i].bytes_pp());

    if ( !images[i].scale_xy(scaleFactor_, scaled_[i]) )
    {
      std::tcout << _T("Can't scale image ") << std::endl;
      return;
    }
  }
}

bool ImageAligner::findFeatures(int index, int angle, int deltaAngle, double varThreshold)
{
  if ( !collectFeatures(index, varThreshold) )
    return false;

  if ( !rotateFeatures(index, angle, deltaAngle) )
    return false;

  for (int i = 0; i < features_.size(); ++i)
  {
    TCHAR fname[256];
    _stprintf(fname, _T("..//..//..//data//temp//feature_%02d.png"), i);

    PngImager::write(fname, features_[i].images_[0]);
  }

  return true;
}

bool ImageAligner::collectFeatures(int index, double varThreshold)
{
  Image & img_scaled = scaled_[index];

  int w = img_scaled.width();
  int h = img_scaled.height();

  int offset_x = featureSize_*0.22;
  int offset_y = featureSize_*0.22;

  int x0 = offset_x;
  int y0 = offset_y;
  int x1 = w - offset_x - featureSize_;
  int y1 = h - offset_y - featureSize_;
  int x = x0;
  int y = y0;

  int perimeter = 2 * ((w-offset_x-featureSize_) + (h-offset_y-featureSize_));
  int d = perimeter / featuresCount_;

  for (; x <= x1; x += d)
  {
    Image part;
    if ( !img_scaled.take_part(x, y, featureSize_, part) )
    {
      break;
    }

    double var = part.take_variation();
    if ( var < 0 )
      continue;

    features_.push_back(Feature());
    features_.back().tr_.push_back(Vec2(x, y));
    features_.back().variation_ = var;
  }
  if ( x > x1 )
  {
    y += (x - x1);
    x = x1;
  }
  for ( ; y <= y1; y += d)
  {
    Image part;
    if ( !img_scaled.take_part(x, y, featureSize_, part) )
    {
      break;
    }

    double var = part.take_variation();
    if ( var < 0 )
      continue;

    features_.push_back(Feature());
    features_.back().tr_.push_back(Vec2(x, y));
    features_.back().variation_ = var;
  }
  if ( y > y1 )
  {
    x -= (y - y1);
    y = y1;
  }
  for ( ; x >= x0; x -= d)
  {
    Image part;
    if ( !img_scaled.take_part(x, y, featureSize_, part) )
    {
      break;
    }

    double var = part.take_variation();
    if ( var < 0 )
      continue;

    features_.push_back(Feature());
    features_.back().tr_.push_back(Vec2(x, y));
    features_.back().variation_ = var;
  }
  if ( x < x0 )
  {
    y -= (x0 - x);
    x = x0;
  }
  for ( ; y >= y0; y -= d)
  {
    if ( features_.size() == featuresCount_ )
      break;

    Image part;
    if ( !img_scaled.take_part(x, y, featureSize_, part) )
    {
      break;
    }

    double var = part.take_variation();
    if ( var < 0 )
      continue;

    features_.push_back(Feature());
    features_.back().tr_.push_back(Vec2(x, y));
    features_.back().variation_ = var;
  }

  if ( features_.empty() )
    return false;

  //std::sort(features_.begin(), features_.end());

  //for (std::vector<Feature>::iterator i = features_.begin(); i != features_.end(); ++i)
  //{
  //  if ( i->variation_ < varThreshold )
  //  {
  //    features_.erase(i, features_.end());
  //    break;
  //  }
  //}

  return !features_.empty();
}

bool ImageAligner::rotateFeatures(int index, int angle, int deltaAngle)
{
  Image & img_scaled = scaled_[index];
  int num = (angle*2 / deltaAngle) + 1;

  for (std::vector<Feature>::iterator i = features_.begin(); i != features_.end(); ++i)
  {
    i->images_.resize(num);
    img_scaled.take_part(i->tr_[0].x(), i->tr_[0].y(), featureSize_, i->images_[0]);
  }

  return true;
}
