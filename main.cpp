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
	bool spaceIndents; // true = replace tab indents with spaces
	bool tabIndents; // true = replace space indents with tabs
	bool spaceToTabInLine; // false = do nothing inside line, true = spaces to tabs inside line
	bool tabToSpaceInLine; // false = do nothing inside line, true = tabs to spaces inside line
	bool commentSingleLine; // false = don't alter, true = change /* ... */ at end of single line to // ...
	unsigned short tabSizeBefore; // number of spaces in a tab
	unsigned short tabSizeAfter; // desired number of spaces in a tab
	bool carriageReturnAdd; // add CR if missing
	bool carriageReturnRemove; // remove CR is present
	bool rtw; // remove trailing whitespaces

	format_info()
		: spaceIndents(false),
		tabIndents(false),
		spaceToTabInLine(false),
		tabToSpaceInLine(false),
		commentSingleLine(false),
		tabSizeBefore(DEF_TAB_SIZE),
		tabSizeAfter(DEF_TAB_SIZE),
		carriageReturnAdd(false),
		carriageReturnRemove(false),
		rtw(true)
	{
	}

	format_info(bool _spaceIndents, bool _tabIndents, bool _tabInLine, bool _spaceInLine, bool _commentSingleLine, unsigned short _tabSizeBefore, unsigned short _tabSizeAfter, bool _carriageReturnAdd, bool _carriageReturnRemove, bool _rtw)
		: spaceIndents(_spaceIndents),
		tabIndents(_tabIndents),
		spaceToTabInLine(_tabInLine),
		tabToSpaceInLine(_spaceInLine),
		commentSingleLine(_commentSingleLine),
		tabSizeBefore(_tabSizeBefore),
		tabSizeAfter(_tabSizeAfter),
		carriageReturnAdd(_carriageReturnAdd),
		carriageReturnRemove(_carriageReturnRemove),
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
inline string rtrim(string& s);

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
		"    -h, --help                 Print this help" << endl <<
		"    -s, --spaceIndents         Convert tab indentations to spaces" << endl <<
		"    -t, --tabIndents           Convert space indentations to tabs" << endl <<
		"    -s2t, --spaceToTabInLine   Convert spaces to tabs inside of line." << endl <<
		"    -t2s, --tabToSpaceInLine   Convert tabs to spaces inside of line." << endl <<
		"    -c, --commentSingleLine    Convert /* ... */ at end of single line to // ..." << endl <<
		"    -szb n, --tabSizeBefore n  Tab size to convert from. Set to n (default: " << DEF_TAB_SIZE << ")" << endl <<
		"    -sza n, --tabSizeAfter n   Tab size to convert to. Set to n (default: " << DEF_TAB_SIZE << ")" << endl <<
		"    -ra, --CR-add              Add carriage return at end of line if missing (CRLF)." << endl <<
		"    -rr, --CR-remove           Remove carriage return at end of line if present." << endl <<
		"    -k, --keep-trailing        Keep trailing whitespaces" << endl;
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
		else if (arg == "-s" || arg == "--spaceIndents")
		{
			format.spaceIndents = true;
		}
		else if (arg == "-t" || arg == "--tabIndents")
		{
			format.tabIndents = true;
		}
		else if(arg == "-s2t" || arg == "--spaceToTabInLine")
		{
			format.spaceToTabInLine = true;
		}
		else if (arg == "-t2s" || arg == "--tabToSpaceInLine")
		{
			format.tabToSpaceInLine = true;
		}
		else if (arg == "-c" || arg == "--commentSingleLine")
		{
			format.commentSingleLine = true;
		}
		else if (arg == "-szb" || arg == "--tabSizeBefore")
		{
			if (i + 1 >= numargs)
			{
				cerr << "-szb option requires one argument (size)" << endl;
				return 1;
			}

			int itabsz = atoi(argv[++i]);
			format.tabSizeBefore = static_cast<unsigned short>(itabsz);
		}
		else if (arg == "-sza" || arg == "--tabSizeAfter")
		{
			if (i + 1 >= numargs)
			{
				cerr << "-sza option requires one argument (size)" << endl;
				return 1;
			}

			int itabsz = atoi(argv[++i]);
			format.tabSizeAfter = static_cast<unsigned short>(itabsz);
		}
		else if (arg == "-ra" || arg == "--CR-add")
		{
			format.carriageReturnAdd = true;
		}
		else if (arg == "-rr" || arg == "--CR-remove")
		{
			format.carriageReturnRemove = true;
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

	if (format.spaceIndents && format.tabIndents)
	{
		cerr << "-s and -t are mutually exclusive options!" << endl;
		return 1;
	}
	if (format.spaceToTabInLine && format.tabToSpaceInLine)
	{
		cerr << "-t2s and -s2t are mutually exclusive options!" << endl;
		return 1;
	}
	if (format.carriageReturnAdd && format.carriageReturnRemove)
	{
		cerr << "-ra and -rr are mutually exclusive options!" << endl;
		return 1;
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

int extended_isspace(char c)
{
	return std::isspace((unsigned char)c);
}

// Trims string from start
inline string& ltrim(string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(extended_isspace))));
	return s;
}

// Trims string at the end
// return the trimmed part
inline string rtrim(string& s)
{
	auto trimFrom = std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun(extended_isspace))).base();
	string trimmed = s.substr(trimFrom - s.begin());
	s.erase(trimFrom, s.end());
	return trimmed;
}

///////////////////////////////////////////////////////////////////////////

// Arguments:
//  str - portion of a line consisting of spaces and/or tabs
//  column - the column where str starts - return column where str ends
//  spacesToTabs - Convert spaces to tabs if true, tabs to spaces if false
inline string& whitespace_convert(string& str, size_t& column, bool spacesToTabs, unsigned short tabSizeBefore, unsigned short tabSizeAfter)
{
	unsigned short i;

	// Count spaces and tabs
	unsigned short tabWidth;
	size_t num_spaces = 0;
	size_t currentColumn = column;
	for (i = 0; i < str.length(); ++i)
	{
		const char& c = str[i];
		if (c == '\t')
		{
			tabWidth = tabSizeBefore - (currentColumn % tabSizeBefore);
			num_spaces += tabWidth;
			currentColumn += tabWidth;
		}
		else if (c == ' ')
		{
			++num_spaces;
			++currentColumn;
		}
		else
			throw runtime_error("not whitespace");
	}

	str.clear();

	if (spacesToTabs)
	{
		size_t spacesInFirstTab = tabSizeAfter - column % tabSizeAfter;
		if (num_spaces >= spacesInFirstTab)
		{
			str.append(1, '\t');
			num_spaces -= spacesInFirstTab;

			//now spaces and tabs are column aligned

			size_t num_conv_tabs = num_spaces / tabSizeAfter;
			str.append(num_conv_tabs, '\t');

			num_spaces -= num_conv_tabs * tabSizeAfter;
		}
	}

	str.append(num_spaces, ' ');
	column = currentColumn;

	return str;
}

inline string remove_trailing_whitespaces(string& line)
{
	return rtrim(line);
}

// Split str into sections of whitespace and non-whitespace
vector<string> explode_whitespace(string str)
{
	vector<string> exploded;

	if (str.length() == 0)
		return exploded;

	size_t pos;
	bool findWhitespaceSection = (str[0] != ' ' && str[0] != '\t');
	while (!str.empty())
	{
		if (findWhitespaceSection)
			pos = str.find_first_of(" \t");
		else
			pos = str.find_first_not_of(" \t");

		if (pos == string::npos)
		{
			exploded.push_back(str);
			return exploded;
		}
		else
		{
			exploded.push_back(str.substr(0, pos));
			str = str.substr(pos);
		}

		findWhitespaceSection = !findWhitespaceSection;
	}

	return exploded;
}

inline string& format_line(string& line, const format_info& format)
{
	if (line.empty())
		return line;

	if (format.commentSingleLine)
	{
		string trailing = remove_trailing_whitespaces(line);

		size_t startPos = line.rfind("/*");
		if (startPos != string::npos)
		{
			size_t endPos = line.find("*/", startPos+2);
			if (endPos != string::npos && endPos == line.length() - 2)
			{
				line[startPos + 1] = '/';
				line.erase(endPos, 2);
			}
		}

		line += trailing;
	}

	if (format.rtw)
	{
		bool carriage_return = (line.back() == '\r');
		remove_trailing_whitespaces(line);
		if ((!format.carriageReturnRemove && carriage_return) || format.carriageReturnAdd)
			line += '\r';

		if (line.length() == 0 || (line.length() == 1 && line.back() == '\r'))
			return line;
	}
	else if(format.carriageReturnAdd && line.back() != '\r')
	{
		line += '\r';
	}
	else if (format.carriageReturnRemove && line.back() == '\r')
	{
		line.erase(line.end() - 1);
	}

	if (format.spaceIndents || format.tabIndents || format.spaceToTabInLine || format.tabToSpaceInLine)
	{
		string indent;

		{
			size_t pos = line.find_first_not_of(" \t");

			if (pos == string::npos)
			{
				indent = line;
				line.clear();
			}
			else
			{
				indent = line.substr(0, pos);
				line = line.substr(pos);
			}
		}

		{
			size_t column = 0;

			if (format.spaceIndents || format.tabIndents)
				whitespace_convert(indent, column, format.tabIndents, format.tabSizeBefore, format.tabSizeAfter);
	
			if (format.spaceToTabInLine || format.tabToSpaceInLine)
			{
				vector<string> parts = explode_whitespace(line);

				line = indent;
				for (size_t i = 0; i < parts.size(); ++i)
				{
					if (parts[i][0] == ' ' || parts[i][0] == '\t')
						whitespace_convert(parts[i], column, format.spaceToTabInLine, format.tabSizeBefore, format.tabSizeAfter);
					else
						column += parts[i].length();

					line += parts[i];
				}
			}
			else
			{
				line = indent + line;
			}
		}
	}

	return line;
}

///////////////////////////////////////////////////////////////////////////

void format_file(const string& path, const format_info& format)
{
	ifstream instream;
	instream.open(path, ios_base::binary | ios_base::in);
	if (!instream.is_open())
	{
		cout << "Could not open " << path << " for reading." << endl;
		return;
	}

	// read and convert per line
	stringstream converted;
	string line;
	while (getline(instream, line, '\n'))
	{
		format_line(line, format);
		converted << line << '\n';
	}

	instream.close();

	// save
	ofstream outstream;
	string outpath(path);
	outstream.open(outpath, ios_base::binary | ios_base::out | ios_base::trunc);
	if (!outstream.is_open())
	{
		cout << "Could not open " << path << " for writing!" << endl;
		return;
	}

	outstream << converted.rdbuf();
	outstream.close();

	cout << "Fixed " << path << endl;
}

///////////////////////////////////////////////////////////////////////////

void format_path(string& ppath, const format_info& format)
{
	string abspath(ppath);
	abs_path(abspath);

#if _MSC_VER >= 1700
	DWORD attributes = GetFileAttributes(abspath.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		std::cout << "Invalid path!" << endl;
		return;
	}

	bool isdir = ((attributes & FILE_ATTRIBUTE_DIRECTORY) > 0U);
#else
	struct stat buf;
	int result;
	result = stat(abspath.c_str(), &buf);

	if (result != 0)
	{
		std::cout << "Invalid path!" << endl;
		return;
	}

	bool isdir = ((buf.st_mode & S_IFDIR) > 0U);
#endif

	if (isdir)
	{
		// For output, we want a safe (relative) path
		string path_safe(ppath);
		safe_dir_path(path_safe);

		// Directory traversal needs a safe absolute path
		string abspath_safe(abspath);
		safe_dir_path(abspath_safe);

		cout << "Trying to fix path " << abspath_safe << " ..." << endl;

#ifdef WIN32
		HANDLE hFind;
		WIN32_FIND_DATAA findData;

		string findfile = abspath_safe + "*";

		hFind = FindFirstFileA(findfile.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					continue;

				format_file(path_safe + findData.cFileName, format);
			} while (FindNextFileA(hFind, &findData));
			FindClose(hFind);
		}
#else
		DIR* dir;
		class dirent *ent;
		class stat st;

		dir = opendir(abspath_safe.c_str());
		while ((ent = readdir(dir)) != NULL)
		{
			const string fileName = ent->d_name;
			const string fullFileName = abspath_safe + fileName;

			if (fileName[0] == '.' && (fileName.length() == 1 || fileName.length() == 2 && fileName[1] == '.'))
				continue;

			if (stat(fullFileName.c_str(), &st) == -1)
				continue;

			if ((st.st_mode & S_IFDIR) != 0)
				continue;

			format_file(path_safe + fileName, format);
		}
#endif
	}
	else
	{
		format_file(ppath, format);
	}
}
