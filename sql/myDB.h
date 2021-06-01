#ifndef _MYDB_H_
#define _MYDB_H_
#include<iostream>
#include<string>
#include<mysql/mysql.h>
#include "../locker/locker.h"
#include "../hash/MurmurHash3.h"
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


/*数据库操作类*/
class MyDB
{
private:
    //MYSQL *mysql;//链接mysql的指针
    //MYSQL_RES *result;//指向查询结果的指针
    //MYSQL_ROW row;//按行返回查询结果
    MyDB(ProduceSurl* produceurl);
    MyDB(const MyDB &) = delete;
    MyDB& operator=(const MyDB &) = delete;
    ~MyDB();

    long m_id;
    locker m_lock;

public:
    ProduceSurl* m_produceurl;
    /*获取单例*/
    static MyDB* getInstance(ProduceSurl* m_produceurl);
    /*链接mysql
    host:mysql的地址
    db_name:数据库名称*/
    bool initDB(MYSQL* mysql,string host, string user, string pwd, string db_name);
    /*执行sql语句*/
    MYSQL_RES* stateSQL(MYSQL* mysql,string sql);
    /*获取最新的主键id*/
    long lastId();
    /*插入长链接,返回短链接*/
    string insertLongUrl(MYSQL* mysql,string url);
    /*根据短链接，返回长链接*/
    string getLongUrl(MYSQL* mysql,string short_url);
    /*通过长链接的hash值判断长链接是否已经注册
    若已注册则返回短链接
    若未注册，则新注册*/
    string is_insertLongUrl(MYSQL *mysql, string longurl);
    /*初始化id*/
    bool initID(MYSQL *mysql);
    /*将长链接hash为long*/
    long mmhash(string longurl);
};

#endif