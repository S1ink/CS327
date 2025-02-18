#!/bin/bash

make clean
rm -f richter_sam.assignment-1.03.tar.gz
cp -R ../game/ ./richter_sam.assignment-1.03
rm -f ./richter_sam.assignment-1.03/export.sh ./richter_sam.assignment-1.03/crash_count.txt ./richter_sam.assignment-1.03/*.tar.gz
pushd ./richter_sam.assignment-1.03/src/util
rm -f ./array.h ./grid.h ./list.h ./queue.h ./stack.h
popd
tar cvfz richter_sam.assignment-1.03.tar.gz ./richter_sam.assignment-1.03/
rm -rf ./richter_sam.assignment-1.03/
