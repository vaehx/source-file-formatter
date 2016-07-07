# Source-File-Formatter
Command-line sourcefile formatting tool.
This tool adds a new empty line at end of file by default, if not there yet.
Also removes trailing whitespaces by default.

* Replace space indent with tab indent and tabs in line to spaces: `sff yourfile.txt`
* Replace tab indent with space indent: `sff -s yourfile.txt`
* Replace spaces inside line with tabs: `sff -s2t yourfile.txt`
* Replace tabs inside line with spaces: `sff -t2s yourfile.txt`
* Specify size of tab in spaces before convert (default 4): `sff -szb 8 yourfile.txt`
* Specify size of tab in spaces after convert (default 4): `sff -sza 8 yourfile.txt`
* Disable automatic removal of trailing whitespaces: `sff -k yourfile.txt`
