#!/bin/sh

build/lajolla -o images/disney_clearcoat.exr scenes/disney_bsdf_test/disney_clearcoat.xml
build/lajolla -o images/disney_metal_roughness0.0505.exr scenes/disney_bsdf_test/disney_metal_roughness0.0505.xml # for comparison
