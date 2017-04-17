#!/bin/bash

set -ev

make -j 5 ARCH=i386 BUILD_CLIENT=0
