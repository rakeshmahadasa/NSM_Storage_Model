#!/bin/bash 
function print_info {
    echo "Usage:"
    echo "--------------------------------------------------------" 
    echo "ksh run.ksh <record_count> <page_size> <record_size> <read mode> <scan size>"
    echo "record count : any number of records can be inserted depending on memory availability"
    echo "page size options : 1024,4096,16384"
    echo "record size options : 8,64,256"
    echo "read mode options : r for random , s for sequential reads"
    echo "scan size options : 10,100,1000"
    echo "--------------------------------------------------------" 
}
if [[ $# -lt 5 ]]; then 
    echo "Less Number of Arguments provided !!!!" 
    print_info
    exit 1 
else
    record_count=$1
    page_size=$2
    record_size=$3 
    read_mode=$4
    scan_size=$5 
fi 

dir=${record_size}_${page_size}
cd $dir
echo $dir
g++  -std=c++11 -c DataPage.cc -o DataPage.o 
g++  -std=c++11 -c DirPage.cc -o DirPage.o 
g++  -std=c++11 -c Table.cc -o Table.o 
g++  -std=c++11 -c main.cc -o main.o 
g++  -std=c++11 DataPage.o DirPage.o Table.o main.o -o dbrun
./dbrun $page_size $record_size $read_mode $scan_size $record_count
