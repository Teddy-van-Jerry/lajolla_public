#!/bin/sh

build/lajolla -o images/volpath_test2.exr scenes/volpath_test/volpath_test2.xml
build/lajolla -o images/volpath_test2_sigmaA0.5.exr scenes/volpath_test/volpath_test2_sigmaA0.5.xml
build/lajolla -o images/volpath_test2_sigmaS0.1.exr scenes/volpath_test/volpath_test2_sigmaS0.1.xml
build/lajolla -o images/volpath_test2_g-0.5.exr scenes/volpath_test/volpath_test2_g-0.5.xml
build/lajolla -o images/volpath_test2_g0.exr scenes/volpath_test/volpath_test2_g0.xml
build/lajolla -o images/volpath_test2_g0.5.exr scenes/volpath_test/volpath_test2_g0.5.xml
