#!/bin/bash

make clean
cp -R ../week0/ ./richter_sam.assignment-0
rm -f ./richter_sam.assignment-0/export.sh
tar cvfz richter_sam.assignment-0.tar.gz ./richter_sam.assignment-0/
rm -rf ./richter_sam.assignment-0/
