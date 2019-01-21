#include<iostream>
#include<vector>
#include<cstring>
#include<stdio.h>
#include <unistd.h>

using namespace std;

const uint32_t pagesize=1024;
const uint32_t recordsize=8;
uint32_t pagecount=0;
struct record{
	uint64_t RID; 
	char record_data[recordsize];
};

struct DataPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  uint32_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

struct DirPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t offset_count;   // Number of Offsets to datapages stored
  uint32_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

const uint32_t slots_per_page = (pagesize-sizeof(DataPageHeader))/sizeof(record);
const uint32_t offsets_per_dir = (pagesize-sizeof(DataPageHeader))/sizeof(uint32_t);
class DataPage{
    public:
        struct DataPageHeader header;
        struct record records[slots_per_page];
        DataPage();
        bool Insert(string input_record);
        bool read_page_data();
        void print_page_info();
};
DataPage::DataPage(){
    header.page_size = pagesize;
    header.record_size = recordsize;
    header.record_count=0;
    header.pageID=pagecount;
    header.next=-1;
}

bool DataPage::Insert(string input_record){
    if(header.record_count == slots_per_page){
        return false;
    }
    for(int i = 0; i < recordsize;i++){
        records[header.record_count].record_data[i] = input_record[i];
    }
    records[header.record_count].RID = header.record_count;
    header.record_count++;
    cout<<"Record : "<<input_record<<" "<<"Slot ID : "<<header.record_count<<endl;
    return true;
}
bool DataPage::read_page_data(){
    for(int i = 0; i < header.record_count;i++){
        string current_record_data(records[i].record_data);
        cout<<records[i].RID<<" "<<current_record_data<<" "<<current_record_data.length()<<endl;
    }
}
void DataPage::print_page_info(){
    cout<<"Page ID : "<<header.pageID<<endl;
    cout<<"Page Size : "<<header.page_size<<endl;
    cout << "Record Size : " << header.record_size << endl;
    cout << "Record Count : " << header.record_count << endl;
    read_page_data();
}


class DirPage{
    public:
        struct DirPageHeader header;
        uint32_t data_offsets[offsets_per_dir];
        DirPage();
        bool Insert(size_t offset);
};

DirPage::DirPage(){
    header.page_size = pagesize;
    header.offset_count=0;
    header.pageID=pagecount;
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

class Table{
    public:
        FILE* table;
        size_t latest_dir_offset;
        size_t latest_data_offset;
        Table();
        bool CreateTable(const char* filename);
        bool Insert(string record);
        bool InsertPage(DataPage* P, FILE* table,uint32_t offset);
        bool InsertPage(DirPage* P, FILE* table,uint32_t offset);
        void ReadTable();
        void ReadPage(DataPage* page_buffer,size_t offset);
        void ReadPage(DirPage* page_buffer,size_t offset);
        

};

Table::Table(){
    latest_dir_offset=0;
    latest_data_offset=-1;
}

bool Table::CreateTable(const char* filename){
    table = fopen(filename,"w+");
	if(table == NULL){
		cerr<<"Table creation Failed. Unable to open file"<<endl;
		exit(2);
	}
    DirPage* first_dir_page = new DirPage();
    fwrite(first_dir_page,sizeof(DirPage),1,table);
    fsync(fileno(table));
    pagecount++;
    return true;
}

bool Table::InsertPage(DataPage* P,FILE* table,uint32_t offset){
    P->header.pageID=pagecount;
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DataPage),1,table);
    pagecount++;
    return true;
}

bool Table::InsertPage(DirPage* P,FILE* table,uint32_t offset){
    P->header.pageID=pagecount;
    fseek(table, offset, SEEK_SET);
    fwrite(P,sizeof(DataPage),1,table);
    return true;
}


void Table::ReadPage(DataPage* page_buffer,size_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DataPage),1,table);
}

void Table::ReadPage(DirPage* page_buffer,size_t offset){
    fseek(table,offset,SEEK_SET);
    fread(page_buffer,sizeof(DirPage),1,table);
}

bool Table::Insert(string record){
    if(latest_data_offset == -1){
        DirPage* first_dir_page = new DirPage();
        ReadPage(first_dir_page,latest_dir_offset);
        first_dir_page->Insert(lseek(fileno(table),0,SEEK_END));
        DataPage P;
        P.Insert(record);
        InsertPage(first_dir_page,table,0);        
        latest_data_offset=lseek(fileno(table),0,SEEK_END);
        InsertPage(&P,table,lseek(fileno(table),0,SEEK_END));
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
            if(!old_dir_page->Insert(lseek(fileno(table),0,SEEK_END))){
                old_dir_page->header.next=lseek(fileno(table),0,SEEK_END);
                InsertPage(old_dir_page,table,latest_dir_offset);
                DirPage* new_dir_page = new DirPage();
                new_dir_page->Insert(lseek(fileno(table),0,SEEK_END)+sizeof(DirPage));
                InsertPage(new_dir_page,table,lseek(fileno(table),0,SEEK_END));
                latest_dir_offset=old_dir_page->header.next;
            }
            new_data_page->Insert(record);
            old_data_page->header.next = lseek(fileno(table),0,SEEK_END);
            InsertPage(old_data_page,table,latest_data_offset);
            InsertPage(new_data_page,table,old_data_page->header.next);
            latest_data_offset=old_data_page->header.next;
            return true;
        }
    }
    return false;
}

void Table::ReadTable(){
    size_t offset=sizeof(DirPage);
    size_t eof = lseek(fileno(table),0,SEEK_END);
    cout<<"End of file : "<<eof<<endl;
    fseek(table,offset,SEEK_SET);
    DataPage* current_page = new DataPage();
    uint32_t current_page_offset=0;
    while(current_page_offset!=-1){
        fread(current_page,sizeof(DataPage),1,table);
        current_page->print_page_info();
        fseek(table,current_page->header.next,SEEK_SET);
        current_page_offset=current_page->header.next;
    }
}

int main()
{
    Table db;
    db.CreateTable("init.bin");
    for (int i = 0; i < slots_per_page; i++)
    {
        string name = "RakeshM";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }
    for (int i = 0; i < slots_per_page; i++)
    {   
        string name="Keerthi";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }
    
    for (int i = 0; i < slots_per_page; i++)
    {   
        string name="Madhuli";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }

    for (int i = 0; i < slots_per_page; i++)
    {   
        string name="SushmaM";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }

   /* for (int i = 0; i < slots_per_page; i++)
    {   
        string name="PrasadM";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }
    
    for (int i = 0; i < slots_per_page; i++)
    {   
        string name="GangaMa";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }

    for (int i = 0; i < slots_per_page; i++)
    {   
        string name="Suhasin";
        if (!db.Insert(name))
        {
            cout << "Error writing record to page" << endl;
            exit(2);
        }
    }
*/
    db.ReadTable();

    return 0;
}