#include <iostream>

using namespace std;

int main(){
    uint32_t a,b;
    uint64_t c;
    a= 50;
    b= 20;
    c = (uint64_t)a<<32 | b;
    cout<<c<<endl;
    cout<<(c & 0xffffffff)<<endl;
    cout<<(c >> 32)<<endl;
}