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

	bool operator < (const Feature & other) const
	{
		return contour_.size() > other.contour_.size();
	}

	void simplify(size_t numPoints);

	double radius_;
	Vec2d center_;
	int colorIndex_;
	Contour contour_;
};

typedef std::vector<Feature> Features;

double findClosestFeatures(const Features & features1, const Features & features2, size_t & i1, size_t & i2);
void findBoundaries(Image<int> & image);
void vectorize(Image<int> & image, Features & features, size_t minLength, size_t featuresMax);
void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);

