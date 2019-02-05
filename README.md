# NSM_Storage_Model
In this Project, we are going to experiment with NSM-based row-store storage model. For simplicity, we assume fixed-length records and fields.

# Steps to Run
    1. Clone / Download the repository. Unzip the folder
    2. cd into the folder
    3. start the storage engine using following command
        ksh run.ksh <record_count> <page_size> <record_size> <read mode> <scan size>
        record count : any number of records can be inserted depending on memory availability. Try with > 10000000
        page size options : 1024,4096,16384
        record size options : 8,64,256
        read mode options : r for random , s for sequential reads
        scan size options : 10,100,1000
    4. Example Command:
        bash run.sh 20000000 4096 256 r 1000
        for 20 million records with size of 256 bytes and page size of 4096 bytes. we read the records in random order and scan size is 1000 here.
    5. The setup uses c++11 

# Supported Operations
    1. Insert 
        - Based on given records couunt , we insert the records in the file created in data folder
    2. Read
        - Random read and  Sequential reads are supported for given scan size ( 10, 100 , 1000 etc)
    3. Delete
        - Each records contains a delete flag which will be set as true if the record is deleted
    4. Close
        - After all the above operations the file will be closed and the program will exit
    All the operations are bundled into a single file so when you start the run.sh script all the above operations will be done sequentially and intermediate information will be printed such as RID's etc

# Implementation Details
    1. Insert
        - We start by creating the data page and directory page (dir page)
        - We keep on adding the records to the data page and when its full we write the data page to the file
        - While writing we will check if the directory page can accommodate new data pages.
        - If dir page is full, we will create a new one and write the old one to file and then write the old data page and create a new data page for further record inserts
    
    2. Read
        - We calculate Page ID and slotID based on given RID
        - We load the first directory page into the memory
        - We keep on loading the data pages based on the offsets stored in the dir page
        - If the current page ID matches the required one then we read the record from that page
        - We do checks like if page ID/ Slot ID is valid and make sure we return the record only if it exists and not deleted before
    3. Delete
        - This is similar to read
        - But once we find the record, we set the deleted flag of the record to true and write the updated page to the system.
    4. Close
        - We close the file
