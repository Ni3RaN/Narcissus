mkdir build
cp config.json build
cd build || exit
cmake ..
make