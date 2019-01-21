#include<iostream>
#include<vector>
#include<string.h>
#include<stdio.h>
#include <unistd.h>

using namespace std;

#define recordsize 8
#define pagesize 1024

bool is_page_full = true;
uint32_t pageCount=0;		

struct record{
	uint64_t RID; 
	string record_data;
};

struct PageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  uint32_t next;       // The offset of next page in the same file (described later)
  uint32_t pageID;
};
class Page{
	public:
        struct PageHeader header;
        vector<struct record> records;
		vector<int> dir_offset;
		int max_data_slots;
		int max_offset_slots;
		Page(uint32_t page_size);
		Page(uint32_t page_size, uint32_t record_size);
	  	bool Insert(const char *record);
	  	string Read(uint64_t rid);
		int get_dir_offset();
};

Page::Page(uint32_t page_size){
	header.page_size = page_size;
    header.record_size = 0;
	header.record_count = 0;
	max_data_slots = -1;
	max_offset_slots = (page_size - sizeof(header)) / sizeof(int);
}

Page::Page(uint32_t page_size, uint32_t record_size){
	header.page_size = page_size;
    header.record_size = record_size;
	header.record_count = 0;
    header.next = -1;
	max_data_slots = (page_size - sizeof(header)) / sizeof(record);
    cout<<"Slots Pet Page : "<<max_data_slots<<endl;
    max_offset_slots = -1;
}
bool Page::Insert(const char *record){
	if(header.record_count == max_data_slots){
		cerr<<"Error : Failed to insert new record to the page. Slots full"<<endl;
		return false;
	}
	if(strlen(record) > header.record_size){
		cerr<<"Error : Failed to insert new record to the page. Given record size greater than allowed"<<endl;
		cerr<<"Input Record : "<<*record<<" Size : "<<strlen(record)<<endl;
		cerr<<"Record Size Allowed : "<<header.record_size<<endl;
		return false;
	}
    header.record_count++;
    struct record new_record;
    std::string input_record(record);
    new_record.record_data = input_record;
    new_record.RID = (uint64_t)pageCount<<32 | header.record_count;
	records.push_back(new_record);
    return true;
}
string Page::Read(uint64_t rid){
	uint32_t slotID = (rid & 0xffffffff);
	if(slotID > header.record_count){
		return NULL;
	}
	else{
		return records[slotID].record_data;
	}
}
int Page::get_dir_offset(){
	return header.next;
}
Page *CreatePage(uint32_t page_size, uint32_t record_size){
    pageCount++;
	return(new Page(page_size,record_size));
}
class Table{

	public:
		char* file_name; // Name of the file 
		FILE *table;
		size_t latest_directory_offset;
		size_t latest_datapage_offset;
		/* Create a table file. Returns a file descriptor for the file created. The
 		* file must be properly persisted on storage (e.g., using fsync).
 		*/
        Table();
	  	int CreateTable(const char *filename);

	  	/* Close a table file. Input parameter is a descriptor to an open file. */
	  	void CloseTable(int fd);

	  	/* Insert a record to a table file. Returns true if succeeded. If there is
 		* no space in existing data pages, the function should create a new page.
 		* in the file.
 		*/
	  	bool Insert(const char *record);
	  	/* Read a record with a given RID. Returns true if the operation succeeded.
 		* The record content should be stored in buf which is provided by the
 		* caller.
 		*/
	  	bool Read(uint64_t RID, char *buf);
};

Table::Table(){
    file_name = new char[20];
}

int Table::CreateTable(const char *filename){
	strcpy(file_name,filename); 
	table = fopen(filename,"w+b");
	if(table == NULL){
		cerr<<"Table creation Failed. Unable to open file"<<endl;
		exit(2);
	}
	else{
		fseek(table,0,SEEK_SET); //Set the offset to beginning of the file.
		fsync(fileno(table)); //Persist the data
		cout<<"Table created"<<endl;
		cout<<"Table Name : "<<file_name<<endl;
	}
	return fileno(table);	
}

void Table::CloseTable(int fid){
	fclose(table);
}

bool Table::Insert(const char *record){
    int current_offset = 0;
    while(true){
        Page* current_page = CreatePage(pagesize,recordsize);
        fseek(table,current_offset,SEEK_SET);
        fread(current_page,sizeof(Page),1,table);
        if(current_page->Insert(record)){
            fwrite(current_page,sizeof(Page),1,table);
            return true;
        }
        else{
            current_offset = current_page->header.next;
            if(current_offset == -1){
                Page* new_page = CreatePage(pagesize,recordsize);
                fseek(table,0,SEEK_END);
                size_t new_offset = ftell(table);
                current_page->header.next = new_offset;
                fseek(table,current_offset,SEEK_SET);
                fwrite(current_page,sizeof(Page),1,table);
                fwrite(new_page,sizeof(Page),1,table);
                current_offset = new_offset;
            }
        }
    }
};

int main(int argc, char const *argv[])
{
    Table db;
    db.CreateTable("database.bin");
    for(int i = 0; i < 50;i++){
        cout<<"Inserting "<<i<<endl;
        db.Insert("KeerthiM");
    }
    return 0;
}
