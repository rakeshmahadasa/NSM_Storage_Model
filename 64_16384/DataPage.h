#include "common.h"

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
