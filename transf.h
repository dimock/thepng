#pragma once

#include "vec2.h"

template <class T>
class Transform
{
	Vec2<T> rx_, ry_;
	Vec2<T> tr_;

public:
	Transform() : rx_(1, 0), ry_(0, 1)
	{	
	}

	Transform(const T & angle, const Vec2<T> & translate, const Vec2<T> & rotCenter = Vec2<T>())
	{
		T sina = sin(angle);
		T cosa = cos(angle);

		rx_ = Vec2<T>(cosa, -sina);
		ry_ = Vec2<T>(sina,  cosa);

		Vec2<T> tr = translate + rotCenter;
		tr_ = Vec2<T>(tr.x() - rx_*rotCenter, tr.y() - ry_*rotCenter);
	}

	Vec2<T> operator () (const Vec2<T> & v) const
	{
		Vec2<T> vec(rx_*v, ry_*v);
		vec += tr_;
		return vec;
	}

	Transform<T> operator * (const Transform<T> & other) const
	{
		Transform<T> res;
		res.rx_ = Vec2<T>(rx_.x()*other.rx_.x()+rx_.y()*other.ry_.x(), rx_.x()*other.rx_.y()+rx_.y()*other.ry_.y());
		res.ry_ = Vec2<T>(ry_.x()*other.rx_.x()+ry_.y()*other.ry_.x(), ry_.x()*other.rx_.y()+ry_.y()*other.ry_.y());
		res.tr_ = Vec2<T>(tr_.x()*other.rx_.x()+tr_.y()*other.ry_.x()+other.tr_.x(), tr_.x()*other.rx_.y()+tr_.y()*other.ry_.y()+other.tr_.y());
		return res;
	}

	Vec2<T> tr() const { return tr_; }
	Vec2<T> rx() const { return rx_; }
	Vec2<T> ry() const { return ry_; }

	// invert transform
	Transform<T> operator ~ () const
	{
		Transform<T> trI;
		trI.rx_ = Vec2<T>(rx_.x(), ry_.x());
		trI.ry_ = Vec2<T>(rx_.y(), ry_.y());
		trI.tr_ = Vec2<T>(-tr_*trI.rx(), -tr_*trI.ry());
		return trI;
	}
};

typedef Transform<double> Transformd;
