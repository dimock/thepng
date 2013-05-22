#pragma once

#include "image.h"
#include "vec2.h"
#include "helpers.h"

struct Correlation
{
  Correlation() : deltaDist_(-1.0), corrIndex1_(-1), corrIndex2_(-1), twoIndex_(-1) {}

  bool operator < (const Correlation & other) const
  {
    return deltaDist_ < other.deltaDist_;
  }

  double deltaDist_;
  int corrIndex1_, corrIndex2_, twoIndex_;
};

struct Feature
{
	Feature(int colorIndex = -1) : colorIndex_(colorIndex), radius_(0.0)
	{}

	void prepare();
	double similarity(const Feature & other) const;
  void findClosestFeatures(const std::vector<Feature> & otherFeatures, size_t maxNum);
	void simplify(size_t numPoints);
  bool operator < (const Feature & other) const;

	double radius_;
	Vec2d center_;
	int colorIndex_;
	Contour contour_;
  std::vector<std::pair<int, double>> closestFeatures_;
  std::vector<Correlation> correlatedFeatures_;
};

typedef std::vector<Feature> Features;

void findBoundaries(Image<int> & image);
void vectorize(Image<int> & image, Features & features, size_t minLength, size_t featuresMax);
void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);
void findCorrelatedFeatures(Features & features, Features & other, size_t correlatedNum);

void align(std::vector<ImageUC> & images, double threshold, size_t maxPaletteSize, size_t minContourSize, size_t featuresMax, size_t similarMax, size_t correlatedNum);

