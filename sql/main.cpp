#include <iostream>
#include "myDB.h"
#include <algorithm>// std::random_shuffle
using namespace std;

int main(){
    ProduceSurl *produceurl = new ProduceSurl();
    MyDB &db = MyDB::getInstance(produceurl);
    MyDB &db2 = MyDB::getInstance(produceurl);
    MyDB &db3 = MyDB::getInstance(produceurl);
    db.initDB("localhost","root","123456","myserver");
    db2.initDB("localhost","root","123456","myserver");
    db3.initDB("localhost","root","123456","myserver");
    cout << "db : " << &db << endl;
    cout << "db2 : " << &db2 << endl;
    cout << "db3 : " << &db3 << endl;
    db.getLongUrl("dvXI6v");
    db2.insertLongUrl("www.qinghua.com");
    db3.insertLongUrl("www.beida.com");
    delete produceurl;
    return 0;
}