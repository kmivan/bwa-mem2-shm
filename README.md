# BWA-MEM2-SHM

BWA-MEM2-SHM is a improved version of [BWA-MEM2](https://github.com/bwa-mem2/bwa-mem2), which adds the database into shared memory , which improves the space locality of the processes, and consequently achieves better performance and lower space overhead.


## Getting Started
```sh
# Compile using CMake
git clone https://github.com/kmivan/bwa-mem2-shm.git
cd bwa-mem2-shm
mkdir build
cd build
cmake ..
make

# Run BWA-MEM2-SHM as you do with BWA-MEM2
./bwa-mem2-shm mem ref.fa read1.fq read2.fq > out.sam 
```

## Using Shared Memory
BWA-MEM2-SHM works with reference databases same as BWA-MWM2, and it allows users to store the reference database into shared memory.

### 1. Add Index Database Into Shared Memory
```sh
./bwa-mem2-shm shm add $ID $PREFIX
```
where `$ID` is the identifier specified by the user, and `$PREFIX` is the path prefix of index database. For example,
```sh
./bwa-mem2-shm shm add hg38 /foo/bar/hg38
```
adds database `/foo/bar/hg38.*` into shared memory and name it `hg38`.

### 2. Use Database Added into Shared Memory
```sh
./bwa-mem2-shm mem SHM::$ID read1.fq read2.fq > out.sam 
```
where `$ID` is the identifier you specified in the previous step. `SHM::` indicates that it's the identifier of a database in shared memory rather than a path. For example,
```sh
./bwa-mem2-shm mem SHM::hg38 read1.fq read2.fq > out.sam 
```
uses the database `hg38` which we have added to shared memory in the previous step.

## Citation




