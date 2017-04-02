#!/bin/bash

PINATRACE=/home/zastre/pin/source/tools/ManualExamples/obj-intel64/csc360trace.so
PINBIN=/home/zastre/pin/pin
PINOUT=csc360trace.out
MV=/bin/mv

set -x

BASENAME=/usr/bin/basename
OUTNAME=$($BASENAME "$1")
$PINBIN -injection child -t $PINATRACE -- $1
$MV $PINOUT ./${OUTNAME}_out.txt
