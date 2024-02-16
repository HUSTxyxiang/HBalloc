rm test
gcc -o test test.c ../src/*.c -lpthread

rm -rf /mnt/pmem0/HBalloc
mkdir /mnt/pmem0/HBalloc

./test 64 1000000 100 128