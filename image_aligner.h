#pragma once

#include "image.h"
#include "vec2.h"
#include "helpers.h"

template <class C>
class Feature
{
public:

	Feature() : variation_(0) {}

  Feature(const Vec2i & tr, const C & color, double variation) :
    tr_(tr), color_(color), variation_(variation)
  {}

  const Vec2i & transform() const { return tr_; }
	double variation() const { return variation_; }
	const C & color() const { return color_; }

  double difference(const Feature<C> & other) const
  {
    double var_diff = abs(variation_ - other.variation_);
    Color3i delta_color = Color3i(color_) - Color3i(other.color_);
    double color_diff = sqrt((double)delta_color.r()*delta_color.r() + delta_color.g()*delta_color.g() + delta_color.b()*delta_color.b());
    return color_diff * var_diff;
  }

private:

  Vec2i tr_;
	double variation_;
	C color_;
};

void findBoundaries(Image<int> & image);
void vectorize(Image<int> & image, Contours & contours, size_t minLength);
void saveCountours(const TCHAR * fname, Contours & contours);


typedef Feature<Color3uc> FeatureUC;

typedef std::vector<FeatureUC> FeaturesUC;

/**
 * scale image proportionally in both directions
 * destination image pixels have average color of the corresponding source pixels
 * also save color variation along sub-image
 */

template <class C, class A>
bool scale_xy(int factor, const Image<C> & source_image, Image<Feature<C>> & features_image)
{
	int dst_width  = source_image.width() /factor;
	int dst_height = source_image.height()/factor;

	if ( dst_width < 1 || dst_height < 1 )
		return false;

	// allocate target buffer
	// pixels are unsigned, because we have to store sum of source pixels colors
	std::vector<A> sum_buffer(dst_width*dst_height);
  std::vector<A> sum2_buffer(dst_width*dst_height);

	const C * src_buffer = source_image.buffer();
	A * dst_buffer = &sum_buffer[0];
  A * dst2_buffer = &sum2_buffer[0];

	// sequentially scan source image row by row
	for (int y = 0, y_dst = 0, y_counter = 0; y < source_image.height(); ++y, y_counter++, src_buffer += source_image.width())
	{
		// we need to go to the next line of target image
		if ( y_counter >= factor )
		{
			y_counter = 0;
			if ( ++y_dst >= dst_height )
				break;

			dst_buffer += dst_width;
      dst2_buffer += dst_width;
		}

		const C * src_row = src_buffer;
		A * dst_row = dst_buffer;
    A * dst2_row = dst2_buffer;

		for (int x = 0, x_dst = 0, x_counter = 0; x < source_image.width(); ++x, x_counter++, src_row++)
		{
			// take the next taget pixel
			if ( x_counter >= factor )
			{
				x_counter = 0;
				if ( ++x_dst >= dst_width )
					break;

				dst_row++;
        dst2_row++;
			}

			*dst_row += *src_row;

      A src_color = *src_row;
      *dst2_row += src_color*src_color;
		}
	}

	// copy average values of the result to the target
	features_image.init(dst_width, dst_height);
	Feature<C> * target_buffer = features_image.buffer();
	int factor2 = factor*factor;

	for (size_t i = 0; i < sum_buffer.size(); ++i)
  {
    C color = sum_buffer[i]/factor2;
    A diff = sum2_buffer[i] - sum_buffer[i]*sum_buffer[i]/factor2;
    double var = ( sqrt((double)diff.r()) + sqrt((double)diff.g()) + sqrt((double)diff.b()) ) / factor2;

    int y = i / dst_width;
    int x = i - y*dst_width;

		target_buffer[i] = Feature<C>( Vec2i(x, y), color, var);
  }

	return true;
}


template <class C>
void featuresToImage(const Image<Feature<C>> & source, Image<C> & dest)
{
  dest.init(source.width(), source.height());

  size_t num = source.width() * source.height();

  for (size_t i = 0; i < num; ++i)
    dest[i] = source[i].color();
}

class ImageAligner
{
public:

  ImageAligner(ImagesUC & images, int scaleFactor, int featuresCount);

  double align(int index1, int index2);

private:

  // search for features in image with given index
  bool findFeatures(int index);
  bool collectFeatures(int index);

  std::vector<Image<FeatureUC>> scaled_;
  ImagesUC & images_;

  FeaturesUC features_;

  int scaleFactor_, featuresCount_;
};
