/*
"Source File Formatter" Source File
Copyright (C) 2015-2016  Pascal Rosenkranz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
#else
#include <unistd.h>
#include <dirent.h>
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
		rtw(true)
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
inline string& rtrim(string& s);

///////////////////////////////////////////////////////////////////////////

void print_help()
{
	cout << "Source File Formatter" << endl;
	cout << "Usage: sff [options] <path>" << endl;
	cout << "Makes sure the file is indented with tabs instead of (def: " << DEF_TAB_SIZE << ") spaces." << endl;
	cout << "Also removes trailing whitespaces by default. To disable, add -k flag." << endl;
	cout << endl;
	cout << "If path is a directory, all files in it get fixed (not recursively)." << endl;
	cout << "Options: " << endl <<
		"    -h, --help		Print this help" << endl <<
		"    -s, --spaces	Convert tabs to spaces instead spaces to tabs" << endl <<
		"    -sz n, --tabsz n	Set custom tab size to n (default: " << DEF_TAB_SIZE << ")" << endl <<
		"    -k, --keep-trailing	Keep trailing whitespaces" << endl;
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
		else if (arg == "-k" || arg == "--keep-trailing")
		{
			format.rtw = false;
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
#else
	if (path.empty() || path[0] == '/')
#endif
	{
		char cCurrentPath[FILENAME_MAX];
#ifdef WIN32
		if (!_getcwd(cCurrentPath, sizeof(cCurrentPath)))
#else
		if (!getcwd(cCurrentPath, sizeof(cCurrentPath)))
#endif
			return false;

		// just to be sure...
		cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';

		string curpath(cCurrentPath);

#ifdef _USING_V110_SDK71_
		safe_dir_path(curpath, '\\', '/');
#else
		safe_dir_path(curpath);
#endif
		path = curpath + path;

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////

// Trims string from start
inline string& ltrim(string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// Trims string at the end
inline string& rtrim(string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

///////////////////////////////////////////////////////////////////////////

// Arguments:
//	line - the actual line string without terminating end line
//	spacesToTabs - Convert spaces to tabs if true, tabs to spaces if false
inline string& fix_indent(string& line, bool spacesToTabs, unsigned short tabsz)
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
	unsigned short num_total_spaces = num_spaces + num_tabs * tabsz;

	// Trim the line at beginning
	ltrim(line);

	// prepend new indent
	unsigned short num_remaining_spaces = num_total_spaces;
	unsigned short num_written_chars = 0;
	if (spacesToTabs)
	{
		unsigned short num_conv_tabs = (num_total_spaces - (num_remaining_spaces % tabsz)) / tabsz;
		for (i = 0; i < num_conv_tabs; ++i)
			line.insert(0, 1, '\t');

		num_remaining_spaces -= num_conv_tabs * tabsz;
		num_written_chars = num_conv_tabs;
	}

	for (i = 0; i < num_remaining_spaces; ++i)
		line.insert(num_written_chars, 1, ' ');

	return line;
}

inline string& remove_trailing_whitespaces(string& line)
{
	return rtrim(line);
}

inline string& format_line(string& line, const format_info& format)
{
	fix_indent(line, format.spacesToTabs, format.tabsz);
	if (format.rtw)
		remove_trailing_whitespaces(line);

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

#ifdef _USING_V110_SDK71_
	DWORD attributes = GetFileAttributes(path.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		std::cout << "Invalid path!" << endl;
		return;
	}

	bool isdir = ((attributes & FILE_ATTRIBUTE_DIRECTORY) > 0U);
#else
	struct stat buf;
	int result;
	result = stat(path.c_str(), &buf);

	if (result != 0)
	{
		std::cout << "Invalid path!";
		return;
	}

	bool isdir = ((buf.st_mode & S_IFDIR) > 0U);
#endif

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
		DIR* dir;
		class dirent *ent;
		class stat st;

		dir = opendir(safepath.c_str());
		while ((ent = readdir(dir)) != NULL)
		{
			const string fileName = ent->d_name;
			const string fullFileName = safepath + fileName;

			if (fileName[0] == '.' && (fileName.length() == 1 || fileName.length() == 2 && fileName[1] == '.'))
				continue;

			if (stat(fullFileName.c_str(), &st) == -1)
				continue;

			if ((st.st_mode & S_IFDIR) != 0)
				continue;

			format_file(fileName, format);
		}
#endif
	}
	else
	{
		format_file(path, format);
	}
}
