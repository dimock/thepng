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

struct Params
{
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

struct Correlation
{
	Correlation() : index1_(-1), index2_(-1),
		correlationCoefficient_(-1.0), ok_(false)
	{
	}

	Correlation(int i1, int i2, double coeff) : index1_(i1), index2_(i2),
		correlationCoefficient_(coeff), ok_(true)
	{
	}

	bool operator < (const Correlation & other) const
	{
		return this->correlationCoefficient_ < other.correlationCoefficient_;
	}

	bool ok_;
	int index1_, index2_;
	double correlationCoefficient_;
};

class ImageAligner
{
private:

	void findBoundaries(Image<int> & image);
	void buildCountour(Image<int> & image, int x, int y, Contour & contour);
	void vectorize(Image<int> & image, Features & features);
	void findCorrelatedFeatures(Features & features, Features & other, std::vector<Correlation> & correlations);

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
