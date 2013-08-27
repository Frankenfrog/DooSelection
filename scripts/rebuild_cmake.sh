#!/bin/sh

if [ -f CMakeLists.txt ]
then 
  echo "Directory contains CMakeLists.txt. You should use a clean out of source build. Exiting."
  exit 1
fi

if [ -f CMakeCache.txt ]
then 
  echo "Directory contains CMake files, will clean up."
  rm -rf bin/ CMake* cmake_install.cmake Makefile lib/ main/ src/
fi

echo "Building CMake build system for installation to $DOOSELECTIONSYS."
if [ "$1" = "debug" ]
then
  cmake -DCMAKE_INSTALL_PREFIX=$DOOSELECTIONSYS -DCMAKE_BUILD_TYPE=Debug ..
elif [ "$1" = "profiling" ]
then
  cmake -DCMAKE_INSTALL_PREFIX=$DOOSELECTIONSYS -DCMAKE_BUILD_TYPE=Profiling ..
else 
  cmake -DCMAKE_INSTALL_PREFIX=$DOOSELECTIONSYS -DCMAKE_BUILD_TYPE=Release ..
fi
