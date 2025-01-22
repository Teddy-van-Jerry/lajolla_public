#!/bin/sh

build/lajolla -o images/disney_metal.exr scenes/disney_bsdf_test/disney_metal.xml
build/lajolla -o images/roughplastic.exr scenes/disney_bsdf_test/roughplastic.xml
build/lajolla -o images/disney_metal_roughness0.8.exr scenes/disney_bsdf_test/disney_metal_roughness0.8.xml
