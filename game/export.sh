#!/bin/bash

make clean
rm -f richter_sam.assignment-*.tar.gz
cp -R ../game/ ../richter_sam.assignment-1.05
rm -rf ../richter_sam.assignment-1.05/ext
rm -f ../richter_sam.assignment-1.05/export.sh
tar cvfz richter_sam.assignment-1.05.tar.gz ../richter_sam.assignment-1.05/
rm -rf ../richter_sam.assignment-1.05/
