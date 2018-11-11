mkdir sample_build
cd sample_build

cmake ..
make

wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.scale.bz2
bzip2 -d news20.scale.bz2

./gen_random_data -r news20.scale.rnd -m 62061 -s 32 -g 0
./cws -d news20.scale -r news20.scale.rnd -m 62061 -s 32 -b 4 -i 1 -w 1 -g 0 -l 1

head news20.scale.32.4.cws