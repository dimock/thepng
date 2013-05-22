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

double findClosestFeatures(const Features & features1, const Features & features2, size_t & i1, size_t & i2)
{
	i1 = 0;
	i2 = 0;

	double sim = std::numeric_limits<double>::max();

	for (size_t i = 0; i < features1.size(); ++i)
	{
		const Feature & f1 = features1[i];
		for (size_t j = 0; j < features2.size(); ++j)
		{
			const Feature & f2 = features2[j];
			double s = f1.similarity(f2);
			if ( s < sim )
			{
				sim = s;
				i1 = i;
				i2 = j;
			}
		}
	}

	return sim;
}

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