#include <iostream>
#include "myDB.h"
#include <algorithm>// std::random_shuffle
using namespace std;

int main(){
    MyDB& db = MyDB::getInstance();
    db.initDB("localhost","root","123456","myserver");
    db.stateSQL("INSERT shorturls (short_url,url,creat_time,lastModfication_time) values('www.ak.com/asdf','www.baidu.com',now(),now());");
    db.stateSQL("SELECT * from shorturls;");
    string a = "asdfgh";
    a.insert(0, 10, '0');
    cout << a << endl;
    string s = db.m_produceurl.convert_complex(12346);
    cout << s << endl;
    cout << db.m_produceurl.reconvert_complex(s) << endl;
}