#!/bin/bash

echo "(1) Downlowd and extract news20 data"
if [ -e news20 ]; then
    rm -rf news20
fi
mkdir news20
wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.scale.bz2
bzip2 -d news20.scale.bz2
mv news20.scale news20/news20.scale_base.txt
wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.t.scale.bz2
bzip2 -d news20.t.scale.bz2
head -100 news20.t.scale > news20/news20.scale_query.txt
rm news20.t.scale
echo ""

echo "(2) Generate random matrix data"
./bin/generate_random_data -r news20/news20.scale -d 62061 -D 64 -g 0
echo ""

echo "(3) Do consistent weighted sampling to base data"
./bin/cws_in_ascii -i news20/news20.scale_base.txt -r news20/news20.scale.62061x64.rnd -o news20/news20.scale_base.cws -d 62061 -D 64 -b 1 -w 1 -g 0 -l 1
echo ""
echo "The generated CWS-sketches are as follows..."
./bin/cws_to_txt -i news20/news20.scale_base.cws.bvecs -n 5
echo ""

echo "(4) Do consistent weighted sampling to query data"
./bin/cws_in_ascii -i news20/news20.scale_query.txt -r news20/news20.scale.62061x64.rnd -o news20/news20.scale_query.cws -d 62061 -D 64 -b 1 -w 1 -g 0 -l 1
echo ""
echo "The generated CWS-sketches are as follows..."
./bin/cws_to_txt -i news20/news20.scale_query.cws.bvecs -n 5
echo ""

echo "(5) Make groundtruth data in minmax similarity for topk search"
./bin/make_groundtruth_in_ascii -i news20/news20.scale_base.txt -q news20/news20.scale_query.txt -o news20/news20.scale_groundtruth -b 1 -w 1 -g 0 -l 1
echo ""

echo "(6) Do topk search for the CWS-sketches"
./bin/search -i news20/news20.scale_base.cws.bvecs -q news20/news20.scale_query.cws.bvecs -o news20/news20.scale_score -b 8 -d 64
echo ""

echo "(7) Evaluate the recall"
./scripts/evaluate.py news20/news20.scale_score.topk.8x64.txt news20/news20.scale_groundtruth.txt
echo ""
