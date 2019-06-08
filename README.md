# consistent\_weighted\_sampling

This software implements *consistent weight sampling (CWS)*, a similarity-preserving hashing technique for weighted Jaccard (or min-max) similarity, and approximate nearest neighber (ANN) search via CWS [1].
The software applies a simplification of the original CWS method called *0-bit CWS* that generates non-negative integer vectors [2,3,4].

## Build instruction

You can download and compile the software as follows.

```
$ git clone https://github.com/kampersanda/consistent_weighted_sampling.git
$ cd consistent_weighted_sampling
$ mkdir build && cd build && cmake ..
$ make
```

The library uses C++17, so please install g++ 7.0 (or greater) or clang 4.0 (or greater).
Also, CMake 2.8 (or greater) has to be installed to compile the software.

## Input vector file formats supported

The software supports two input formats `ascii` and `texmex`.

### `ascii` format

This format is based on [LIBSVM format](https://www.csie.ntu.edu.tw/~r94100/libsvm-2.8/README) whose each feature vector is written in ASCII, as follows.

```
<label> <index1>:<value1> <index2>:<value2> ...
<label> <index1>:<value1> <index2>:<value2> ...
.
.
.
<label> <index1>:<value1> <index2>:<value2> ...
```

Based on the LIBSVM format, four format patterns are supported:

- with `<label>` and `<value>` (as above)
- with `<label>` but without `<value>`
- without `<label>` but with `<value>`
- without `<label>` and `<value>`

For example, binary vectors represented as follows can be also input.

```
197 321 399 561 575 587 917 1563 1628
7 1039 1628 1849 2686 2918 3135 4039 4059
77 137 248 271 357 377 400 412 678
```

### `texmex` format

`.bvecs` and `.fvecs` formats used in [BIGANN](http://corpus-texmex.irisa.fr) are supported. In detail, see the project page of [BIGANN](http://corpus-texmex.irisa.fr).

## Running example for dataset news20

I explain the usage of the software via a running example.

You are at directory `consistent_weighted_sampling/build` through the compiling process.
Then, you can try a small demo for dataset [news20](https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass.html#news20) provided from LIBSVM.

```
$ bash scripts/demo_news20.sh
```

The demo script runs as follows.

### (1) Download the dataset

Directory `news20` is made to put the dataset files.

```
$ mkdir news20
```

The dataset `news20.scale.bz2` is downloaded to be used as a database.

```
$ wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.scale.bz2
$ bzip2 -d news20.scale.bz2
$ mv news20.scale news20/news20.scale_base.txt
```

The dataset `news20.t.scale.bz2` is downloaded and the top 100 feature vectors are used as a query collection.

```
$ wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.t.scale.bz2
$ bzip2 -d news20.t.scale.bz2
$ head -100 news20.t.scale > news20/news20.scale_query.txt
$ rm news20.t.scale
```

As a result, there should be the database file `news20/news20.scale_base.txt` and query collection file `news20/news20.scale_query.txt`.

### (2) Generate random matrix data

Random matrix data are generated for use in CWS.

```
$ ./bin/generate_random_data -r news20/news20.scale -d 62061 -D 64 -g 0
```

Then, the options are

- `-r` indicates the output file name.
- `-d` indicates the dimension (i.e., # of features) of the input feature vectors (`news20` in this case).
- `-D` indicates the maximum dimension of CWS vectors generated.
- `-g` indicates whether or not generalization is needed, i.e., feature values include negative values.

As a result, there should be the random matrix data file `news20/news20.scale.62061x64.rnd`.

### (3) Generate CWS vectors from the database

CWS vectors are generated from `news20.scale_base.txt`.

```
$ ./bin/cws_in_ascii -i news20/news20.scale_base.txt -r news20/news20.scale.62061x64.rnd -o news20/news20.scale_base.cws -d 62061 -D 64 -b 1 -w 1 -l 1 -g 0
```

Then, the options are

- `-i` indicates the input file name of feature vectors.
- `-r` indicates the input file name of the random matrix data.
- `-o` indicates the output file name of CWS vectors in `.bvecs` format.
- `-d` indicates the dimension (i.e., # of features) of the input feature vectors (`news20` in this case).
- `-D` indicates the dimension of CWS vectors generated. This value must be no more than the maximum dimension set in process (2).
- `-b` indicates at which `<index>` starts.
- `-w` indicates whether or not there is `<value>` column in the input file.
- `-l` indicates whether or not there is `<label>` column in the input file.
- `-g` indicates whether or not generalization is needed, i.e., feature values include negative values.

As a result, there should be the CWS data file `news20/news20.scale_base.cws.bvecs`.

#### Range of sampled values

As generated CWS vectors are in `.bvecs` format, each value of them is represented in 8 bits, i.e., in the range between 0 and 255.
In other words, parameter *b* in [2,3,4] is 8.
If you want to use *b* smaller than 8, please take the lowest *b* bits of each value.

### (4) Generate CWS vectors from the query collection

CWS vectors are generated from `news20.scale_query.txt` in the same manner.

```
$ ./bin/cws_in_ascii -i news20/news20.scale_query.txt -r news20/news20.scale.62061x64.rnd -o news20/news20.scale_query.cws -d 62061 -D 64 -b 1 -w 1 -l 1 -g 0
```

As a result, there should be the CWS data file `news20/news20.scale_query.cws.bvecs`.

### (5) Make groundtruth data in (weighted) Jaccard similarity

To evaluate kANN search in process (7), make groundtruth data in (weighted) Jaccard similarity from `news20.scale_base.txt` and `news20.scale_query.txt`.

```
./bin/make_groundtruth_in_ascii -i news20/news20.scale_base.txt -q news20/news20.scale_query.txt -o news20/news20.scale_groundtruth -b 1 -w 1 -l 1 -g 0
```

Option `-o` indicates the output file name of the groundtruth.

As a result, there should be the groundtruth file `news20/news20.scale_groundtruth.txt`.

### (6) Perform kANN search

Search kNN vectors from the database `news20.scale_base.cws.bvecs` for each query vector in `news20.scale_query.cws.bvecs`.

```
./bin/search -i news20/news20.scale_base.cws.bvecs -q news20/news20.scale_query.cws.bvecs -o news20/news20.scale_score -b 8 -d 64 -k 100
```

Then, the options are

- `-o` indicates the output file name of the search results.
- `-b` indicates the lowest *b* bits to be used.
- `-d` indicates the dimension of CWS vectors to be used.
- `-k` indicates the top-k value to be searched.

As a result, there should be the result file `news20/news20.scale_score.topk.8x64.txt`.

### (7) Evaluate the recall

Evaluate the recalls for the search results.

```
./scripts/evaluate.py news20/news20.scale_score.topk.8x64.txt news20/news20.scale_groundtruth.txt
```

The following output is an example of the obtained result.

```
Recall@1:	0.600
Recall@2:	0.660
Recall@5:	0.770
Recall@10:	0.810
Recall@20:	0.840
Recall@50:	0.880
Recall@100:	0.900
```

## References

1. Mark Manasse, Frank McSherry and Kunal Talwar: **Consistent weighted sampling**, *Microsoft Research Technical Report*, 2010.
2. Ping Li: **0-bit consistent weighted sampling**, *KDD*, 2015.
3. Ping Li: **Linearized GMM kernels and normalized random fourier features**, *KDD*, 2017.
4. Ping Li and Cun-Hui Zhang: **Theory of the GMM Kernel**, *WWW*, 2017.

