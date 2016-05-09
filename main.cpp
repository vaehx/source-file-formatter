///////////////////////////////////////////////////////////////////////////
//
// Source File Formatter
// Copyright (c) 2015-2016, prosenkranz
//
///////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#include <Shlwapi.h>
#endif

using namespace std;

#define DEF_TAB_SIZE 4

struct format_info
{
	bool spacesToTabs; // true = indent with tabs, false = indent with spaces
	unsigned short tabsz; // number of spaces in a tab
	bool rtw; // remove trailing whitespaces

	format_info()
		: spacesToTabs(true),
		tabsz(DEF_TAB_SIZE),
		rtw(false)
	{
	}

	format_info(bool _spacesToTabs, unsigned short _tabsz, bool _rtw)
		: spacesToTabs(_spacesToTabs),
		tabsz(_tabsz),
		rtw(_rtw)
	{
	}
};

void format_file(const string& path, const format_info& format);
void format_path(string& path, const format_info& format);
static inline string& format_line(string& line, const format_info& format);

bool abs_path(string& path);
void safe_dir_path(string& path, char correctSlash = '/', char invalidSlash = '\\');
inline string& ltrim(string& s);

///////////////////////////////////////////////////////////////////////////

void print_help()
{
	cout << "Source File Formatter" << endl;
	cout << "Usage: sff [options] <path>" << endl;
	cout << "Makes sure the file is indented with tabs instead of (def: " << DEF_TAB_SIZE << ") spaces." << endl;
	cout << endl;
	cout << "If path is a directory, all files in it get fixed (not recursively)." << endl;
	cout << "Options: " << endl <<
		"    -h, --help		Print this help" << endl <<
		"    -s, --spaces	Convert tabs to spaces instead spaces to tabs" << endl <<
		"    -sz n, --tabsz n	Set custom tab size to n (default: " << DEF_TAB_SIZE << ")" << endl;
}

///////////////////////////////////////////////////////////////////////////

int main(int numargs, char* argv[])
{
	if (numargs == 1)
	{
		print_help();
		return 1;
	}

	format_info format;
	string path;
	for (int i = 1; i < numargs; ++i)
	{	
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			return 1;
		}
		else if (arg == "-s" || arg == "--spaces")
		{
			format.spacesToTabs = false;
		}
		else if (arg == "-sz" || arg == "--tabsz")
		{
			if (i + 1 >= numargs)
			{
				cerr << "-sz option requires one argument (size)" << endl;
				return 1;
			}

			int itabsz = atoi(argv[++i]);
			format.tabsz = static_cast<unsigned short>(itabsz);
		}
		else
		{
			if (!arg.compare(0, 1, "-"))
			{
				cerr << arg << " is not a valid option!" << endl;
				return 1;
			}

			path = arg;
		}
	}

	if (path.length() == 0)
	{
		cerr << "No path given!" << endl;
		return 1;
	}

	format_path(path, format);
	return 0;
}

///////////////////////////////////////////////////////////////////////////

// Replaces all occurences of backslash with forward slash and make
// sure the path ends with a trailing slash.
// The function assumes that path is actually a directory.
void safe_dir_path(string& path, char correctSlash, char invalidSlash)
{
	if (!path.empty())
	{
		for (size_t i = 0, sz = path.size(); i < sz; ++i)
		{
			if (path[i] == invalidSlash)
				path[i] = correctSlash;
		}
	}

	if (path.empty() || path.back() != correctSlash)
		path += correctSlash;
}

///////////////////////////////////////////////////////////////////////////

// returns true if the path has been changed to be absolute
bool abs_path(string& path)
{
#ifdef WIN32
	if (PathIsRelativeA(path.c_str()))
	{
		char cCurrentPath[FILENAME_MAX];		
		if (!_getcwd(cCurrentPath, sizeof(cCurrentPath)))
			return false;

		// just to be sure...
		cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';

		string curpath(cCurrentPath);
		safe_dir_path(curpath);
		path = curpath + path;

		return true;
	}
#endif

	return false;
}

///////////////////////////////////////////////////////////////////////////

// Trims string from start
inline string& ltrim(string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

///////////////////////////////////////////////////////////////////////////

// Arguments:
//	line - the actual line string without terminating end line
//	spacesToTabs - Convert spaces to tabs if true, tabs to spaces if false
inline string& format_line(string& line, const format_info& format)
{
	unsigned short i;

	// Count spaces and tabs at line start
	unsigned short num_spaces = 0, num_tabs = 0;
	for (i = 0; i < line.length(); ++i)
	{
		char c = line[i];
		if (c == '\t')
			++num_tabs;
		else if (c == ' ')
			++num_spaces;
		else
			break;
	}

	// calculate total space count
	unsigned short num_total_spaces = num_spaces + num_tabs * format.tabsz;

	// Trim the line at beginning
	ltrim(line);

	// prepend new indent		
	unsigned short num_remaining_spaces = num_total_spaces;
	unsigned short num_written_chars = 0;
	if (format.spacesToTabs)
	{
		unsigned short num_conv_tabs = (num_total_spaces - (num_remaining_spaces % format.tabsz)) / format.tabsz;
		for (i = 0; i < num_conv_tabs; ++i)
			line.insert(0, 1, '\t');

		num_remaining_spaces -= num_conv_tabs * format.tabsz;
		num_written_chars = num_conv_tabs;
	}
	
	for (i = 0; i < num_remaining_spaces; ++i)
		line.insert(num_written_chars, 1, ' ');

	return line;
}

///////////////////////////////////////////////////////////////////////////

void format_file(const string& path, const format_info& format)
{		
	cout << "Fixing file " << path << "..." << endl;

	ifstream instream;	
	instream.open(path);
	if (!instream.is_open())
	{
		cout << "Could not open " << path << " for reading." << endl;
		return;
	}

	// read and convert per line	
	stringstream converted;
	string line;
	while (getline(instream, line))
	{
		format_line(line, format);
		converted << line << endl;
	}

	instream.close();

	// save
	ofstream outstream;
	string outpath(path);
	outstream.open(outpath);
	if (!outstream.is_open())
	{
		cout << "Could not open " << path << " for writing!" << endl;
		return;
	}

	outstream << converted.rdbuf();
	outstream.close();
}

///////////////////////////////////////////////////////////////////////////

void format_path(string& ppath, const format_info& format)
{
	string path(ppath);
	abs_path(path);

	cout << "Trying to fix path " << path << "..." << endl;

	struct stat buf;	
	int result;

	result = stat(path.c_str(), &buf);
	
	if (result != 0)
	{
		std::cout << "Invalid path!";
		return;
	}

	bool isdir = ((buf.st_mode & S_IFDIR) > 0U);
	if (isdir)
	{
		// Make a safe directory path
		string safepath(path);
		safe_dir_path(safepath);
#ifdef WIN32
		HANDLE hFind;
		WIN32_FIND_DATAA findData;

		string findfile = safepath;
		findfile += "*.*";

		hFind = FindFirstFileA(findfile.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{				
				format_file(findData.cFileName, format);
			} while (FindNextFileA(hFind, &findData));
			FindClose(hFind);
		}
#else
		cout << "Fixing path not supported in linux yet..." << endl;
#endif
	}
	else
	{
		format_file(path, format);
	}
}
