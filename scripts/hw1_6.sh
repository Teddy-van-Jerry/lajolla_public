#!/bin/sh

build/lajolla -o images/disney_bsdf.exr scenes/disney_bsdf_test/disney_bsdf.xml
build/lajolla -o images/disney_bsdf_specularTint0.2.exr scenes/disney_bsdf_test/disney_bsdf_specularTint0.2.xml
build/lajolla -o images/disney_bsdf_specularTint0.8.exr scenes/disney_bsdf_test/disney_bsdf_specularTint0.8.xml
