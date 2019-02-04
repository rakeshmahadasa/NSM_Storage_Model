#include<iostream>
#include<vector>
#include<cstring>
#include<stdio.h>
#include <unistd.h>
#include <sys/stat.h> // open()
#include <fcntl.h> // open flags
#include <chrono>
#include <cstdlib>
#include "Table.h"

using namespace std;
using namespace chrono;



uint64_t randomRIDgenerator(){

    uint32_t random_pageID = rand()%datapagecount; 
    uint32_t random_slotID = rand()%slots_per_page;
    return((uint64_t)random_pageID<<32 | random_slotID);

}

bool randomRead(Table db,uint32_t scan_size){

    char *result = new char[recordsize];
    for (int i = 0; i < scan_size; i++)
    {
        uint64_t RID = randomRIDgenerator();
        if (db.SingleRead(RID, result))
        {
            for (int i = 0; i < recordsize; i++)
            {
                cout << result[i];
            }
            cout << endl;
        }
        else
        {
            cout << "NOT FOUND RECORD : RID : " << RID << endl;
        }
    }
    return true;

}

bool Insert(Table db,uint32_t record_count){
    for (int j = 0; j < record_count; j++)
    {
        if (!db.LazyInsert(to_string(j)))
        {
            cout << "Error writing to page. slots full" << endl;
            exit(1);
        }
    }
    db.InsertPage(db.current_dir_page, db.table, db.latest_dir_offset);
    db.InsertPage(db.current_data_page,db.table,db.latest_data_offset);
    return true;
}

int main(int argc, char const *argv[])
{
    if(argc < 5){
        cerr<<"Insufficient Number of Arguments"<<endl;
        cerr<<"correct command : ./a.out <pagesize | 1024 , 4096 , 16384 > <recordsize | 8 , 64 , 256> <type of read | r or s> <read length | 10 , 100 , 1000"<<endl;
        cerr<<"Example Command : ./a.out 1024 64 r 10"<<endl;
        exit(1);
    }   

    uint32_t pagesize_temp = atoi(argv[1]);
    uint32_t recordsize_temp = atoi(argv[2]);
    char reader_type = *argv[3];
    uint32_t scan_size = atoi(argv[4]);
    Table db;
    db.CreateTable("init.bin");
    high_resolution_clock::time_point insert_start_time = high_resolution_clock::now();
    Insert(db,100000);
    cout<<"Inserting Data Completed"<<endl;
    cout<<"Total Data Pages : "<<datapagecount<<endl;
    cout<<"Total Dir Pages : "<<dirpagecount<<endl;
    cout<<"Total slots per data page : "<<slots_per_page<<endl;
    cout<<"Total slots per dir page : "<<offsets_per_dir<<endl;
    high_resolution_clock::time_point insert_end_time = high_resolution_clock::now();
    cout<<"Insert time taken : "<<duration_cast<microseconds>(insert_end_time - insert_start_time).count()/1000000.0<<endl;
    
    if( reader_type == 'r'){
        cout << "Starting Random Read" << endl;
        high_resolution_clock::time_point random_read_start_time = high_resolution_clock::now();
        randomRead(db,scan_size);
        high_resolution_clock::time_point random_read_end_time = high_resolution_clock::now();
        cout << "Random Read Completed" << endl;
        cout << "Random Read Scan Size : " << scan_size << endl;
        cout << "Random Read time taken : " << duration_cast<microseconds>(random_read_end_time - random_read_start_time).count() / 1000000.0 << endl;
    }

    else{
        cout<<"Starting Sequential Read"<<endl;
        char *result = new char[recordsize];
        high_resolution_clock::time_point sequential_read_start_time = high_resolution_clock::now();
        uint64_t RID = randomRIDgenerator();
        db.SeqRead(RID, result,scan_size);
        high_resolution_clock::time_point sequential_read_end_time = high_resolution_clock::now();
        cout<<"Sequential Read Completed"<<endl;
        cout<<"Sequential Read Scan Size : "<<scan_size<<endl;
        cout<<"Sequential Read time taken : "<<duration_cast<microseconds>(sequential_read_end_time - sequential_read_start_time).count()/1000000.0<<endl;        
    }


    db.CloseTable();
    return 0;    
}