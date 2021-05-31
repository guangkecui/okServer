#include <iostream>
#include "myDB.h"
#include <algorithm>// std::random_shuffle
#include "sqlpool.h"
using namespace std;

int main(){
    ProduceSurl *produceurl = new ProduceSurl();
    MyDB * db = MyDB::getInstance(produceurl);
    MYSQL* mysql=mysql_init(NULL);
    db->initDB(mysql,"localhost","root","123456","myserver");
    // connection_pool* m_sqlpool = connection_pool::GetInstance();
    // m_sqlpool->init("localhost", "root", "123456", "myserver",3306,1);
    // cout << "m_sqlpool:" << m_sqlpool << endl;
    // {
    //     MYSQL *asd = nullptr;
    //     connectionRAII mysqlcon(&asd, m_sqlpool);
    //     db->initID(asd);
    // }
    // string s = db->insertLongUrl(mysql,"www.baidu.com") ;
    // cout << "s = " << s << endl;
    // cout << "id = " << db->m_produceurl->reconvert_complex(s) << endl;
    // string a;
    // while (cin >> a)
    // {
    //     if(a=="q"){
    //         break;
    //     }
    // }
    // s = db->insertLongUrl(mysql,"www.123.com") ;
    // cout << "s = " << s << endl;
    // cout << "id = " << db->m_produceurl->reconvert_complex(s) << endl;
    // s = db->insertLongUrl(mysql,"www.456.com") ;
    string s = "P9rx9yv";
    cout << "s = " << s << endl;
    cout << "id = " << db->m_produceurl->reconvert_complex(s) << endl;
    return 0;
}