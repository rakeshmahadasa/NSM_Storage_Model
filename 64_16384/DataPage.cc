#include<iostream>
#include<vector>
#include<cstring>
#include<stdio.h>
#include <unistd.h>
#include <sys/stat.h> // open()
#include <fcntl.h> // open flags
#include <chrono>
#include <cstdlib>
#include "DataPage.h"

using namespace std;
uint32_t datapagecount=0;
uint32_t dirpagecount=0;
DataPage::DataPage(){
    header.page_size = pagesize;
    header.record_size = recordsize;
    header.record_count=0;
    header.pageID=datapagecount;
    header.next=-1;
}

bool DataPage::Insert(string input_record){
    if(header.record_count == slots_per_page){
        return false;
    }
    for(int i = 0; i < recordsize;i++){
        records[header.record_count].record_data[i] = input_record[i];
    }
    records[header.record_count].RID = (uint64_t)header.pageID<<32 | header.record_count;
    records[header.record_count].deleted=false;
    header.record_count++;
    return true;
}
bool DataPage::read_page_data(){
    for(int i = 0; i < header.record_count;i++){
        string current_record_data(records[i].record_data);
        cout<<records[i].RID<<" "<<current_record_data<<endl;
    }
}
void DataPage::print_page_info(){
    cout<<"Page ID : "<<header.pageID<<endl;
    cout<<"Page Size : "<<header.page_size<<endl;
    cout << "Record Size : " << header.record_size << endl;
    cout << "Record Count : " << header.record_count << endl;
    read_page_data();
}

bool DataPage::Read(uint64_t RID,char* buff){
    uint32_t slotID = (RID & 0xffffffff);
    if(slotID >= header.record_count) return false;
    if(records[slotID].deleted) return false;
    strcpy(buff,records[slotID].record_data);
    return true;
}

bool DataPage::Delete(uint64_t RID){
    uint32_t slotID = (RID & 0xffffffff);
    if(slotID >= header.record_count) return false;
    if(records[slotID].deleted) return true;
    records[slotID].deleted=true;
    return true;
}