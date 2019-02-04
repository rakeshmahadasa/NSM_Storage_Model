g++ -c DataPage.cpp -o DataPage.o 
g++ -c DirPage.cpp -o DirPage.o 
g++ -c Table.cpp -o Table.o 
g++ -c main.cc -o main.o 
g++ DataPage.o DirPage.o Table.o main.o -o dbrun
