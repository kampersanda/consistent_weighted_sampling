# zero\_bit\_cws

## How to build

```
$ git clone https://github.com/kampersanda/zero_bit_cws.git
$ cd zero_bit_cws
$ mkdir build && cd build && cmake ..
$ make
```


## Running Example

```
$ ./gen_rand -r siftsmall_base -d 128 -D 64 -g 0
Generating random data with gamma_distribution...
Generating random data with gamma_distribution...
Generating random data with uniform_real_distribution...
Output siftsmall_base.128x64.rnd
```

```
$ ./cws_ANN -i siftsmall_base.fvecs -r siftsmall_base.128x64.rnd -o siftsmall_base -d 128 -D 64
Loading random matrix data...
The data are consuming 0.09375 MiB...
Done!! --> 10000 vecs processed in 0h0m1s!!
Output siftsmall_base.cws
```

```
$ ./to_txt -i siftsmall_base.cws -n 5
64
49 86 104 50 9 124 41 59 52 25 92 127 109 113 40 110 60 86 115 7 7 71 85 32 45 102 77 78 52 92 50 33 56 47 70 42 71 50 27 78 96 60 26 125 60 42 92 1 4 19 52 64 85 83 87 37 63 88 24 79 79 79 27 59 
49 86 71 50 9 127 41 78 52 25 32 127 109 113 40 110 125 86 30 109 7 29 85 32 9 0 77 78 52 92 118 33 56 47 70 42 33 50 27 78 1 86 26 125 60 42 120 1 119 19 109 64 85 32 87 37 93 88 40 40 79 79 27 43 
49 86 104 50 9 124 41 59 52 25 92 92 109 113 40 110 60 86 115 7 7 71 85 32 9 102 77 78 52 92 50 33 5 47 70 42 71 50 27 78 96 60 26 125 60 42 103 60 4 19 52 64 85 32 87 37 49 59 24 79 79 79 27 59 
49 86 71 50 9 32 41 26 52 25 32 127 109 113 40 110 125 86 67 109 7 29 85 32 9 0 77 78 52 92 118 33 79 47 70 42 33 50 27 78 1 51 26 125 60 42 120 1 125 19 52 64 85 32 87 37 49 88 40 40 79 79 27 110 
49 86 104 50 9 127 41 59 27 25 96 127 85 113 40 110 60 86 67 7 7 29 85 32 9 27 59 78 71 92 50 33 6 47 118 99 33 50 27 78 96 86 26 125 60 42 10 60 119 19 41 64 85 32 87 37 49 59 40 40 79 79 27 58 
...
```