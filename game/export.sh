#!/bin/bash

make clean
rm -f richter_sam.assignment-*.tar.gz
cp -R ../game/ ../richter_sam.assignment-1.09
rm -rf ../richter_sam.assignment-1.09/ext
rm -f ../richter_sam.assignment-1.09/export.sh
tar cvfz richter_sam.assignment-1.09.tar.gz ../richter_sam.assignment-1.09/
rm -rf ../richter_sam.assignment-1.09/
