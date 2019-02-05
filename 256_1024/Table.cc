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

long long int get_eof_offset(){
    return (dirpagecount*sizeof(DirPage) + datapagecount*sizeof(DataPage));
}

Table::Table(){
    latest_dir_offset=0;
    latest_data_offset=-1;
    current_dir_page = new DirPage;
    current_data_page = new DataPage;
}

bool Table::CreateTable(const char* filename){
    table = fopen(filename,"wb+");
	if(table == NULL){
		cerr<<"Table creation Failed. Unable to open file"<<endl;
		exit(2);
	}
    DirPage* first_dir_page = new DirPage();
    fseek(table,0,SEEK_SET);
    fwrite(first_dir_page,sizeof(DirPage),1,table);
    fsync(fileno(table));
    dirpagecount++;
    return true;
}

bool Table::InsertPage(DataPage* P,FILE* table,int64_t offset){
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DataPage),1,table);
    return true;
}

bool Table::InsertPage(DirPage* P,FILE* table,int64_t offset){
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DirPage),1,table);
    return true;
}


void Table::ReadPage(DataPage* page_buffer,int64_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DataPage),1,table);
}

void Table::ReadPage(DirPage* page_buffer,int64_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DirPage),1,table);
}
bool Table::Insert(string record){
    if(latest_data_offset == -1){
        DirPage* first_dir_page = new DirPage();
        ReadPage(first_dir_page,latest_dir_offset);
        first_dir_page->Insert(get_eof_offset());
        DataPage* P = new DataPage();
        P->Insert(record);
        InsertPage(first_dir_page,table,0);        
        latest_data_offset=get_eof_offset();
        InsertPage(P,table,latest_data_offset);
        datapagecount++;
        return true;
    }
    else{
        DataPage* old_data_page = new DataPage();
        ReadPage(old_data_page,latest_data_offset);
        if(old_data_page->Insert(record)){
            InsertPage(old_data_page,table,latest_data_offset);
            return true;
        }
        else{

            DataPage* new_data_page = new DataPage();
            DirPage* old_dir_page = new DirPage();
            ReadPage(old_dir_page,latest_dir_offset);
            if(!old_dir_page->Insert(get_eof_offset())){
                old_dir_page->header.next=get_eof_offset();
                InsertPage(old_dir_page,table,latest_dir_offset);
                DirPage* new_dir_page = new DirPage();
                new_dir_page->Insert(get_eof_offset()+sizeof(DirPage));
                InsertPage(new_dir_page,table,get_eof_offset());
                dirpagecount++;
                latest_dir_offset=old_dir_page->header.next;
            }
            else{
                InsertPage(old_dir_page,table,latest_dir_offset);
            }

            new_data_page->Insert(record);
            old_data_page->header.next = get_eof_offset();
            InsertPage(old_data_page,table,latest_data_offset);
            InsertPage(new_data_page,table,old_data_page->header.next);
            latest_data_offset=old_data_page->header.next;
            datapagecount++;
            return true;
        }
    }
    return false;
}

bool Table::LazyInsert(string record){
    if(latest_data_offset == -1){
        
        InsertPage(current_dir_page,table,0);        
        current_dir_page->Insert(get_eof_offset());
        current_data_page->Insert(record);
        latest_data_offset=get_eof_offset();
        InsertPage(current_data_page,table,latest_data_offset);
        fflush(table);
        datapagecount++;
        return true;

    }
    else{
        if(current_data_page->Insert(record)){
            return true;
        }
        else{

            if(!current_dir_page->Insert(get_eof_offset())){

                current_dir_page->header.next=get_eof_offset();
                InsertPage(current_dir_page,table,latest_dir_offset);
                latest_dir_offset=current_dir_page->header.next;
                delete current_dir_page;
                current_dir_page = new DirPage;
                current_dir_page->Insert(latest_dir_offset+sizeof(DirPage));
                InsertPage(current_dir_page,table,latest_dir_offset);        
                dirpagecount++;
            }

            current_data_page->header.next = get_eof_offset();
            InsertPage(current_data_page, table, latest_data_offset);
            latest_data_offset=current_data_page->header.next;
            delete current_data_page;
            current_data_page = new DataPage;
            current_data_page->Insert(record);
            InsertPage(current_data_page, table,latest_data_offset);
            datapagecount++;
            fflush(table);
            return true;
        }
    }
    return false;
}


void Table::ReadTable(){
    fseek(table,0,SEEK_SET);
    long long int current_dir_offset=0;
    while(true){
        DirPage* current_dir_page = new DirPage();
        ReadPage(current_dir_page,current_dir_offset);
        cout<<"====================DIR==============="<<endl;
        current_dir_page->print_page_info();
        cout<<"======================================"<<endl;
        for(int i = 0; i < current_dir_page->header.offset_count;i++){
            long long int current_data_offset = current_dir_page->data_offsets[i];
            DataPage* current_data_page = new DataPage();
            ReadPage(current_data_page,current_data_offset);
            current_data_page->print_page_info();
        }
        current_dir_offset = current_dir_page->header.next;
        if(current_dir_offset == -1){
            cout<<"End of Reading DB"<<endl;
            break;
        }
    }
}

void Table::CloseTable(){
    fclose(table);
}

bool Table::SingleRead(uint64_t RID,char* buff){
    DirPage* dir_page = new DirPage();
    DataPage* data_page = new DataPage();
    //cout<<"Rand Read "<<RID<<endl;
    uint32_t slotID = (RID & 0xffffffff);
    uint32_t pageID = (RID >> 32); 
    //cout<<"PageID : "<<pageID<<" Slot ID: "<<slotID<<endl;
    uint64_t current_dir_offset=0;
    while(true){
        ReadPage(dir_page,current_dir_offset);
        for(int i = 0; i < dir_page->header.offset_count;i++){
            uint64_t current_data_offset = dir_page->data_offsets[i];
            ReadPage(data_page,current_data_offset);
            if(data_page->header.pageID == pageID){
                if(data_page->Read(RID,buff)){
                    //cout<<"Page ID Now : "<<data_page->header.pageID<<endl;
                    delete dir_page;
                    delete data_page;
                    return true;
                }
                delete dir_page;
                delete data_page;
                return false;
            }
        }
        current_dir_offset = dir_page->header.next;
        if(current_dir_offset == -1){
            //cout<<"Final Offset : "<<dir_page->data_offsets[dir_page->header.offset_count-1]<<endl;
            cout<<dir_page->header.pageID<<" End of Reading DB"<<endl;
            break;
        }
    }
    delete dir_page;
    delete data_page;
    return true;
}
bool Table::SeqRead(uint64_t RID,char* buff,uint32_t scan_size){
    DirPage* dir_page = new DirPage();
    DataPage* data_page = new DataPage();
    uint32_t slotID = (RID & 0xffffffff);
    uint32_t pageID = (RID >> 32); 
    //cout<<"PageID : "<<pageID<<" Slot ID: "<<slotID<<endl;
    uint64_t current_dir_offset=0;
    while(true){
        ReadPage(dir_page,current_dir_offset);
        for(int i = 0; i < dir_page->header.offset_count;i++){
            uint64_t current_data_offset = dir_page->data_offsets[i];
            ReadPage(data_page,current_data_offset);
            if(data_page->header.pageID == pageID){
                
                for(int i = 0; i < scan_size;i++){
                    if(!data_page->Read(slotID , buff)){
                        if(data_page->header.next == -1) return true;
                        ReadPage(data_page,data_page->header.next);
                        slotID=0;
                        data_page->Read(slotID , buff);
                        for (int i = 0; i < recordsize; i++)
                        {
                            cout << buff[i];
                        }
                        cout << endl;
                    }
                    else{
                        for(int i = 0; i < recordsize;i++){
                            cout<<buff[i];
                        }
                        cout<<endl;
                    }
                    slotID++;
                }
                delete dir_page;
                delete data_page;                
                return true;            
            }
        }
        current_dir_offset = dir_page->header.next;
        if(current_dir_offset == -1){

            cout<<dir_page->header.pageID<<" End of Reading DB"<<endl;
            break;
        }
    }
    delete dir_page;
    delete data_page;
    return true;
}

bool Table::Delete(uint64_t RID){
    DirPage* dir_page = new DirPage();
    DataPage* data_page = new DataPage();
    //cout<<"Rand Read "<<RID<<endl;
    uint32_t slotID = (RID & 0xffffffff);
    uint32_t pageID = (RID >> 32); 
    //cout<<"PageID : "<<pageID<<" Slot ID: "<<slotID<<endl;
    uint64_t current_dir_offset=0;
    while(true){
        ReadPage(dir_page,current_dir_offset);
        for(int i = 0; i < dir_page->header.offset_count;i++){
            uint64_t current_data_offset = dir_page->data_offsets[i];
            ReadPage(data_page,current_data_offset);
            if(data_page->header.pageID == pageID){
                if(data_page->Delete(RID)){
                    //cout<<"Page ID Now : "<<data_page->header.pageID<<endl;
                    InsertPage(data_page,table,current_data_offset);
                    delete dir_page;
                    delete data_page;
                    return true;
                }
                delete dir_page;
                delete data_page;
                return false;
            }
        }
        current_dir_offset = dir_page->header.next;
        if(current_dir_offset == -1){
            //cout<<"Final Offset : "<<dir_page->data_offsets[dir_page->header.offset_count-1]<<endl;
            cout<<dir_page->header.pageID<<" End of Reading DB"<<endl;
            break;
        }
    }
    delete dir_page;
    delete data_page;
    return true;
}
