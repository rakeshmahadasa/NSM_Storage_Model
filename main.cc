#include<iostream>
#include<vector>
#include<string.h>
#include<stdio.h>

using namespace std;

int slot_count = 0;
struct record{
	uint64_t RID; // Record ID of the record = PageID + SlotID
	char* record_data; // Record Data
};

struct PageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  PageHeader *next;       // The next page in the same file (described later)
};

//size of Page Header is 24 bytes after Padding 
//if Page size is 1024 bytes we are left with 1000 bytes for record storage
//if record size if 8 bytes then each page can store 125 records

class Page{
    private:
        struct PageHeader header;
        vector<struct record> records;
	public:
		/* Create a page with specified page size and record size */
		Page(uint32_t page_size, uint32_t record_size);
	  	/* Insert a record. Returns true if the insertion was successful, false
 		* otherwise. There is no need to check for whether the record already
 		* exists in the page (i.e., duplication is allowed).
 		*/
	  	bool Insert(const char *record);
	  	/* Read a record with a given RID. Returns the a pointer to the record,
 		* NULL if it does not exist. Note that the higher order bits in rid refer
 		* to the page ID, and the lower 32 bits refer to the slot ID.
 		*/
	  	const char *Read(uint64_t rid);
};

Page::Page(uint32_t page_size, uint32_t record_size){
	header.page_size = page_size;
    header.record_size = record_size;
	header.record_count = 0;
}
bool Page::Insert(const char *record){
	if(header.record_count == slot_count){
		cerr<<"Error : Failed to insert new record to the page. Slots full"<<endl;
		return false;
	}
	if(strlen(record) > header.record_size){
		cerr<<"Error : Failed to insert new record to the page. Given record size greater than allowed"<<endl;
		cerr<<"Input Record : "<<*record<<" Size : "<<strlen(record)<<endl;
		cerr<<"Record Size Allowed : "<<header.record_size<<endl;
		return false;
	}
	strcpy(records[header.record_count].record_data,record);
	header.record_count++;
}
const char *Page::Read(uint64_t rid){
	uint32_t slotID = (rid & 0xffffffff);
	if(slotID > header.record_count){
		return NULL;
	}
	else{
		return records[slotID].record_data;
	}
}

class Table{
	private:
		string file_name; // Name of the file 
		string file_directory; // Location of the file
		FILE *table;
	public:
	 	Table(string file_directory);
		/* Create a table file. Returns a file descriptor for the file created. The
 		* file must be properly persisted on storage (e.g., using fsync).
 		*/
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

Table::Table(string file_directory){
	this->file_directory = file_directory;
}

int Table::CreateTable(const char *filename){

}

void Table::CloseTable(int fid){

}
/* Create a page with specified page size and record size */
Page *CreatePage(uint32_t page_size, uint32_t record_size){
	return(new Page(page_size,record_size));
}

int main(){
    cout<<sizeof(struct PageHeader)<<endl;
    return 0;
}