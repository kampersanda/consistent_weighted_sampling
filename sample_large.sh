mkdir sample_large_build
cd sample_large_build

cmake ..
make

wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/vehicle/combined_scale.bz2
bzip2 -d combined_scale.bz2

mkdir split-data
split -l 20000 -a 1 combined_scale split-data/combined_scale-
find $PWD/split-data/* > data-list.txt

./gen_random_data -r combined_scale.rnd -m 100 -s 32 -g 1
./cws_large -d data-list.txt -r combined_scale.rnd -m 100 -s 32 -b 4 -i 1 -w 1 -g 1 -l 1

find split-data/*.cws | xargs head
