#include "kdtree.h"
#include <algorithm>
#include <limits>

using namespace std;

kdNode::kdNode(kdNode * parent) :
	parent_(parent), left_(0), right_(0), k_(0)
{
}

kdNode::~kdNode()
{
	delete left_;
	delete right_;
}

double kdNode::distPt(const Vec2d & p) const
{
	return (p_ - p).length2();
}

KdTree::KdTree() :
	root_(0)
{
}

KdTree::~KdTree()
{
	clear();
}

class kdSortCriteria
{
	int k_;

public:
	kdSortCriteria(int k) :
		k_(k)
	{
	}

	bool operator () (const Vec2d & p0, const Vec2d & p1) const
	{
		if ( k_ == 0 )
			return p0.x() < p1.x();
		else
			return p0.y() < p1.y();
	}
};

void KdTree::clear()
{
	delete root_;
	root_ = 0;
}

kdNode * KdTree::getRoot()
{
	return root_;
}

kdNode * KdTree::buildBranch(Vec2d * from, Vec2d * to, kdNode * parent)
{
	size_t n = to-from;
	if ( n == 0 )
		return 0;

	size_t m = n/2;

	int k = 0;
	if ( parent )
		k = (parent->k_ + 1) & 1;

	nth_element(from, from+m, to, kdSortCriteria(k));

	Vec2d & p = from[m];
	kdNode * node = new kdNode(parent);
	node->k_ = k;
	node->p_ = p;
	node->left_  = buildBranch(from, from+m, node);
	node->right_ = buildBranch(from+m+1, to, node);

	return node;
}

void KdTree::build(Contour & contour)
{
	clear();
	Vec2d * first = &contour[0];
	Vec2d * last  = first + contour.size();
	root_ = buildBranch(first, last, 0);
}

kdNode * KdTree::searchNN(kdNode * best, kdNode * current, const Vec2d & p, double & bestDist, int & iters) const
{
	if ( !current )
		return best;

	if ( !best )
	{
		best = current;
		bestDist = best->distPt(p);
		iters++;
	}

	if ( best != current )
	{
		double dist = current->distPt(p);
		iters++;
		if ( dist < bestDist )
		{
			best = current;
			bestDist = dist;
		}
	}

	kdNode * ne = 0, * fa = 0;
	double distAxis = std::numeric_limits<double>::max();
	if ( current->k_ == 0 )
	{
		distAxis = p.x() - current->p_.x();
		if ( p.x() < current->p_.x() )
		{
			ne = current->left_;
			fa = current->right_;
		}
		else
		{
			ne = current->right_;
			fa = current->left_;
		}
	}
	else
	{
		distAxis = p.y() - current->p_.y();
		if ( p.y() < current->p_.y() )
		{
			ne = current->left_;
			fa = current->right_;
		}
		else
		{
			ne = current->right_;
			fa = current->left_;
		}
	}

	best = searchNN(best, ne, p, bestDist, iters);
	if ( !fa )
		return best;

	distAxis *= distAxis;
	if ( distAxis < bestDist )
	{
		best = searchNN(best, fa, p, bestDist, iters);
	}

	return best;
}


kdNode * KdTree::searchNN(const Vec2d & p, int & steps) const
{
	double dist = std::numeric_limits<double>::max();
	steps = 0;
	return searchNN(0, root_, p, dist, steps);
}
