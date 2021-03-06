#pragma once

#include <tchar.h>
#include <string>
#include <iostream>
#include <locale.h>
#include <stdio.h>
#include "vec2.h"

namespace std
{

#ifdef _UNICODE
	typedef wstring tstring;
	#define tcout wcout
#else
	typedef string tstring;
	#define tcout cout
#endif

}


const TCHAR slash = L'/';
const TCHAR bk_slash = L'\\';
const TCHAR point = L'.';
const TCHAR asteric = L'*';

void replace_backslashes(std::tstring & pathname);
std::tstring extract_filename(const std::tstring & pathname);
std::tstring extract_dirname(const std::tstring & filename);

class TestTimer
{
  long long timeStart_;
  long long qpfreq_;

public:

  TestTimer();

  double getTimeMs() const;
};

class FileWrapper
{
	FILE * fp_;

public:

	FileWrapper(const TCHAR * filename, const TCHAR * mode) :	fp_(NULL)
	{
		_tfopen_s(&fp_, filename, mode);
	}

	operator FILE * ()
	{
		return fp_;
	}

	~FileWrapper()
	{
		if ( fp_ != NULL )
			fclose(fp_);
	}
};
