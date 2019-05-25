# consistent\_weighted\_sampling

This software provides the 0-bit consistent weighted sampling (0-bit CWS), which is 

## Build Instruction

You can download and compile the software as follows.

```
$ git clone https://github.com/kampersanda/consistent_weighted_sampling.git
$ cd consistent_weighted_sampling
$ mkdir build && cd build && cmake ..
$ make
```

The library uses C++17, so please install g++ 7.0 (or greater) or clang 4.0 (or greater).
Also, CMake 2.8 (or greater) has to be installed to compile the software.

## Running Example for dataset news20

You can try a small demo for dataset [news20](https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass.html#news20).

```
$ bash scripts/demo_news20.sh
```
