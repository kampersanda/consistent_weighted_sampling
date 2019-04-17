#!/bin/bash

echo "(1) Downlowd and extract siftsmall vectors"
wget ftp://ftp.irisa.fr/local/texmex/corpus/siftsmall.tar.gz
tar -zxvf siftsmall.tar.gz
echo ""

echo "(2) Generate random matrix data"
./generate_random_data -r siftsmall -d 128 -D 64 -g 0
echo ""

echo "(3) Do consistent weighted sampling to base/query data"
./cws_in_texmex -i siftsmall/siftsmall_base.fvecs -r siftsmall.128x64.rnd -o siftsmall_base_cws -d 128 -D 64
./cws_in_texmex -i siftsmall/siftsmall_query.fvecs -r siftsmall.128x64.rnd -o siftsmall_query_cws -d 128 -D 64
echo ""

echo "(4) Make groundtruth data in minmax similarity for topk search"
./make_groundtruth_in_texmex -i siftsmall/siftsmall_base.fvecs -q siftsmall/siftsmall_query.fvecs -o siftsmall_groundtruth -d 128
echo ""

echo "(5) Do topk search for the CWS-data vectors"
./topk_search -i siftsmall_base_cws.bvecs -q siftsmall_query_cws.bvecs -o siftsmall_score -b 8 -d 64
echo ""

echo "(6) Evaluate the recall"
python scripts/evaluate.py siftsmall_score_8x64.txt siftsmall_groundtruth.txt
echo ""
