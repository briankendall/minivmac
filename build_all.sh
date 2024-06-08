#!/usr/bin/env bash
set -e
set -x

rm -rf ./dist
mkdir -p ./dist
BUILD_TYPE=$1
COMMON_FLAGS='-n minivmac-3.7-2024.02.19 -bg 1 -sony-tag 1 -sony-sum 1 -sony-dc42 1 -sound 1 -drives 20 '
if [[ $BUILD_TYPE == "ltoudp" ]]; then
    echo "Adding LocalTalk UDP options, Speed set to 1."
    COMMON_FLAGS="$COMMON_FLAGS -speed z -lto udp -lt "
fi
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
