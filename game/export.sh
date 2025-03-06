#!/bin/bash

make clean
rm -f richter_sam.assignment-1.04.tar.gz
cp -R ../game/ ./richter_sam.assignment-1.04
rm -f ./richter_sam.assignment-1.04/export.sh ./richter_sam.assignment-1.04/crash_count.txt ./richter_sam.assignment-1.04/*.tar.gz
tar cvfz richter_sam.assignment-1.04.tar.gz ./richter_sam.assignment-1.04/
rm -rf ./richter_sam.assignment-1.04/
