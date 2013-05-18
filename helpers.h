#pragma once

#include <tchar.h>
#include <iostream>
#include <locale.h>

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


const TCHAR dir_delimiter = L'/';
const TCHAR dir_delimiter1 = L'\\';
const TCHAR point = L'.';
const TCHAR asteric = L'*';

void replace_delimiter(std::tstring & pathname);
std::tstring extract_filename(const std::tstring & pathname);

class TestTimer
{
  long long timeStart_;
  long long qpfreq_;

public:

  TestTimer();

  double getTimeMs() const;
};