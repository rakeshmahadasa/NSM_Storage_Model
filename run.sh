#!/bin/bash 
function print_info {
    echo "Usage:"
    echo "--------------------------------------------------------" 
    echo "ksh run.ksh <page_size> <record_size> <read mode> <scan size>"
    echo "page size options : 1024,4096,16384"
    echo "record size options : 8,64,256"
    echo "read mode options : r for random , s for sequential reads"
    echo "scan size options : 10,100,1000"
    echo "--------------------------------------------------------" 
}
if [[ $# -lt 4 ]]; then 
    echo "Less Number of Arguments provided !!!!" 
    print_info
    exit 1 
else
    page_size=$1 
    record_size=$2 
    read_mode=$3 
    scan_size=$4 
fi 

dir=${record_size}_${page_size}
cd $dir
echo $dir
g++  -std=c++11 -c DataPage.cc -o DataPage.o 
g++  -std=c++11 -c DirPage.cc -o DirPage.o 
g++  -std=c++11 -c Table.cc -o Table.o 
g++  -std=c++11 -c main.cc -o main.o 
g++  -std=c++11 DataPage.o DirPage.o Table.o main.o -o dbrun
./dbrun $page_size $record_size $read_mode $scan_size
