#pragma once

template <class V, class F>
class GoldSection
{
public:

	GoldSection(F & f, V * argsMin, V * argsMax, int argsN) :
	  func_(f), argsMin_(argsMin), argsMax_(argsMax), argsN_(argsN)
	{
	}


	V calc(int stepsN, V * arg, V * errs)
	{
		std::vector<V> difs(argsN_);
		std::vector<V> deltaArg(argsN_);

		for (int i = 0; i < argsN_; ++i)
		{
			difs[i] = argsMax_[i] - argsMin_[i];
			arg[i]  = (argsMin_[i] + argsMax_[i]) * (V)0.5;
			deltaArg[i] = (argsMax_[i] - argsMin_[i]) * (V)0.5;
		}

		for (int step = 0; step < stepsN; ++step)
		{
			for (int i = 0; i < argsN_; ++i)
			{
				V arg_i = arg[i];
				V min_i = arg_i - deltaArg[i];
				V max_i = arg_i + deltaArg[i];
				if ( min_i < argsMin_[i] )
					min_i = argsMin_[i];
				if ( max_i > argsMax_[i] )
					max_i = argsMax_[i];

				V x[4] = {min_i,
					min_i + (max_i - min_i) * gamma1_,
					min_i + (max_i - min_i) * gamma2_,
					max_i};

				arg[i] = x[0];
				V v0 = func_(arg, argsN_);

				arg[i] = x[1];
				V vm = func_(arg, argsN_);

				arg[i] = x[2];
				V vp = func_(arg, argsN_);

				arg[i] = x[3];
				V v1 = func_(arg, argsN_);

				for (; fabs(x[2] - x[1]) > errs[i];)
				{
					if ( vm < vp )
					{
						x[3] = x[2];
						x[2] = x[1];
						arg[i] = x[1] = x[0] + (x[3] - x[0]) * gamma1_;

						v1 = vp;
						vp = vm;
						vm = func_(arg, argsN_);
					}
					else
					{
						x[0] = x[1];
						x[1] = x[2];
						arg[i] = x[2] = x[0] + (x[3] - x[0]) * gamma2_;

						v0 = vm;
						vm = vp;
						vp = func_(arg, argsN_);
					}
				}

				arg[i] = (x[1] + x[2]) * (V)0.5;
				difs[i] = (V)fabs(arg[i] - arg_i);
				deltaArg[i] *= (V)0.5;
			}

			bool ok = true;
			for (int i = 0; i < argsN_; ++i)
			{
				if ( difs[i] > errs[i])
				{
					ok = false;
					break;
				}
			}

			if ( ok )
				break;
		}

		return func_(arg, argsN_);
	}

private:

	F & func_;
	V * argsMin_, * argsMax_;
	int argsN_;
	static V gamma1_, gamma2_;
};

template <class V, class F>
V GoldSection<V, F>::gamma1_ = static_cast<V>((sqrt(5.0)-1)/(sqrt(5.0)+1));

template <class V, class F>
V GoldSection<V, F>::gamma2_ = static_cast<V>(2.0/(sqrt(5.0)+1));
