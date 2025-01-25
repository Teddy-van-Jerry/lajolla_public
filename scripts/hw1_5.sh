#!/bin/sh

build/lajolla -o images/disney_sheen.exr scenes/disney_bsdf_test/disney_sheen.xml
build/lajolla -o images/simple_sphere_disney_sheen.exr scenes/disney_bsdf_test/simple_sphere_disney_sheen.xml
build/lajolla -o images/simple_sphere_disney_sheen_grazing.exr scenes/disney_bsdf_test/simple_sphere_disney_sheen_grazing.xml
build/lajolla -o images/disney_sheen_tint0.2.exr scenes/disney_bsdf_test/disney_sheen_tint0.2.xml
build/lajolla -o images/disney_sheen_tint0.8.exr scenes/disney_bsdf_test/disney_sheen_tint0.8.xml
