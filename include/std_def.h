#ifndef __STD_DEFINE_H__
#define __STD_DEFINE_H__

#include <string>
#include <vector>
#include <list>

typedef std::basic_string<TCHAR> TString;
typedef std::basic_string<WCHAR> WString;
typedef std::basic_ostringstream<TCHAR> TOStringStream;
typedef std::list<TString> TStringList;
typedef std::vector<TString> TStringArray;
typedef std::vector<unsigned long> ULongArray;

#endif // __STD_DEFINE_H__