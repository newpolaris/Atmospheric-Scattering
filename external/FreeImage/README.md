# FreeImage 3.15.3

This repository adds CMake and CI support to [FreeImage](http://freeimage.sourceforge.net/).

## Build

To build FreeImage with CMake
```C++
  mkdir build
  cd build
  cmake .. 
  camke --build .
```
which will setup CMake with your default build-system (e.g Make on Linux) and build it.

## Continuous integration  <a id="continuous-integration"></a>

|    OS   |  Status                                                                                                                 |
|:------- |:-----------------------------------------------------------------------------------------------------------------------:|
| Mac OSX | [![Build Status](https://travis-ci.org/thfabian/FreeImage.svg?branch=master)](https://travis-ci.org/thfabian/FreeImage) | 
| Linux   | [![Build Status](https://travis-ci.org/thfabian/FreeImage.svg?branch=master)](https://travis-ci.org/thfabian/FreeImage) |
