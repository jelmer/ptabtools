#!/bin/sh
# Test which files can't be parsed
for I in $*
do
	echo "$I"
	./ptbinfo "$I" > /dev/null 2>/dev/null
	./ptb2xml -q "$I" || echo "xml: $I"
	./ptb2ly -q "$I" || echo "ly: $I"
done
