g++  -std=c++11 -c DataPage.cc -o DataPage.o 
g++  -std=c++11 -c DirPage.cc -o DirPage.o 
g++  -std=c++11 -c Table.cc -o Table.o 
g++  -std=c++11 -c main.cc -o main.o 
g++  -std=c++11 DataPage.o DirPage.o Table.o main.o -o dbrun
