#pragma once

#include "image.h"
#include "helpers.h"


typedef std::vector<Vec2d> Contour;
typedef std::vector<Contour> Contours;

struct Feature
{
	Feature(int colorIndex = -1) : colorIndex_(colorIndex), radius_(0.0)
	{}

	void prepare();
	void simplify(size_t step);
	double similarity(const Feature & other) const;
  bool operator < (const Feature & other) const;

	double radius_;
	Vec2d center_;
	int colorIndex_;
  Contour contour_;
};

typedef std::vector<Feature> Features;

class Params
{
public:

	Params() :
		threshold(15.0),
		maxPaletteSize(16),
		minContourSize(100),
		featuresMax(20),
		similarMax(5),
		correlatedNum(5)
	{
	}

	double threshold;

	size_t maxPaletteSize,
				 minContourSize,
				 featuresMax,
				 similarMax,
				 correlatedNum;

};

class ImageAligner
{
private:

	void findBoundaries(Image<int> & image);
	void buildCountour(Image<int> & image, int x, int y, Contour & contour);
	void vectorize(Image<int> & image, Features & features);
	void findCorrelatedFeatures(Features & features, Features & other, size_t & index0, size_t & index1);

public:

	void setParams(const Params & params)
	{
		params_ = params;
	}

	/// do alignment with given params
	void align(std::vector<ImageUC> & images);

private:

	Params params_;
};


//////////////////////////////////////////////////////////////////////////

void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);
