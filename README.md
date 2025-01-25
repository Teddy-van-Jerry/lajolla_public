# lajolla
UCSD CSE 272 renderer ([Wuqiong Zhao](https://wqzhao.org)'s version, Winter 2025).

- Course website: https://cseweb.ucsd.edu/~tzli/cse272/wi2025.
- GitHub repo (Wuqiong Zhao's version): [Teddy-van-Jerry/lajolla_wqzhao](https://github.com/Teddy-van-Jerry/lajolla_wqzhao).

> [!IMPORTANT]
> Support for Windows has been dropped as I do not use Windows for development.

## Build
This project uses CMake to manage the build process:
```sh
mkdir build
cd build
cmake ..
cmake --build .
```
It requires compilers that support C++17 (gcc version >= 8, clang version >= 7, Apple Clang version >= 11.0).

The dependency [`embree`](https://github.com/RenderKit/embree) is required.
Apple M1 users need to build Embree from scratch since the prebuilt MacOS binary provided is built for x86 machines:
```sh
brew install tbb
git clone https://github.com/RenderKit/embree.git --depth=1
cd embree && mkdir build && cd build
cmake ..
make -j
sudo make install
```

## Run
Try
```sh
cd build
./lajolla ../scenes/cbox/cbox.xml
```
This will generate an image "image.exr".

To view the image, use [hdrview](https://github.com/wkjarosz/hdrview), or [tev](https://github.com/Tom94/tev).
The macOS built-in Preview app can also open EXR files.

## Acknowledgment
The renderer is heavily inspired by [pbrt](https://pbr-book.org/), [mitsuba](http://www.mitsuba-renderer.org/index_old.html), and [SmallVCM](http://www.smallvcm.com/).

- We use [Embree](https://www.embree.org/) for ray casting.
- We use [pugixml](https://pugixml.org/) to parse XML files.
- We use [pcg](https://www.pcg-random.org/) for random number generation.
- We use [stb_image](https://github.com/nothings/stb) and [tinyexr](https://github.com/syoyo/tinyexr) for reading & writing images.
- We use [miniz](https://github.com/richgel999/miniz) for compression & decompression.
- We use [tinyply](https://github.com/ddiakopoulos/tinyply) for parsing PLY files.

Many scenes in the scenes folder are directly downloaded from [http://www.mitsuba-renderer.org/download.html](http://www.mitsuba-renderer.org/download.html). Scenes courtesy of Wenzel Jakob, Cornell Program of Computer Graphics, Marko Dabrovic, Eric Veach, Jonas Pilo, and Bernhard Vogl.
