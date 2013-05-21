#pragma once

#include "image.h"
#include "vec2.h"

template <class C>
class Feature
{
public:

	Feature() : variation_(0) {}

	const Image<C> & image() const { return image_; }
	Image<C> & image() { return image_; }

	const Vec2i & transform() const { return tr_; }
	void set_transform(const Vec2i & tr) { tr_ = tr; }

	void calcCharacter()
	{
		variation_ = calc_variation<Color3uc, Color3<unsigned long long>>(image_, average_color_);
	}

	double variation() const { return variation_; }
	const C & average_color() const { return average_color_; }

private:

	Image<C> image_;
  Vec2i tr_;
	double variation_;
	C average_color_;
};

typedef Feature<Color3uc> FeatureUC;

class ImageAligner
{
public:

  ImageAligner(ImagesUC & images, int scaleFactor, int featureSize, int featuresCount);

  // search for features in image with given index
  bool findFeatures(int index);

private:

  bool collectFeatures(int index);

  ImagesUC scaled_;
  ImagesUC & images_;

  // current features for image with given index
  // each feature has featuresCount_ images rotated to some angle in range [-angle..+angle]
  std::vector<FeatureUC> features_;

  int scaleFactor_, featureSize_, featuresCount_;
};
