echo "#### Step 1. Create working space. ####"
if [ -e sample-build ]; then
  echo "...removing the old working space."
  rm -rf sample-build
fi
mkdir sample-build
cd sample-build

echo -e "\n#### Step 2. Complile the source files. ####"
cmake ..
make

echo -e "\n#### Step 3. Download a small dataset. ####"
wget https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass/news20.scale.bz2
bzip2 -d news20.scale.bz2

echo -e "\n#### Step 4. Generate random data. ####"
./gen_random_data -r news20.scale.rnd -m 62061 -s 32 -g 0

echo -e "\n#### Step 5. Run sampling. ####"
./cws -d news20.scale -r news20.scale.rnd -m 62061 -s 32 -b 4 -i 1 -w 1 -g 0 -l 1

echo -e "\nGood job!! You obtained the sampled dataset news20.scale.32.4.cws as follows:"
head news20.scale.32.4.cws
echo "..."
