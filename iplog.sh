#!/bin/bash

./iplog \
	<(sed -ne '/^\(\S\+\) \([A-Z]\{1,100\}\)$/!{q98};p' $1 || echo "ERROR: Bad format") \
	<(sed -ne '/^\(\S\+\) \([1-9][0-9]\{,19\}\)$/!{q99};p' $2 || echo "ERROR: Bad format") \
	$3
# echo $?