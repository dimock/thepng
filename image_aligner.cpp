#include "image_aligner.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <limits>

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


double Feature::similarity(const Feature & other) const
{
	if ( contour_.size() == 0 || other.contour_.size() == 0 || colorIndex_ != other.colorIndex_ ||
       contour_.size() > other.contour_.size()*2 ||
       other.contour_.size() > contour_.size()*2 ||
       radius_ > other.radius_*2 ||
       other.radius_ > radius_*2 )
	{
		return -1.0;
	}

  double diff = 0;
  Vec2d tr = other.center_ - center_;
  for (size_t i = 0; i < contour_.size(); ++i)
  {
    Vec2d p = contour_[i] + tr;
    double dist = std::numeric_limits<double>::max();
    for (size_t j = 0; j < other.contour_.size(); ++j)
    {
      const Vec2d & q = other.contour_[j];
      double d = (p - q).length();
      if ( d < dist )
        dist = d;
    }
    diff += dist;
  }
  diff /= contour_.size();
  return diff;
}

void Feature::simplify(size_t step)
{
  Contour temp;
	for (size_t i = 0; i < contour_.size(); i += step)
    temp.push_back(contour_[i]);
  contour_.swap(temp);
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

  size_t index1, index2;
  findCorrelatedFeatures(features_arr[0], features_arr[1], index1, index2);

#ifdef SAVE_DEBUG_INFO_
  saveContour( _T("..\\..\\..\\data\\temp\\correlated1.txt"), features_arr[0][index1].contour_ );
  saveContour( _T("..\\..\\..\\data\\temp\\correlated2.txt"), features_arr[1][index2].contour_ );
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

void ImageAligner::findCorrelatedFeatures(Features & features, Features & other, size_t & index0, size_t & index1)
{
  double sim = std::numeric_limits<double>::max();

  for (size_t i = 0; i < features.size(); i++)
  {
    Feature & one = features[i];
    for (size_t j = 0; j < other.size(); ++j)
    {
      Feature & two = other[j];

      double s = one.similarity(two);
      if ( s > 0 && s < sim )
      {
        sim = s;
        index0 = i;
        index1 = j;
      }
    }
  }
}
