#!/bin/bash

make clean
rm -f richter_sam.assignment-*.tar.gz
cp -R ../final/ ../richter_sam.assignment-1.10
rm -rf ../richter_sam.assignment-1.10/include
rm -f ../richter_sam.assignment-1.10/export.sh \
    ../richter_sam.assignment-1.10/ref.html \
    ../richter_sam.assignment-1.10/src/old.cpp \
    ../richter_sam.assignment-1.10/src/util/grid.hpp
tar cvfz richter_sam.assignment-1.10.tar.gz ../richter_sam.assignment-1.10/
rm -rf ../richter_sam.assignment-1.10/