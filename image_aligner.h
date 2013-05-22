#pragma once

#include "image.h"
#include "vec2.h"
#include "helpers.h"

struct Feature
{
	Feature(int colorIndex = -1) : colorIndex_(colorIndex), radius_(0.0)
	{}

	void prepare();
	double similarity(const Feature & other) const;
	void simplify(size_t numPoints);
  bool operator < (const Feature & other) const;

	double radius_;
	Vec2d center_;
	int colorIndex_;
  Contour contour_;
};

typedef std::vector<Feature> Features;

void findBoundaries(Image<int> & image);
void vectorize(Image<int> & image, Features & features, size_t minLength, size_t featuresMax);
void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);
void findCorrelatedFeatures(Features & features, Features & other, size_t & index0, size_t & index1);

void align(std::vector<ImageUC> & images, double threshold, size_t maxPaletteSize, size_t minContourSize, size_t featuresMax, size_t similarMax, size_t correlatedNum);

