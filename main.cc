#include<iostream>
#include<vector>
#include<string.h>
#include<stdio.h>
#include <unistd.h>

using namespace std;

#define recordsize 8
#define pagesize 1024

// Boolean flag to know if all data pages are full. 
//New page will be created for next insert opration
bool is_page_full = true;
						

int slot_count = 0;
struct record{
	uint64_t RID; // Record ID of the record = PageID + SlotID
	char* record_data; // Record Data
};



struct PageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  uint32_t next;       // The offset of next page in the same file (described later)
};

//size of Page Header is 24 bytes after Padding 
//if Page size is 1024 bytes we are left with 1000 bytes for record storage
//if record size if 8 bytes then each page can store 125 records

class Page{
	public:
        struct PageHeader header;
        vector<struct record> records;
		vector<int> dir_offset;
		int max_data_slots;
		int max_offset_slots;
		/* Create a directory page with specified page size */
		Page(uint32_t page_size);
		/* Create a data page with specified page size and record size */
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
		
		/*
		*	Calculate next offset for directory page
		*	+ve = Offset for next directory page
		*	-ve = Offset for next data page
		*/
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
	max_data_slots = (page_size - sizeof(header)) / sizeof(record);
	max_offset_slots = -1;

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
int Page::get_dir_offset(){
	return header.next;
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


int Table::CreateTable(const char *filename){
	strcpy(file_name,filename); //Set current table name
	table = fopen(filename,"w+b"); //Open the file in write/Update, binary Mode
	//Verify if opening file is correct. New file will be created if file doesnt exist.
	//If file already exists, data will be overwritten
	if(table == NULL){
		cerr<<"Table creation Failed. Unable to open file"<<endl;
		exit(2);
	}
	else{
		Page* directory_page = CreatePage(pagesize);
		fseek(table,0,SEEK_SET); //Set the offset to beginning of the file.
		fwrite(directory_page,sizeof(directory_page),1,table);
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
	// If pages are full, create a new page
	if(is_page_full){
		//Check if your directory page has space to hold the offset of new page
		Page *current_directory = set_directory_page(table);
		int current_offset = current_directory->get_dir_offset();
		// if next is positive, then current directory has space to hold offsets for new pages
		if (current_offset > 0)
		{
			Page *data_page = CreatePage(pagesize, recordsize);
			if(current_directory->dir_offset.size()==current_directory->max_offset_slots){
				Page *new_directory_page = CreatePage(pagesize);
				new_directory_page->dir_offset
				fwrite(data_page,sizeof(data_page),1,table);	
			}
			current_directory->dir_offset.push_back(current_offset+sizeof(Page));
			fwrite(data_page,sizeof(data_page),1,table);
		}
		else
		{
		}


	}
};
/* Create a data page with specified page size and record size */
Page *CreatePage(uint32_t page_size, uint32_t record_size){
	return(new Page(page_size,record_size));
}

Page *set_directory_page(FILE* table){
	//read the first directory page
	uint32_t current_offset=0;
	while(true){
		fseek(table,current_offset,SEEK_SET);
		Page* current_directory = new Page(pagesize);
		size_t input_size = fread(current_directory,sizeof(Page),1,table);
		if(input_size != 1){
			cerr<<"Failed to read directory page"<<endl;
			exit(2);
		}
		else{
			int offset = current_directory->get_dir_offset();
			// if next is positive, then current directory has space to hold offsets for new pages
			if( offset > 0){
				return current_directory;
			}
			else{
				current_offset = -1*offset;
			}
		}
	}
}
/* Create a direcotry page with specified page size and record size */
Page *CreatePage(uint32_t page_size){
	return(new Page(page_size));
}
int main(){
    cout<<sizeof(struct PageHeader)<<endl;
    return 0;
}