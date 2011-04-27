#!/bin/bash

USFDUMP="../tools/usfdump"
USF2USF="../tools/usf2usf"

USFFILE="./data/gcc.usf"
REFFILE="./data/gcc_ref.txt"
TMPFILE1="/tmp/usf_test.usf"
TMPFILE2="/tmp/usf_test.txt"

RETVAL=0

function run_test {
    opts=$1; shift

    $USF2USF $opts $USFFILE $TMPFILE1
    $USFDUMP $TMPFILE1 | grep -iE "^\[(trace|burst|sample|dangling)\]" > $TMPFILE2

    diff $REFFILE $TMPFILE2
    if [ "$?" != "0" ]; then
        echo "FAILED: $compression $delta_comp"
	RETVAL=1
    fi

}

run_test "-c none"
run_test "-c none -d"
run_test "-c bzip2"
run_test "-c bzip2 -d"

exit $RETVAL
