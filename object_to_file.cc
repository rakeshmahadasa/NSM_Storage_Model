#include<iostream>
#include<vector>
#include<cstring>
#include<stdio.h>
#include <unistd.h>
#include <sys/stat.h> // open()
#include <fcntl.h> // open flags
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace chrono;

const uint32_t pagesize=4096;
const uint32_t recordsize=256;
uint32_t datapagecount=0;
uint32_t dirpagecount=0;
struct record{
	uint64_t RID; 
	char record_data[recordsize];
};

struct DataPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  int32_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

struct DirPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t offset_count;   // Number of Offsets to datapages stored
  int32_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

const uint32_t slots_per_page = (pagesize-sizeof(DataPageHeader))/sizeof(record);
const uint32_t offsets_per_dir = (pagesize-sizeof(DirPageHeader))/sizeof(uint32_t);
class DataPage{
    public:
        struct DataPageHeader header;
        struct record records[slots_per_page];
        DataPage();
        bool Insert(string input_record);
        bool read_page_data();
        void print_page_info();
        bool Read(uint64_t RID,char* buff);
};
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
    strcpy(buff,records[slotID].record_data);
    return true;
}

class DirPage{
    public:
        struct DirPageHeader header;
        uint32_t data_offsets[offsets_per_dir];
        DirPage();
        bool Insert(size_t offset);
        void print_page_info();
};



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

class Table{
    public:
        FILE* table;
        int32_t latest_dir_offset;
        int32_t latest_data_offset;
        DirPage* current_dir_page;
        DataPage* current_data_page;
        Table();
        bool CreateTable(const char* filename);
        bool Insert(string record);
        bool LazyInsert(string record);
        bool InsertPage(DataPage* P, FILE* table,int32_t offset);
        bool InsertPage(DirPage* P, FILE* table,int32_t offset);
        void ReadTable();
        void ReadPage(DataPage* page_buffer,int32_t offset);
        void ReadPage(DirPage* page_buffer,int32_t offset);
        void CloseTable();
        bool SingleRead(uint64_t RID,char* buff);
        bool SeqRead(uint64_t RID,char* buff,uint32_t scan_szie);
};

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

bool Table::InsertPage(DataPage* P,FILE* table,int32_t offset){
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DataPage),1,table);
    return true;
}

bool Table::InsertPage(DirPage* P,FILE* table,int32_t offset){
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DirPage),1,table);
    return true;
}


void Table::ReadPage(DataPage* page_buffer,int32_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DataPage),1,table);
}

void Table::ReadPage(DirPage* page_buffer,int32_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DirPage),1,table);
}
int get_eof_offset(){
    return (dirpagecount*sizeof(DirPage) + datapagecount*sizeof(DataPage));
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
    int current_dir_offset=0;
    while(true){
        DirPage* current_dir_page = new DirPage();
        ReadPage(current_dir_page,current_dir_offset);
        cout<<"====================DIR==============="<<endl;
        current_dir_page->print_page_info();
        cout<<"======================================"<<endl;
        for(int i = 0; i < current_dir_page->header.offset_count;i++){
            int current_data_offset = current_dir_page->data_offsets[i];
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
    
    uint32_t slotID = (RID & 0xffffffff);
    uint32_t pageID = (RID >> 32); 
    fseek(table,0,SEEK_SET);
    //cout<<"PageID : "<<pageID<<" Slot ID: "<<slotID<<endl;
    uint32_t current_dir_offset=0;
    while(true){
        DirPage* current_dir_page = new DirPage();
        ReadPage(current_dir_page,current_dir_offset);
        for(int i = 0; i < current_dir_page->header.offset_count;i++){
            uint32_t current_data_offset = current_dir_page->data_offsets[i];
            DataPage* current_data_page = new DataPage();
            ReadPage(current_data_page,current_data_offset);
            if(current_data_page->header.pageID == pageID){
                if(current_data_page->Read(RID,buff)){
                    cout<<"Page ID Now : "<<current_data_page->header.pageID<<endl;
                    return true;
                }
                return false;
            }
            
        }
        current_dir_offset = current_dir_page->header.next;
        if(current_dir_offset == -1){

            cout<<current_dir_page->header.pageID<<" End of Reading DB"<<endl;
            break;
        }
    }
}
bool Table::SeqRead(uint64_t RID,char* buff,uint32_t scan_size){
    
    uint32_t slotID = (RID & 0xffffffff);
    uint32_t pageID = (RID >> 32); 
    fseek(table,0,SEEK_SET);
    cout<<"PageID : "<<pageID<<" Slot ID: "<<slotID<<endl;
    uint32_t current_dir_offset=0;
    while(true){
        DirPage* current_dir_page = new DirPage();
        ReadPage(current_dir_page,current_dir_offset);
        for(int i = 0; i < current_dir_page->header.offset_count;i++){
            uint32_t current_data_offset = current_dir_page->data_offsets[i];
            DataPage* current_data_page = new DataPage();
            ReadPage(current_data_page,current_data_offset);
            if(current_data_page->header.pageID == pageID){
                
                for(int i = 0; i < scan_size;i++){
                    if(!current_data_page->Read(slotID , buff)){
                        if(current_data_page->header.next == -1) return true;
                        ReadPage(current_data_page,current_data_page->header.next);
                        slotID=0;
                        current_data_page->Read(slotID , buff);
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
                return true;            
            }
        }
        current_dir_offset = current_dir_page->header.next;
        if(current_dir_offset == -1){

            cout<<current_dir_page->header.pageID<<" End of Reading DB"<<endl;
            break;
        }
    }
}


uint64_t randomRIDgenerator(){

    uint32_t random_pageID = rand()%datapagecount; 
    uint32_t random_slotID = rand()%slots_per_page;
    return((uint64_t)random_pageID<<32 | random_slotID);

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
    for(int j = 0; j < 20000;j++){
        if (!db.LazyInsert(to_string(j)))
        {
            cout << "Error writing to page. slots full" << endl;
            exit(1);
        }
    }
    db.InsertPage(db.current_dir_page,db.table,db.latest_dir_offset);
    db.InsertPage(db.current_data_page,db.table,db.latest_data_offset);
    high_resolution_clock::time_point insert_end_time = high_resolution_clock::now();
    cout<<"Inserting Data Completed"<<endl;
    cout<<"Total Data Pages : "<<datapagecount<<endl;
    cout<<"Total Dir Pages : "<<dirpagecount<<endl;
    cout<<"Total slots per data page : "<<slots_per_page<<endl;
    cout<<"Total slots per dir page : "<<offsets_per_dir<<endl;
    cout<<"Insert time taken : "<<duration_cast<microseconds>(insert_end_time - insert_start_time).count()/1000000.0<<endl;

    if( reader_type == 'r'){
        cout<<"Starting Random Read"<<endl;
        char *result = new char[recordsize];
        high_resolution_clock::time_point random_read_start_time = high_resolution_clock::now();
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
                cout << "NOT FOUND RECORD : RID : " <<RID<< endl;
            }
        }
        high_resolution_clock::time_point random_read_end_time = high_resolution_clock::now();
        cout<<"Random Read Completed"<<endl;
        cout<<"Random Read Scan Size : "<<scan_size<<endl;
        cout<<"Random Read time taken : "<<duration_cast<microseconds>(random_read_end_time - random_read_start_time).count()/1000000.0<<endl;
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