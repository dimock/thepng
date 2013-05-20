#include "helpers.h"
#include <algorithm>
#include <windows.h>

class char_replacer
{
	const TCHAR what_, by_;

public:

	char_replacer(const TCHAR & what, const TCHAR & by) :
			what_(what), by_(by)
	{}

	void operator () (TCHAR  & c) const
	{
		if ( c == what_ )
			c = by_;
	}
};

void replace_delimiter(std::tstring & pathname)
{
	std::for_each(pathname.begin(), pathname.end(), char_replacer(dir_delimiter1, dir_delimiter));
}


std::tstring extract_filename(const std::tstring & pathname)
{
	std::tstring filename = pathname;
	size_t dl_pos = filename.find_last_of(dir_delimiter);
	if ( dl_pos != std::tstring::npos )
		filename.erase(0, dl_pos+1);
	return filename;
}

std::tstring extract_dirname(const std::tstring & filename)
{
	std::tstring dirname = filename;
	size_t dl_pos = dirname.find_last_of(dir_delimiter);
	if ( dl_pos != std::tstring::npos )
	{
		dirname.erase(dl_pos+1);
		return dirname;
	}
	else
		return std::tstring();
}

TestTimer::TestTimer()
{
  LARGE_INTEGER qf, qc;
  QueryPerformanceFrequency(&qf);
  QueryPerformanceCounter(&qc);

  timeStart_ = qc.QuadPart;
  qpfreq_ = qf.QuadPart;
}

double TestTimer::getTimeMs() const
{
  LARGE_INTEGER qc;
  QueryPerformanceCounter(&qc);

  return ((double)(qc.QuadPart - timeStart_) / qpfreq_) * 1000.0;
}
