#include "image_aligner.h"
#include "png_imager.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <limits>
#include "kdtree.h"
#include "goldsect.h"

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

  for (size_t i = 0; i < simple_.size(); ++i)
  {
    Vec2d p = simple_[i] + tr;
		int itersN = 0;
		kdNode * node = kdtree.searchNN(p, itersN);
		if ( !node )
			continue;

    double dist = (node->p_ - p).length();
    diff += dist;
		++count;
  }
	
	if ( !count )
		return std::numeric_limits<double>::max();

  diff /= count;
  return diff;
}

void Feature::simplify(size_t step)
{
	for (size_t i = 0; i < contour_.size(); i += step)
    simple_.push_back(contour_[i]);
}

//////////////////////////////////////////////////////////////////////////

bool ImageAligner::align()
{
  std::vector<Color3uc> palette;
  std::vector<Image<int>> paletteBuffers(images_.size());

	imgTransforms_.resize(images_.size());
	features_arr_.resize(images_.size());

  for (size_t i = 0; i < images_.size(); ++i)
	{
		imgTransforms_[i].image_ = &images_[i];
    images_[i].make_palette(paletteBuffers[i], palette, params_.threshold, params_.maxPaletteSize);
	}

  normalize_palette(palette);

#ifdef SAVE_DEBUG_INFO_
  std::vector<ImageUC> paletteImages(images_.size());
#endif

  for (size_t i = 0; i < images_.size(); ++i)
  {
    findBoundaries(paletteBuffers[i]);

#ifdef SAVE_DEBUG_INFO_
    imageToPalette(paletteBuffers[i], paletteImages[i], palette);
    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\palette_%d.png"), i);
    PngImager::write(fname, paletteImages[i]);
#endif
  }

  for (size_t i = 0; i < paletteBuffers.size(); ++i)
  {
    vectorize(paletteBuffers[i], features_arr_[i]);

#ifdef SAVE_DEBUG_INFO_
    TCHAR fname[256];
    _stprintf(fname, _T("..\\..\\..\\data\\temp\\contours_%d.txt"), i);
    saveContours(fname, features_arr_[i]);
#endif
  }

  for (size_t i = 0; i < features_arr_.size(); ++i)
  {
    Features & features = features_arr_[i];
    for (size_t j = 0; j < features.size(); ++j)
      features[j].prepare();
  }

	findCorrelations();

	writeResult(_T("..\\..\\..\\data\\temp\\result.png"));

	return true;
}
//////////////////////////////////////////////////////////////////////////
bool ImageAligner::findCorrelations()
{
	if ( features_arr_.size() < 2 )
		return false;

	/// try to align each image except 0 to other image.
	/// 0 image is base
	for (size_t i = 1; i < features_arr_.size(); ++i)
	{
		if ( imgTransforms_[i].baseIndex_ >= 0 )
			continue;

		for (size_t j = 0; j < features_arr_.size(); ++j)
		{
			if ( i == j )
				continue;

			std::vector<Correlation> correlations;
			findCorrelatedFeatures(j, i, correlations);

			if ( correlations.empty() )
				continue;

			Transformd tr;

			/// find transform for i-th image relates to j
			if ( !findTransform(j, i, correlations, tr) )
				continue;

#ifdef SAVE_DEBUG_INFO_
			for (size_t m = 0; m < correlations.size(); ++m)
			{
				Features corrFeatures;
				Correlation & corr = correlations[m];
				corrFeatures.push_back(features_arr_[j][corr.index1_]);
				corrFeatures.push_back(features_arr_[i][corr.index2_]);

				for (size_t n = 0; n < corrFeatures[1].contour_.size(); ++n)
				{
					corrFeatures[1].contour_[n] = tr(corrFeatures[1].contour_[n]);
				}

				TCHAR fname[256];
				_stprintf(fname, _T("..\\..\\..\\data\\temp\\correlated_%d.txt"), m);
				saveContours(fname , corrFeatures );
			}
#endif

			imgTransforms_[i].baseIndex_ = j;
			imgTransforms_[i].tr_ = tr;
			break;
		}

		/// transform not found
		if ( imgTransforms_[i].baseIndex_ < 0 )
			return false;
	}

	/// do we have at least 1 image based on 0-th
	bool ok = false;
	for (size_t i = 0; i < imgTransforms_.size(); ++i)
	{
		if ( imgTransforms_[i].baseIndex_ == 0 )
		{
			ok = true;
			break;
		}
	}

	if ( !ok )
		return false;

	// find full transforms relates to base image
	for (size_t i = 0; i < imgTransforms_.size(); ++i)
	{
		Transformd tr = imgTransforms_[i].tr_;
		int baseIndex = imgTransforms_[i].baseIndex_;
		for ( ; baseIndex >= 0; )
		{
			tr = imgTransforms_[baseIndex].tr_ * tr;
			baseIndex = imgTransforms_[baseIndex].baseIndex_;
		}
		imgTransforms_[i].fullTr_ = tr;
	}


	return ok;
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

void ImageAligner::findCorrelatedFeatures(size_t index1, size_t index2, std::vector<Correlation> & correlations)
{
	const Features & features1 = features_arr_[index1];
	const Features & features2 = features_arr_[index2];

  for (size_t i = 0; i < features1.size(); i++)
  {
    const Feature & one = features1[i];
    for (size_t j = 0; j < features2.size(); ++j)
    {
      const Feature & two = features2[j];

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

		const Feature & one_i = features1[corr_i.index1_];
		const Feature & two_i = features2[corr_i.index2_];

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

			const Feature & one_j = features1[corr_j.index1_];
			const Feature & two_j = features2[corr_j.index2_];

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

	Vec2d dir1 = features1[corr0.index1_].center_ - features1[corr1.index1_].center_;
	Vec2d dir2 = features2[corr0.index2_].center_ - features2[corr1.index2_].center_;

	for (std::vector<Correlation>::iterator i = correlations.begin()+2; i != correlations.end(); )
	{
		Correlation & corr_i = *i;

		Vec2d dir1_i = features1[corr_i.index1_].center_ - features1[corr0.index1_].center_;
		Vec2d dir2_i = features2[corr_i.index2_].center_ - features2[corr0.index2_].center_;

		double z1 = dir1_i ^ dir1;
		double z2 = dir2_i ^ dir2;

		if ( z1*z2 >= 0 )
			++i;
		else
			i = correlations.erase(i);
	}
}

//////////////////////////////////////////////////////////////////////////

class CorrelationFunction
{
	Contour & contour1_;
	Contour & contour2_;
	Vec2d center2_;
	KdTree kdtree_;

public:

	CorrelationFunction(Contour & contour1, Contour & contour2, const Vec2d & center2) :
		contour1_(contour1), contour2_(contour2), center2_(center2)
	{
		kdtree_.build(contour1_);
	}

	double operator () (double * prm, int n) const
	{
		assert(n >= 3);
		
		Vec2d v(prm[0], prm[1]);
		double angle = prm[2];

		Transformd tr(angle, v+center2_);

		int itersN;
		double diff = 0;
		int count = 0;
		for (size_t i = 0; i < contour2_.size(); ++i)
		{
			Vec2d p = tr(contour2_[i]);
			kdNode * node = kdtree_.searchNN(p, itersN);
			if ( !node )
				continue;

			double d = (p - node->p_).length2();
			diff += d;
			count++;
		}

		assert( count > 0 );

		diff /= count;
		return diff;
	}
};

bool ImageAligner::findTransform(size_t index1, size_t index2,
																 const std::vector<Correlation> & correlations,
																 Transformd & tr12)
{
	Contour contour1, contour2;

	Vec2d startTr, rotCenter;
	double startAngle = 0;
	double dxy = images_[0].width()/4.0;

	const Features & features1 = features_arr_[index1];
	const Features & features2 = features_arr_[index2];

	if ( correlations.size() > 1 )
	{
		const Correlation & corr0 = correlations[0];
		const Correlation & corr1 = correlations[1];

		Vec2d dir1 = features1[corr0.index1_].center_ - features1[corr1.index1_].center_;
		Vec2d dir2 = features2[corr0.index2_].center_ - features2[corr1.index2_].center_;

		dir1.norm();
		dir2.norm();

		double sina = dir2 ^ dir1;
		startAngle = asin(sina);
	}

	for (size_t i = 0; i < correlations.size(); ++i)
	{
		const Correlation & corr = correlations[i];
		const Feature & f1 = features1[corr.index1_];
		const Feature & f2 = features2[corr.index2_];

		startTr += f1.center_ - f2.center_;

		if ( f1.radius_ < dxy )
			dxy = f1.radius_;

		Contour * contours [] = { &contour1, &contour2 };
		const Contour * fcontours[] = { &f1.contour_, &f2.contour_ };

		for (int j = 0; j < 2; ++j)
		{
			size_t start = contours[j]->size();
			contours[j]->resize(start+fcontours[j]->size());
			std::copy(fcontours[j]->begin(), fcontours[j]->end(), contours[j]->begin()+start);
		}

    rotCenter += f2.center_;
	}

	dxy *= 0.25;
	startTr *= 1.0/correlations.size();
  rotCenter *= 1.0/correlations.size();

	for (size_t i = 0; i < contour2.size(); ++i)
		contour2[i] -= rotCenter;

	double argsMin[] = { startTr.x() - dxy, startTr.y() - dxy, startAngle - toRad(params_.deltaAngle) };
	double argsMax[] = { startTr.x() + dxy, startTr.y() + dxy, startAngle + toRad(params_.deltaAngle) };
	double errs[] = { dxy*0.02, dxy*0.02, toRad(params_.deltaAngle)*0.02 };
	double args[] = { startTr.x(), startTr.y(), startAngle };

	CorrelationFunction cf(contour1, contour2, rotCenter);
	GoldSection<double, CorrelationFunction> gs(cf, argsMin, argsMax, 3);

	double diff0 = cf(args, 3);
	double diff = gs.calc(10, args, errs);

	tr12 = Transformd(args[2], Vec2d(args[0], args[1]), rotCenter);
	return true;
}

void ImageAligner::writeResult(const TCHAR * outname) const
{
	assert(imgTransforms_.size() > 0 && images_.size() == imgTransforms_.size());

	// find result image size
	double left =0;
	double bottom = 0;
	double right = images_[0].width();
	double top = images_[0].height();

	for (size_t i = 1; i < images_.size(); ++i)
	{
		Vec2d vec[4];

		vec[0] = Vec2d(0, 0);
		vec[1] = Vec2d(images_[i].width(), 0);
		vec[2] = Vec2d(0, images_[i].height());
		vec[3] = Vec2d(images_[i].width(), images_[i].height());

		for (int j = 0; j < 4; ++j)
		{
			Vec2d v = imgTransforms_[i].fullTr_(vec[j]);
			if ( v.x() < left )
				left = v.x();
			if ( v.x() > right )
				right = v.x();
			if ( v.y() < bottom )
				bottom = v.y();
			if ( v.y() > top )
				top = v.y();
		}
	}

	int width = right - left;
	int height = top - bottom;

	Vec2d offset(left < 0 ? -left : 0, bottom < 0 ? -bottom : 0);
	Transformd offTr(0, offset, Vec2d());

	ImageUC transformed(width, height);

	for (size_t i = 0; i < images_.size(); ++i)
	{
		Transformd tr = imgTransforms_[i].fullTr_;
		tr = tr * offTr;
		transform<Color3uc, Color3u>(tr, images_[i], transformed);
	}

	PngImager::write(outname, transformed);
}
