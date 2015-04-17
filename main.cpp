///////////////////////////////////////////////////////////////////////////
//
// identfix - Indentation fixer
// Copyright (c) 2015, prosenkranz
//
///////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
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

// Arguments:
//	line - the actual line string without terminating end line
//	spacesToTabs - Convert spaces to tabs if true, tabs to spaces if false
void convert_line(string& line, bool spacesToTabs)
{
	// \t   asdasd
	int spacecount = 0;
	int firstchar = 0;
	for (unsigned int i = 0; i < line.length(); ++i)
	{
		char c = line[i];
		if (c == '\t')
		{
			spacecount = 0;
			continue;
		}
		else if (c == ' ')
		{
			if (spacecount == 0)
				firstchar = i;

			spacecount++;
			if (spacecount == 4)
			{
				// replace 4 spaces with one tab
				line.erase(firstchar, 3);
				line[firstchar] = '\t';
				i = firstchar;
				spacecount = 0;
			}
		}
		else
		{
			// found any character, stop convertion
			break;
		}
	}
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
		convert_line(line, true);
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