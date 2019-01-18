#include<stdio.h>
#include<iostream>
#include <stdlib.h>
#include <sys/stat.h>

using namespace std;

struct record{
    int recordid;
    string recordname;
};
struct stat finfo;

int main(int argc, char const *argv[])
{
    struct record records[3];

    records[0].recordid = 0;
    records[1].recordid = 1;
    records[2].recordid = 2;

    records[0].recordname = "zero";
    records[1].recordname = "one";
    records[2].recordname = "two";

    FILE* table;
    table = fopen(argv[1],"w+b");
    fstat(fileno(table), &finfo);
    cout<<"File Size before writing : "<<finfo.st_size<<endl;

    //for(int i = 0; i < 3;i++){
    fwrite(records,sizeof(records[0]),sizeof(records)/sizeof(records[0]),table);
    //}
    fclose(table);

    table = fopen(argv[1],"r+b");

    fstat(fileno(table), &finfo);
    cout<<"File Size : "<<finfo.st_size<<endl;
    cout<<"Object size : "<<sizeof(records[0])<<endl;
    // allocate memory to contain the whole file:
    struct record* buffer = (struct record*) malloc (sizeof(struct record)*1);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
    fseek(table,sizeof(struct record),SEEK_SET);
    // copy the file into the buffer:
    size_t result = fread (buffer,sizeof(records[0]),1,table);
    if (result != 1) {cerr<<"Reading Error "<<result<<endl; exit (3);}

    cout<<buffer[0].recordid<<" "<<buffer[0].recordname<<endl;
    cout<<buffer[1].recordid<<" "<<buffer[1].recordname<<endl;
    return 0;
}
