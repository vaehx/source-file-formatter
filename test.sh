#!/bin/bash

SFF="Release/sff.exe"
if [ ! -f $SFF ]; then
	echo "Error: Release build not found at $SFF!"
	exit 1
fi

function escape_whitespaces {
	echo "$(echo "$1" | sed -r 's/ /\./g; s/\t/\\t/g')"
}

num_succeeded=0
num_run=0

# param 1: test description
# param 2: original content
# param 3: expected result
function do_test {
	printf "Test '$1'... "

	echo "$2" > sff-unittest.txt
	$SFF sff-unittest.txt > /dev/null
	if [ "$(cat sff-unittest.txt)" != "$3" ]; then
		echo "Failed!"
		echo "  Input   : \"$(escape_whitespaces "$2")\""
		echo "  Expected: \"$(escape_whitespaces "$3")\""
		echo "  Actual  : \"$(escape_whitespaces "$(cat sff-unittest.txt)")\""
		echo
	else
		echo "OK"
		((num_succeeded++))
	fi

	((num_run++))
}

function test_clean {
	rm sff-unittest.txt 2> /dev/null
}

do_test "Leading mixed" "  	  	    CONTENT" "			CONTENT"
do_test "Trailing mixed" "CONTENT	   		 " "CONTENT"

echo
echo "----------------------------------------------"
echo "Tests run: $num_run"
echo "Succeeded: $num_succeeded"

test_clean
