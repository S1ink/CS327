#!/bin/bash

make clean
rm -f richter_sam.assignment-1.01.tar.gz
cp -R ../game/ ./richter_sam.assignment-1.01
rm -f ./richter_sam.assignment-1.01/export.sh
pushd ./richter_sam.assignment-1.01/src/util
rm -f ./array.h ./grid.h ./list.h ./queue.h ./stack.h
popd
tar cvfz richter_sam.assignment-1.01.tar.gz ./richter_sam.assignment-1.01/
rm -rf ./richter_sam.assignment-1.01/
