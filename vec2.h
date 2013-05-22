#pragma once

template <class T>
class Vec2
{
	T x_, y_;

public:

	Vec2<T>() : x_(0), y_(0) {}

	Vec2<T>( const T x_, const T y_ )
	{ this->x_ = x_; this->y_ = y_; }

	template <class U>
	Vec2<T>(const Vec2<U> & v)
	{
		this->x_ = v.x();
		this->y_ = v.y();
	}

	template <class U>
	Vec2<T> & operator = (const Vec2<U> & v)
	{
		this->x_ = v.x();
		this->y_ = v.y();

		return *this;
	}

	T x() const { return x_; }
	T y() const { return y_; }

	Vec2<T> operator + ( const Vec2<T> &v ) const { return Vec2<T>(x_+v.x_, y_+v.y_); }
	Vec2<T> operator - ( const Vec2<T> &v ) const { return Vec2<T>(x_-v.x_, y_-v.y_); }
	Vec2<T> operator * ( const T t  ) const { return Vec2<T>(x_*t, y_*t); }
	T operator * ( const Vec2<T> &v ) const { return x_*v.x_ + y_*v.y_; }

	Vec2<T> & operator += ( const Vec2<T> &v ) { x_ += v.x_; y_ += v.y_; return *this; }
	Vec2<T> & operator -= ( const Vec2<T> &v ) { x_ -= v.x_; y_ -= v.y_; return *this; }
	Vec2<T> & operator *= ( const T t  ) { x_ *= t; y_ *= t; return *this; }

	Vec2<T> operator - () const { return Vec2<T>(-x_, -y_); }

	T length () const { return sqrt(x_*x_ + y_*y_); }
	T length2() const { return x_*x_ + y_*y_; }
	Vec2<T> &  norm() { operator *= (1.0f/length()); return *this; }
	Vec2<T> getnorm() const { Vec2<T> vec(*this); vec.norm(); return vec;  }
	Vec2<T> & scale( Vec2<T> & v ) { x_ *= v.x_; y_ *= v.y_; return *this; }

	bool operator == (const Vec2<T> &v) const
	{ return ( x_ == v.x_ ) && ( y_ == v.y_ ); }

	bool operator != (const Vec2<T> &v) const
	{ return ( x_ != v.x_ ) ||( y_ != v.y_ ); }

};
typedef Vec2<int> Vec2i;
typedef Vec2<double> Vec2d;

typedef std::vector<Vec2d> Contour;
typedef std::vector<Contour> Contours;