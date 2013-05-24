#pragma once

#include "image.h"
#include "helpers.h"
#include "transf.h"

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
		deltaAngle(8.0),
		colorThreshold(15.0),
		correlationThreshold(10.0),
		maxPaletteSize(16),
		minContourSize(200),
		featuresMax(50),
		correlatedNum(2)
	{
	}

	double colorThreshold, deltaAngle, correlationThreshold;

	size_t maxPaletteSize,
				 minContourSize,
				 featuresMax,
				 correlatedNum;

	std::tstring outname_;
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

struct ImageTransform
{
	ImageTransform() : image_(0), baseIndex_(-1) {}

	const ImageUC * image_;

	/// transform is applied to this image relates to images_[baseIndex_];
	Transformd tr_, fullTr_;
	int baseIndex_;
};

class ImageAligner
{
private:

	void findBoundaries(Image<int> & image);
	void buildCountour(Image<int> & image, int x, int y, Contour & contour);
	void vectorize(Image<int> & image, Features & features);

	bool findCorrelations();
	void findCorrelatedFeatures(size_t index1, size_t index2, std::vector<Correlation> & correlations);
	bool findTransform(size_t index1, size_t index2, const std::vector<Correlation> & correlations, Transformd & tr12, double & diff);

	void writeResult() const;

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
	std::vector<Features> features_arr_;
	std::vector<ImageTransform> imgTransforms_;

	Params params_;
};


//////////////////////////////////////////////////////////////////////////

void saveContours(const TCHAR * fname, Features & features);
void saveContour(const TCHAR * fname, Contour & contour);
