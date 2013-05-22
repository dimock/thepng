#include "image_aligner.h"
#include "helpers.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <limits>


void Feature::prepare()
{
	if ( contour_.empty() )
		return;

	center_ = contour_[0];
	for (size_t i = 1; i < contour_.size(); ++i)
		center_ += contour_[i];

	double n1 = 1.0/contour_.size();
	center_ *= n1;

	for (size_t i = 0; i < contour_.size(); ++i)
	{
		double r = (contour_[i] - center_).length();
		radius_ += r;
	}

	radius_ *= n1;
}

bool Feature::operator < (const Feature & other) const
{
  return contour_.size() > other.contour_.size();
}


double Feature::similarity(const Feature & other) const
{
	if ( contour_.size() == 0 || other.contour_.size() == 0 || colorIndex_ != other.colorIndex_ )
		return -1.0;

	double sim_size = abs((double)contour_.size() - other.contour_.size())/((double)contour_.size() + other.contour_.size());
	double sim_radius = abs(radius_ - other.radius_) / (radius_ + other.radius_);

	double sim = sim_size * sim_radius;
	return sim;
}

void Feature::simplify(size_t numPoints)
{
	//while ( contour_.size() > numPoints )
	//{
	//	for ()
	//}
}

class CompareSimilarity
{
public:

  bool operator () (const std::pair<int, double> & pr1, const std::pair<int, double> & pr2) const
  {
    return pr1.second < pr2.second;
  }
};

void Feature::findClosestFeatures(const Features & otherFeatures, size_t maxNum)
{
  if ( otherFeatures.empty() )
    return;

  closestFeatures_.clear();
	for (size_t i = 0; i < otherFeatures.size(); ++i)
	{
		const Feature & feature = otherFeatures[i];
		double s = similarity(feature);
    if ( s >= 0 )
      closestFeatures_.push_back( std::make_pair(i, s) );
	}

  std::sort(closestFeatures_.begin(), closestFeatures_.end(), CompareSimilarity());
  if ( closestFeatures_.size() > maxNum )
    closestFeatures_.erase(closestFeatures_.begin()+maxNum, closestFeatures_.end());
}

//////////////////////////////////////////////////////////////////////////
void findBoundaries(Image<int> & image)
{
	int * buff = image.buffer();
	int * prev = 0;
	for (int y = 0; y < image.height(); ++y, buff += image.width())
	{
		int * buf = buff;
		int * next = y < image.height()-1 ? buff + image.width() : 0;
		int * nxt = next;
		int * prv = prev;

		for (int x = 0; x < image.width(); ++x, ++buf, ++nxt, ++prv)
		{
			int c = *buf;
			if ( x > 0 && buf[-1] != -1 && c != buf[-1] )
				continue;
			if ( x < image.width()-1 && buf[1] != -1 && c != buf[1] )
				continue;
			if ( next )
			{
				if ( c != *nxt )
					continue;
				if ( x > 0 && c != nxt[-1] )
					continue;
				if ( x < image.width()-1 && c != nxt[1] )
					continue;
			}
			if ( prev )
			{
				if ( *prv != -1 && c != *prv )
					continue;
				if ( x > 0 && prv[-1] != -1 && c != prv[-1] )
					continue;
				if ( x < image.width()-1 && prv[1] != -1 && c != prv[1] )
					continue;
			}
			*buf = -1;
		}

		prev = buff;
	}
}

void writeCountour(Image<int> & image, int x, int y, Contour & contour)
{
	static const int offsets[8][2] = { {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1} };

	int * buffer = image.buffer();
	int n0 = x + image.width()*y;
	int colorIndex = buffer[n0];
	int i = 0;
	bool ok = true;

	for ( ; ok; )
	{
		contour.push_back( Vec2d(x, y) );
		ok = false;

		for (int j = 0; j < 8; ++j, ++i)
		{
			if ( i > 7 )
				i = 0;

			int x1 = x + offsets[i][0];
			int y1 = y + offsets[i][1];

			if ( x1 < 0 || x1 >= image.width() || y1 < 0 || y1 >= image.height() )
				continue;

			int n1 = x1 + y1*image.width();
			if ( n1 == n0 )
				break;

			if ( buffer[n1] == -1 || buffer[n1] != colorIndex )
				continue;

			x = x1;
			y = y1;
			i = (i + 4) % 8;
			i++;
			if ( i > 7 )
				i = 0;

			ok = true;
			break;
		}
	}

	for (size_t i = 0; i < contour.size(); ++i)
	{
		Vec2i v = contour[i];
		int index = image.width()*v.y() + v.x();
		buffer[index] = -1;
	}
}

class CompareContours
{
public:

	bool operator () (const Contour & c1, const Contour & c2) const
	{
		return c1.size() > c2.size();
	}
};

void vectorize(Image<int> & image, Features & features, size_t minLength, size_t featuresMax)
{
	int * buffer = image.buffer();

	for (int y = 0; y < image.height(); ++y, buffer += image.width())
	{
		int * buff = buffer;
		for (int x = 0; x < image.width(); ++x, buff++)
		{
			if ( *buff == -1 )
				continue;

			features.push_back(Feature(*buff));

			writeCountour(image, x, y, features.back().contour_);
			if ( features.back().contour_.size() < minLength )
				features.pop_back();
		}
	}

	std::sort(features.begin(), features.end());
	if ( features.size() > featuresMax )
		features.erase(features.begin()+featuresMax, features.end());
}

void saveContours(const TCHAR * fname, Features & features)
{
	FileWrapper ff(fname, _T("wt"));

	for (size_t i = 0; i < features.size(); ++i)
	{
		Contour & contour = features[i].contour_;

		_ftprintf(ff, _T("{\n"));
		for (size_t j = 0; j < contour.size(); ++j)
		{
			Vec2i v = contour[j];
			_ftprintf(ff, _T("  {%d, %d}\n"), v.x(), v.y());
		}
		_ftprintf(ff, _T("}\n"));
	}
}

void saveContour(const TCHAR * fname, Contour & contour)
{
	FileWrapper ff(fname, _T("wt"));

	_ftprintf(ff, _T("{\n"));
	for (size_t j = 0; j < contour.size(); ++j)
	{
		Vec2i v = contour[j];
		_ftprintf(ff, _T("  {%d, %d}\n"), v.x(), v.y());
	}
	_ftprintf(ff, _T("}\n"));
}

//////////////////////////////////////////////////////////////////////////
void align(std::vector<ImageUC> & images, double threshold, size_t maxPaletteSize, size_t minContourSize, size_t featuresMax, size_t similarMax, size_t correlatedNum)
{
  std::vector<Color3uc> palette;
  std::vector<Image<int>> paletteBuffers(images.size());

  for (size_t i = 0; i < images.size(); ++i)
    images[i].make_palette(paletteBuffers[i], palette, threshold, maxPaletteSize);

  normalize_palette(palette);

  std::vector<ImageUC> paletteImages(images.size());
  for (size_t i = 0; i < images.size(); ++i)
  {
    findBoundaries(paletteBuffers[i]);
    imageToPalette(paletteBuffers[i], paletteImages[i], palette);

    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\palette_%d.png"), i);
    PngImager::write(fname, paletteImages[i]);
  }

  std::vector<Features> features_arr(paletteBuffers.size());

  for (size_t i = 0; i < paletteBuffers.size(); ++i)
  {
    vectorize(paletteBuffers[i], features_arr[i], minContourSize, featuresMax);

    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\contours_%d.txt"), i);
    saveContours(fname, features_arr[i]);
  }

  for (size_t i = 0; i < features_arr.size(); ++i)
  {
    Features & features = features_arr[i];
    for (size_t j = 0; j < features.size(); ++j)
      features[j].prepare();
  }

  Features & features = features_arr[0];
  for (size_t j = 0; j < features.size(); ++j)
    features[j].findClosestFeatures(features_arr[1], similarMax);

  findCorrelatedFeatures(features_arr[0], features_arr[1], correlatedNum);

  int bestCorrelated = -1;
  double corrDist = std::numeric_limits<double>::max();

  for (size_t i = 0; i < features_arr[0].size(); ++i)
  {
    Feature & feature = features_arr[0][i];
    if ( feature.correlatedFeatures_.empty() )
      continue;

    Correlation & corr = *feature.correlatedFeatures_.begin();
    Feature & two = features_arr[0][corr.twoIndex_];
    Feature & other1 = features_arr[1][corr.corrIndex1_];
    Feature & other2 = features_arr[1][corr.corrIndex2_];

    double s1 = feature.similarity(other1);
    double s2 = two.similarity(other2);

    double cdist = s1 * s2 * corr.deltaDist_;
    if ( cdist < corrDist )
    {
      corrDist = cdist;
      bestCorrelated = i;
    }
  }

  if ( bestCorrelated >= 0 )
  {
    Feature & best = features_arr[0][bestCorrelated];
    Correlation & corr = *best.correlatedFeatures_.begin();
    Feature & two = features_arr[0][corr.twoIndex_];
    Feature & other1 = features_arr[1][corr.corrIndex1_];
    Feature & other2 = features_arr[1][corr.corrIndex2_];

    std::vector<Feature> bestFeatures, corrFeatures;
    bestFeatures.push_back(best);
    bestFeatures.push_back(two);
    corrFeatures.push_back(other1);
    corrFeatures.push_back(other2);
    saveContours( _T("..\\..\\..\\data\\temp\\correlated1.txt"), bestFeatures );
    saveContours( _T("..\\..\\..\\data\\temp\\correlated2.txt"), corrFeatures );
  }
}

//////////////////////////////////////////////////////////////////////////
void findCorrelatedFeatures(Features & features, Features & other, size_t correlatedNum)
{
  for (size_t i = 0; i < features.size(); i++)
  {
    Feature & one = features[i];
    for (size_t j = i+1; j < features.size(); ++j)
    {
      Feature & two = features[j];
      double dist = (one.center_ - two.center_).length();
      for (size_t k = 0; k < one.closestFeatures_.size(); ++k)
      {
        int corrIndex1 = one.closestFeatures_[k].first;
        Feature & corr1 = other[corrIndex1];

        for (size_t l = 0; l < two.closestFeatures_.size(); ++l)
        {
          int corrIndex2 = two.closestFeatures_[l].first;
          if ( corrIndex1 == corrIndex2 )
            continue;

          Feature & corr2 = other[corrIndex2];

          double d = (corr1.center_ - corr2.center_).length();
          double delta = abs(dist - d) / (dist + d);

          Correlation correlation;
          correlation.deltaDist_ = delta;
          correlation.corrIndex1_ = corrIndex1;
          correlation.corrIndex2_ = corrIndex2;
          correlation.twoIndex_ = j;

          one.correlatedFeatures_.push_back(correlation);
        }
      }

      std::sort(one.correlatedFeatures_.begin(), one.correlatedFeatures_.end());
      if ( one.correlatedFeatures_.size() > correlatedNum )
        one.correlatedFeatures_.erase(one.correlatedFeatures_.begin() + correlatedNum, one.correlatedFeatures_.end());
    }
  }
}