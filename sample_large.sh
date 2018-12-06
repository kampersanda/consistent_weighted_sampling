echo "#### Step 1. Create working space. ####"
if [ -e sample-large-build ]; then
  echo "...removing the old working space."
  rm -rf sample-large-build
fi
mkdir sample-large-build
cd sample-large-build

echo -e "\n#### Step 2. Complile the source files. ####"
cmake ..
make

echo -e "\n#### Step 3. Download a large dataset. ####"
wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/covtype.scale01.bz2
bzip2 -d covtype.scale01.bz2

echo -e "\n#### Step 4. Split the dataset. ####"
mkdir split-data
split -l 100000 -a 1 covtype.scale01 split-data/covtype.scale01-
find $PWD/split-data/* > data-list.txt

echo -e "\n#### Step 5. Generate random data. ####"
./gen_random_data -r covtype.scale01.rnd -m 54 -s 32

echo -e "\n#### Step 6. Run sampling for the split data ####"
./cws_large -d data-list.txt -r covtype.scale01.rnd -m 54 -s 32 -b 4 -i 1 -w 1 -l 1 -t 4

echo -e "\n#### Step 7. Concatenate the sub sampled data. ####"
find $PWD/split-data/*.cws > cws-list.txt
./concatenate -d cws-list.txt -o covtype.scale01.32.4.cws

echo -e "\nGood job!! You obtained the sampled dataset covtype.scale01.32.4.cws as follows:"
head covtype.scale01.32.4.cws
echo "..."
