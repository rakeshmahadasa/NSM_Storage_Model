#include<iostream>

using namespace std;

struct PageHeader {
 private:
  uint32_t page_size;     // Total size of the page, including header
  uint32_t record_size;   // Size of each record
  uint32_t record_count;  // Number of records stored in the page
  PageHeader *next;       // The next page in the same file (described later)
};



int main(){
    struct PageHeader header;
    cout<<sizeof(header)<<endl;
    return 0;
}