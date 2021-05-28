#include "myDB.h"
#include <algorithm>// std::random_shuffle
#include <math.h>

ProduceSurl::ProduceSurl(){
    codeLength = 6;//62^6 > 11位
    produceKeys(); //打乱字符串
    string sixtwo_s = "";//最大的62进制数
    for (int i = 0; i < codeLength;++i){
        sixtwo_s.push_back(SeqKey[SeqKey.size() - 1]);
    }
    long num = reconvert(sixtwo_s);//转化成10进制数
    maxLength = to_string(num).size();//10进制数的位数
}
/*随机打乱字符串*/
void ProduceSurl::produceKeys(){
    string seqKey = "0123456789abcdefghigklmnopqrstuvwxyzABCDEFGHIGKLMNOPQRSTUVWXYZ";
    std::random_shuffle(seqKey.begin(), seqKey.end());
    SeqKey = seqKey;
}
/*10进制转62进制，单纯转换*/
string ProduceSurl::convert(long id){
    if(id<62){
        char k = SeqKey[(int)id];
        return string(1, k);
    }
    int y = (int)(id % 62);
    int x = id / 62;
    return convert(x) + SeqKey[y];
}
/*62进制转10进制，单纯转换*/
long ProduceSurl::reconvert(string num){
    long val = 0;
    int len = num.length();
    for (int i = len - 1; i >= 0;--i){
        int t = SeqKey.find(num[i]);
        double s = (len - i) - 1;
        long m = (long)(pow(62, s) * t);
        val += m;
    }
    return val;
}
/*10进制转62进制，复杂转换*/
string ProduceSurl::convert_complex(long id){

}
/*62进制转10进制，复杂转换*/
long ProduceSurl::reconvert_complex(string num){

}


MyDB::MyDB(){
    mysql = mysql_init(nullptr);
    if(mysql==nullptr){
        cout << "Error:" << mysql_error(mysql) << endl;
        exit(1);
    }
}

MyDB::~MyDB(){
    if(result!=nullptr){

    }
    if(mysql!=nullptr){
        mysql_close(mysql);
    }

}

MyDB& MyDB::getInstance(){
    static MyDB local_db;
    return local_db;
}

bool MyDB::initDB(string host, string user, string pwd, string db_name){
    // 函数mysql_real_connect建立一个数据库连接  
    // 成功返回MYSQL*连接句柄，失败返回NULL  
    mysql = mysql_real_connect(mysql, host.c_str(), user.c_str(), pwd.c_str(), db_name.c_str(), 0, NULL, 0);  
    if(mysql == nullptr)  
    {  
        cout << "Error: " << mysql_error(mysql);  
        exit(1);  
    }  
    return true; 
}

bool MyDB::stateSQL(string sql){
    if (mysql_query(mysql,sql.c_str()))
    {
        cout<<"Query Error: "<<mysql_error(mysql);
        return false;
    }
    else // 查询成功
    {
        result = mysql_store_result(mysql);  //获取结果集
        // if (result)  // 返回了结果集
        // {
        //    int  num_fields = mysql_num_fields(result);   //获取结果集中总共的字段数，即列数
        //    int  num_rows=mysql_num_rows(result);       //获取结果集中总共的行数
        //    for(int i=0;i<num_rows;i++) //输出每一行
        //     {
        //         //获取下一行数据
        //         // row=mysql_fetch_row(result);
        //         // if(row<0) break;

        //         // for(int j=0;j<num_fields;j++)  //输出每一字段
        //         // {
        //         //     //cout<<row[j]<<"\t\t";
        //         // }
        //         // cout<<endl;
        //     }
        // }
        if(result == nullptr)  // result==NULL
        {
            if(mysql_field_count(mysql) == 0)   //代表执行的是update,insert,delete类的非查询语句
            {
                // (it was not a SELECT)
                int num_rows = mysql_affected_rows(mysql);  //返回update,insert,delete影响的行数
            }
            else // error
            {
                cout<<"Get result error: "<<mysql_error(mysql);
                return false;
            }
        }
    }

    return true;
}
long MyDB::lastId(){
    stateSQL("SELECT LAST_INSERT_ID();");//获取自增ID，存储在result中
    long ret = -1;
    if (result != nullptr && mysql_num_rows(result) == 1 && mysql_num_fields(result) == 1)
    {
        ret = mysql_insert_id(mysql);//该方法用于获取刚刚插入数据的id,赋值给x
        cout << "ret size:" << sizeof(ret) << endl;
    }
    return ret;
}

string MyDB::ten_converto_sixtwo(int id){

}