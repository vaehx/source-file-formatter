# Source-File-Formatter
Command-line sourcefile formatting tool.
This tool adds a new empty line at end of file by default, if not there yet.
Also removes trailing whitespaces by default.

* Replace space indent with tab indent: `sff yourfile.txt`
* Replace tab indent with space indent: `sff -s yourfile.txt`
* Specify size of tab in spaces (default 4): `sff -sz 8 yourfile.txt`
* Disable automatic removal of trailing whitespaces: `sff -k yourfile.txt`
