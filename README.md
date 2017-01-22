# Source-File-Formatter
Command-line sourcefile formatting tool.
This tool adds a new empty line(only \n unless -ra is used) at end of file by default, if not there yet.
Also removes trailing whitespaces by default. Carriage Returns will be preserved
if present unless the -ra or -rr options are used.

* Replace tab indent with space indent: `sff -s yourfile.txt`
* Replace space indent with tab indent: `sff -t yourfile.txt`
* Replace spaces inside line with tabs: `sff -s2t yourfile.txt`
* Replace tabs inside line with spaces: `sff -t2s yourfile.txt`
* Replace /* ... */ at end of single line with // ...: `sff -c yourfile.txt`
* Specify size of tab in spaces before convert (default 4): `sff -szb 8 yourfile.txt`
* Specify size of tab in spaces after convert (default 4): `sff -sza 8 yourfile.txt`
* Add carriage return at end of line if missing (CRLF): `sff -ra yourfile.txt`
* Remove carriage return at end of line if present: `sff -rr yourfile.txt`
* Disable automatic removal of trailing whitespaces: `sff -k yourfile.txt`
