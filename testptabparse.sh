#!/bin/sh
# Test which files can't be parsed
for I in $*
do
	./ptbinfo "$I" > /dev/null 2>/dev/null || echo $I
done
