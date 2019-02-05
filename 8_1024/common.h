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


const uint32_t pagesize=1024;
const uint32_t recordsize=8;
extern uint32_t datapagecount;
extern uint32_t dirpagecount;
struct record{
	uint64_t RID; 
	char record_data[recordsize];
  bool deleted;
};

struct DataPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  int64_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

struct DirPageHeader {
  uint32_t page_size;     // Total size of the page, including header
  uint32_t offset_count;   // Number of Offsets to datapages stored
  int64_t next;          // The offset of next page in the same file (described later)
  uint32_t pageID;        // ID of the page. Used to build ID of the record
};

const uint32_t slots_per_page = (pagesize-sizeof(DataPageHeader))/sizeof(record);
const uint32_t offsets_per_dir = (pagesize-sizeof(DirPageHeader))/sizeof(uint32_t);


