///////////////////////////////////////////////////////////////////////////
//
// identfix - Indentation fixer
// Copyright (c) 2015, prosenkranz
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
#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#include <Shlwapi.h>
#endif

using namespace std;

void print_help();
void fix_file(const string& path);
void fix_path(const char* path);
bool abs_path(string& path);
void safe_dir_path(string& path, char correctSlash = '/', char invalidSlash = '\\');

///////////////////////////////////////////////////////////////////////////

void main(int numargs, char* argv[])
{
	switch (numargs)
	{	
	case 2:
		fix_path(argv[1]);
		break;

	default:
		print_help();
		break;
	}
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
static inline string& ltrim(string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

///////////////////////////////////////////////////////////////////////////

// Arguments:
//	line - the actual line string without terminating end line
//	spacesToTabs - Convert spaces to tabs if true, tabs to spaces if false
static inline string& convert_line(string& line, bool spacesToTabs, unsigned short tabsz)
{
	unsigned short i;

	// Determine chars to convert
	char needlech = (spacesToTabs ? ' ' : '\t');
	char replacech = (spacesToTabs ? '\t' : ' ');

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
		unsigned short num_conv_tabs = (num_total_spaces - (num_remaining_spaces % tabsz)) / 4;
		for (i = 0; i < num_conv_tabs; ++i)
			line.insert(0, 1, '\t');

		num_remaining_spaces -= num_conv_tabs * 4;
		num_written_chars = num_conv_tabs;
	}
	
	for (i = 0; i < num_remaining_spaces; ++i)
		line.insert(num_written_chars, 1, ' ');

	return line;
}

///////////////////////////////////////////////////////////////////////////

void fix_file(const string& path)
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
		convert_line(line, true, 4);
		converted << line << endl;
	}

	instream.close();

	// safe
	ofstream outstream;
	string outpath(path);
	//outpath += ".out";
	outstream.open(outpath);
	if (!outstream.is_open())
	{
		cout << "Could not open " << path << " for writing!" << endl;
	}

	outstream << converted.rdbuf();
	outstream.close();
}

///////////////////////////////////////////////////////////////////////////

void fix_path(const char* ppath)
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
				fix_file(findData.cFileName);
			} while (FindNextFileA(hFind, &findData));
			FindClose(hFind);
		}
#else
		cout << "Fixing path not supported in linux yet..." << endl;
#endif
	}
	else
	{
		fix_file(path);
	}
}

///////////////////////////////////////////////////////////////////////////

void print_help()
{	
	cout << "Usage: identfix [path]" << endl;
	cout << "Make sure file is indented with tabs instead of 4 spaces." << endl;
	cout << endl;
	cout << "If path is a directory, all files in it get fixed (not recursively)." << endl;
}