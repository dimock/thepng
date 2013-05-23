#include "image_aligner.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <limits>
#include "kdtree.h"

//////////////////////////////////////////////////////////////////////////


void saveContours(const TCHAR * fname, Features & features)
{
	FileWrapper ff(fname, _T("wt"));
	if ( !ff )
		return;

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
	if ( !ff )
		return;

	_ftprintf(ff, _T("{\n"));
	for (size_t j = 0; j < contour.size(); ++j)
	{
		Vec2i v = contour[j];
		_ftprintf(ff, _T("  {%d, %d}\n"), v.x(), v.y());
	}
	_ftprintf(ff, _T("}\n"));
}

//////////////////////////////////////////////////////////////////////////

void Feature::prepare()
{
	if ( contour_.empty() )
		return;

  simplify(4);

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

/// rough check. used only to find correlating fetaures
double Feature::similarity(const Feature & other) const
{
	if ( simple_.size() == 0 || other.simple_.size() == 0 || colorIndex_ != other.colorIndex_ ||
       simple_.size() > other.simple_.size()*2 ||
       other.simple_.size() > simple_.size()*2 ||
       radius_ > other.radius_*2 ||
       other.radius_ > radius_*2 )
	{
		return -1.0;
	}

  double diff = 0;
  Vec2d tr = other.center_ - center_;

	KdTree kdtree;

	Contour contour = other.simple_;
	kdtree.build(contour);
	int count = 0;

  for (size_t i = 0; i < simple_.size(); ++i, ++count)
  {
    Vec2d p = simple_[i] + tr;
		int itersN = 0;
		kdNode * node = kdtree.searchNN(p, itersN);
		if ( !node )
			continue;

    double dist = (node->p_ - p).length();
    diff += dist;
  }
  diff /= simple_.size();
  return diff;
}

void Feature::simplify(size_t step)
{
	for (size_t i = 0; i < contour_.size(); i += step)
    simple_.push_back(contour_[i]);
}

//////////////////////////////////////////////////////////////////////////

void ImageAligner::align(std::vector<ImageUC> & images)
{
  std::vector<Color3uc> palette;
  std::vector<Image<int>> paletteBuffers(images.size());

  for (size_t i = 0; i < images.size(); ++i)
    images[i].make_palette(paletteBuffers[i], palette, params_.threshold, params_.maxPaletteSize);

  normalize_palette(palette);

  std::vector<ImageUC> paletteImages(images.size());
  for (size_t i = 0; i < images.size(); ++i)
  {
    findBoundaries(paletteBuffers[i]);
    imageToPalette(paletteBuffers[i], paletteImages[i], palette);

#ifdef SAVE_DEBUG_INFO_
    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\palette_%d.png"), i);
    PngImager::write(fname, paletteImages[i]);
#endif
  }

  std::vector<Features> features_arr(paletteBuffers.size());

  for (size_t i = 0; i < paletteBuffers.size(); ++i)
  {
    vectorize(paletteBuffers[i], features_arr[i]);

#ifdef SAVE_DEBUG_INFO_
    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\contours_%d.txt"), i);
    saveContours(fname, features_arr[i]);
#endif
  }

  for (size_t i = 0; i < features_arr.size(); ++i)
  {
    Features & features = features_arr[i];
    for (size_t j = 0; j < features.size(); ++j)
      features[j].prepare();
  }

	std::vector<Correlation> correlations;
  findCorrelatedFeatures(features_arr[0], features_arr[1], correlations);

#ifdef SAVE_DEBUG_INFO_
	for (size_t i = 0; i < correlations.size(); ++i)
	{
		Features corrFeatures;
		Correlation & corr = correlations[i];
		corrFeatures.push_back(features_arr[0][corr.index1_]);
		corrFeatures.push_back(features_arr[1][corr.index2_]);

		TCHAR fname[256];
		_stprintf(fname, _T("..\\..\\..\\data\\temp\\correlated_%d.txt"), i);
		saveContours(fname , corrFeatures );
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
void ImageAligner::findBoundaries(Image<int> & image)
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

void ImageAligner::buildCountour(Image<int> & image, int x, int y, Contour & contour)
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

void ImageAligner::vectorize(Image<int> & image, Features & features)
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

			buildCountour(image, x, y, features.back().contour_);

			if ( features.back().contour_.size() < params_.minContourSize )
				features.pop_back();
		}
	}

	std::sort(features.begin(), features.end());

	if ( features.size() > params_.featuresMax )
		features.erase(features.begin()+params_.featuresMax, features.end());
}

void ImageAligner::findCorrelatedFeatures(Features & features, Features & other, std::vector<Correlation> & correlations)
{
  for (size_t i = 0; i < features.size(); i++)
  {
    Feature & one = features[i];
    for (size_t j = 0; j < other.size(); ++j)
    {
      Feature & two = other[j];

      double s = one.similarity(two);
			if ( s < 0 )
				continue;

			correlations.push_back( Correlation(i, j, s) );
    }
  }

	/// sort features by correlation coefficient in ascending order
	std::sort(correlations.begin(), correlations.end());

	/// 1. remove invalid features by corresponding distances

	/// always mark j-feature to remove, because it has worse correlation than i-th
	for (size_t i = 0; i < correlations.size(); ++i)
	{
		Correlation & corr_i = correlations[i];
		if ( !corr_i.ok_ )
			continue;

		Feature & one_i = features[corr_i.index1_];
		Feature & two_i = other[corr_i.index2_];

		for (size_t j = i+1; j < correlations.size(); ++j)
		{
			Correlation & corr_j = correlations[j];
			if ( !corr_j.ok_ )
				continue;

			// can be used only once
			if ( corr_i.index1_ == corr_j.index1_ ||
					 corr_i.index2_ == corr_j.index2_ )
			{
				corr_j.ok_ = false;
				continue;
			}

			Feature & one_j = features[corr_j.index1_];
			Feature & two_j = other[corr_j.index2_];

			double dist_one = (one_i.center_ - one_j.center_).length();
			double dist_two = (two_i.center_ - two_j.center_).length();

			/// distances between corresponding features pairs are too different
			double d = abs(dist_one-dist_two)/(dist_one+dist_two);
			if ( d > 0.05 )
			{
				correlations[j].ok_ = false;
			}
		}
	}

	for (std::vector<Correlation>::iterator i = correlations.begin(); i != correlations.end(); )
	{
		if ( i->ok_ )
			i++;
		else
			i = correlations.erase(i);
	}

	if ( correlations.size() > params_.correlatedNum )
		correlations.erase( correlations.begin() + params_.correlatedNum, correlations.end() );


	/// 2. remove invalid features using directions

	if ( correlations.size() < 3 )
		return;

	Correlation & corr0 = correlations[0];
	Correlation & corr1 = correlations[1];

	Vec2d dir1 = features[corr0.index1_].center_ - features[corr1.index1_].center_;
	Vec2d dir2 = other[corr0.index2_].center_ - other[corr1.index2_].center_;

	for (std::vector<Correlation>::iterator i = correlations.begin()+2; i != correlations.end(); )
	{
		Correlation & corr_i = *i;

		Vec2d dir1_i = features[corr_i.index1_].center_ - features[corr0.index1_].center_;
		Vec2d dir2_i = other[corr_i.index2_].center_ - other[corr0.index2_].center_;

		double z1 = dir1_i ^ dir1;
		double z2 = dir2_i ^ dir2;

		if ( z1*z2 >= 0 )
			++i;
		else
			i = correlations.erase(i);
	}
}
