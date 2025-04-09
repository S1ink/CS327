#!/bin/bash

make clean
rm -f richter_sam.assignment-*.tar.gz
cp -R ../game/ ../richter_sam.assignment-1.07
rm -rf ../richter_sam.assignment-1.07/ext
rm -f ../richter_sam.assignment-1.07/export.sh
tar cvfz richter_sam.assignment-1.07.tar.gz ../richter_sam.assignment-1.07/
rm -rf ../richter_sam.assignment-1.07/
