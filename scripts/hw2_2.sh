#!/bin/sh

build/lajolla -o images/volpath_test1.exr scenes/volpath_test/volpath_test1.xml
build/lajolla -o images/volpath_test1_sigmaA0.1.exr scenes/volpath_test/volpath_test1_sigmaA0.1.xml
build/lajolla -o images/volpath_test1_sigmaA0.9.exr scenes/volpath_test/volpath_test1_sigmaA0.9.xml
