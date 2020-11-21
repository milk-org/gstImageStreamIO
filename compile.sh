#!/bin/bash
if [ ! -d build ]
then
        echo "Create build directory"
        mkdir -p build
fi

cd build

# BUILD_TOOL="-GNinja" # build with ninja instead of make
# DEBUG="-DCMAKE_BUILD_TYPE=Debug"
# NCPUS=`fgrep processor /proc/cpuinfo | wc -l`

cmake .. $DEBUG $BUILD_TOOL
cmake --build . -- -j $NCPUS
echo install libgstimageStreamIO.so in /opt/gstreamer/lib64/gstreamer-1.0
sudo cp libgstimageStreamIO.so /opt/gstreamer/lib64/gstreamer-1.0/libgstimageStreamIO.so
