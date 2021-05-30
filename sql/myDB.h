#ifndef _MYDB_H_
#define _MYDB_H_
#include<iostream>
#include<string>
#include<mysql/mysql.h>

using namespace std;

/*生成短链接类*/
class ProduceSurl
{
private:
    string SeqKey;
    int codeLength;//短码的最大长度
    int maxLength;//id转化成字符串的最大长度，及codeLength个SeqKey[61]转化成数字的最大长度


public:
    ProduceSurl();
    ~ProduceSurl(){}
    void produceKeys();
    /*10进制转62进制，单纯转换*/
    string convert(long id);
    /*62进制转10进制，单纯转换*/
    long reconvert(string num);
    /*10进制转62进制，复杂转换*/
    string convert_complex(long id);
    /*62进制转10进制，复杂转换*/
    long reconvert_complex(string num);

    /*string转long*/
    long stol(string s);
    /*x的n次方*/
    long power(int x, int n);
};

class MyDB
{
private:
    MYSQL *mysql;//链接mysql的指针
    MYSQL_RES *result;//指向查询结果的指针
    MYSQL_ROW row;//按行返回查询结果
    static MyDB myInstance;//单例对象
    MyDB(ProduceSurl* produceurl);
    MyDB(const MyDB &) = delete;
    MyDB& operator=(const MyDB &) = delete;

    ~MyDB();

public:
    ProduceSurl* m_produceurl;
    /*获取单例*/
    static MyDB& getInstance(ProduceSurl* m_produceurl);
    /*链接mysql
    host:mysql的地址
    db_name:数据库名称*/
    bool initDB(string host, string user, string pwd, string db_name);
    /*执行sql语句*/
    bool stateSQL(string sql);
    /*获取最新的主键id*/
    long lastId();
    /*插入长链接,返回短链接*/
    string insertLongUrl(string url);
    /*根据短链接，返回长链接*/
    string getLongUrl(string short_url);
};

#endif