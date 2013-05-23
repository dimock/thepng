#pragma once

#include "vec2.h"

struct kdNode
{
	kdNode(kdNode * parent = 0);
	virtual ~kdNode();

	double distPt(const Vec2d & p) const;

	kdNode * left_, * right_;
	kdNode * parent_;
	Vec2d p_;
	int k_;
};

class KdTree
{
	kdNode * root_;
	kdNode * buildBranch(Vec2d * from, Vec2d * to, kdNode * parent);
	kdNode * searchNN(kdNode * best, kdNode * current, const Vec2d & p, double & bestDist, int & steps) const;

public:

	KdTree();
	virtual ~KdTree();

	kdNode * getRoot();
	void clear();
	void build(Contour & contour);
	kdNode * searchNN(const Vec2d & p, int & iters) const;

};
