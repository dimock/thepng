#pragma once

#include "image.h"
#include "vec2.h"

class Feature
{
public:

  Feature() : variation_(-1.0) {}

  bool operator < (const Feature & other) const
  {
    return this->variation_ > other.variation_;
  }

  Images images_;
  std::vector<Vec2> tr_;
  double variation_;
};

class ImageAligner
{
public:

  ImageAligner(Images & images, int scaleFactor, int featureSize, int featuresCount);

  // search for features in image with given index
  // rotate them from -angle to +angle and save to array features_
  bool findFeatures(int index, int angle, int deltaAngle, double varThreshold);

private:

  bool collectFeatures(int index, double varThreshold);
  bool rotateFeatures(int index, int angle, int deltaAngle);

  Images scaled_;
  Images & images_;

  // current features for image with given index
  // each feature has featuresCount_ images rotated to some angle in range [-angle..+angle]
  std::vector<Feature> features_;

  int scaleFactor_, featureSize_, featuresCount_;
};
