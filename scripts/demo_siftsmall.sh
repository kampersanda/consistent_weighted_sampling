#!/bin/bash

echo "(1) Downlowd and extract siftsmall vectors"
if [ ! -e siftsmall.tar.gz ]; then
    wget ftp://ftp.irisa.fr/local/texmex/corpus/siftsmall.tar.gz
fi
if [ -e siftsmall ]; then
    rm -rf siftsmall
fi
tar -zxvf siftsmall.tar.gz
echo ""

echo "(2) Generate random matrix data"
./bin/generate_random_data -r siftsmall/siftsmall -d 128 -D 64 -g 0
echo ""

echo "(3) Do consistent weighted sampling to base data"
./bin/cws_in_texmex -i siftsmall/siftsmall_base.fvecs -r siftsmall/siftsmall.128x64.rnd -o siftsmall/siftsmall_base.cws -d 128 -D 64
echo "The generated CWS-sketches are as follows..."
./bin/cws_to_txt -i siftsmall/siftsmall_base.cws.bvecs -n 5
echo ""

echo "(4) Do consistent weighted sampling to query data"
./bin/cws_in_texmex -i siftsmall/siftsmall_query.fvecs -r siftsmall/siftsmall.128x64.rnd -o siftsmall/siftsmall_query.cws -d 128 -D 64
echo "The generated CWS-sketches are as follows..."
./bin/cws_to_txt -i siftsmall/siftsmall_query.cws.bvecs -n 5
echo ""

echo "(5) Make groundtruth data in minmax similarity for topk search"
./bin/make_groundtruth_in_texmex -i siftsmall/siftsmall_base.fvecs -q siftsmall/siftsmall_query.fvecs -o siftsmall/siftsmall_groundtruth -d 128
echo ""

echo "(6) Do topk search for the CWS-sketches"
./bin/topk_search -i siftsmall/siftsmall_base.cws.bvecs -q siftsmall/siftsmall_query.cws.bvecs -o siftsmall/siftsmall_score -b 8 -d 64
echo ""

echo "(7) Evaluate the recall"
python scripts/topk_evaluate.py siftsmall/siftsmall_score.topk.8x64.txt siftsmall/siftsmall_groundtruth.txt
echo ""
