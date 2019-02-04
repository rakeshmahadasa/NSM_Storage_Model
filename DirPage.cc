#include<iostream>
#include<vector>
#include<cstring>
#include<stdio.h>
#include <unistd.h>
#include <sys/stat.h> // open()
#include <fcntl.h> // open flags
#include <chrono>
#include <cstdlib>
#include "DirPage.h"

using namespace std;

DirPage::DirPage(){
    header.page_size = pagesize;
    header.offset_count=0;
    header.pageID=dirpagecount;
    header.next=-1;
}

bool DirPage::Insert(size_t offset){
    if(header.offset_count == offsets_per_dir){
        return false;
    }
    data_offsets[header.offset_count]=offset;
    header.offset_count++;
    return true;
}

void DirPage::print_page_info(){
    cout<<"Page ID : "<<header.pageID<<endl;
    cout<<"Page Size : "<<header.page_size<<endl;
    cout << "Offset Count : " << header.offset_count<< endl;
    for(int i = 0; i<header.offset_count;i++){
        cout<<data_offsets[i]<<endl;
    }
}
