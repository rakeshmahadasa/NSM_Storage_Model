#include "DataPage.h"

class DirPage{
    public:
        struct DirPageHeader header;
        uint64_t data_offsets[offsets_per_dir];
        DirPage();
        bool Insert(size_t offset);
        void print_page_info();
};
