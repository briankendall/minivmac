#!/usr/bin/env bash
set -e
set -x

rm -rf ./dist
mkdir -p ./dist

COMMON_FLAGS=${1:-'-n minivmac-3.7-2024.02.19 -bg 1 -sony-tag 1 -sony-sum 1 -sony-dc42 1 -sound 1 -drives 20 '}

./build_mac.sh $COMMON_FLAGS
./build_lin.sh $COMMON_FLAGS
./build_win.sh $COMMON_FLAGS

cd dist
for file in ./*; 
do
    zip -vr $file.zip $file
done
mkdir -p zips
mv *.zip zips/
cd ..
