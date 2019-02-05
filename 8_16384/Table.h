#include "DirPage.h"

class Table{
    public:
        FILE* table;
        int64_t latest_dir_offset;
        int64_t latest_data_offset;
        DirPage* current_dir_page;
        DataPage* current_data_page;
        Table();
        bool CreateTable(const char* filename);
        bool Insert(string record);
        bool LazyInsert(string record);
        bool InsertPage(DataPage* P, FILE* table,int64_t offset);
        bool InsertPage(DirPage* P, FILE* table,int64_t offset);
        void ReadTable();
        void ReadPage(DataPage* page_buffer,int64_t offset);
        void ReadPage(DirPage* page_buffer,int64_t offset);
        void CloseTable();
        bool SingleRead(uint64_t RID,char* buff);
        bool SeqRead(uint64_t RID,char* buff,uint32_t scan_szie);
        bool Delete(uint64_t RID);
};

