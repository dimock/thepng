#pragma once

#include "image.h"
#include "helpers.h"


struct Feature
{
	Feature(int colorIndex = -1) : colorIndex_(colorIndex), radius_(0.0)
	{}

	void prepare();
	void simplify(size_t step);

	/// rough check. used only to find correlating fetaures
	double similarity(const Feature & other) const;

	bool operator < (const Feature & other) const;

	double radius_;
	Vec2d center_;
	int colorIndex_;
  Contour contour_, simple_;
};

typedef std::vector<Feature> Features;

struct Params
{
	Params() :
		deltaAngle(15.0),
		threshold(15.0),
		maxPaletteSize(16),
		minContourSize(100),
		featuresMax(20),
		similarMax(5),
		correlatedNum(1)
	{
	}

	double threshold, deltaAngle;

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
	void findCorrelatedFeatures(const Features & features, const Features & other, std::vector<Correlation> & correlations);

	bool findAlignment(const Features & features1, const Features & features2,
		const Vec2d & center1, const Vec2d & center2,
		const std::vector<Correlation> & correlations,
		Transformd & tr12,
    Vec2d & rotCenter);

public:

	void setParams(const Params & params)
	{
		params_ = params;
	}

	ImageAligner(std::vector<ImageUC> & images) : images_(images)
	{
	}

	/// do alignment with given params
	bool align();

private:

	std::vector<ImageUC> & images_;

	Params params_;
};


//////////////////////////////////////////////////////////////////////////

void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);
