#include "myDB.h"
#include <algorithm>// std::random_shuffle
#include <math.h>
#include <sstream>
ProduceSurl::ProduceSurl(){
    codeLength = 6;//62^6 > 11位
    produceKeys(); //打乱字符串
    string sixtwo_s = "";//最大的62进制数
    for (int i = 0; i < codeLength;++i){
        sixtwo_s.push_back(SeqKey[SeqKey.size() - 1]);
    }
    long num = reconvert(sixtwo_s);//转化成10进制数
    maxLength = to_string(num).size();//10进制数的位数
    cout << SeqKey << endl;
}
/*随机打乱字符串*/
void ProduceSurl::produceKeys(){
    string seqKey = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
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
    long x = id / 62;
    return convert(x) + SeqKey[y];
}
/*62进制转10进制，单纯转换*/
long ProduceSurl::reconvert(string num){
    long val = 0;
    int len = num.length();
    for (int i = len - 1; i >= 0;--i){
        int t = SeqKey.find(num[i]);
        int s = (len - i) - 1;
        long m = (power(62, s) * t);
        val += m;
    }
    return val;
}
/*10进制转62进制，复杂转换*/
string ProduceSurl::convert_complex(long id){
    string id_s = to_string(id);//将long转换成string
    if(id_s.length()>maxLength){
        cout << "Error:id's length is" << id_s.length() << ",too max" << endl;
        exit(1);
    }
    if(maxLength - id_s.length()>0){
        id_s.insert(0, maxLength - id_s.length(),'0' );//将id_s的前面补0，补成11位；
    }
    reverse(id_s.begin(), id_s.end()); //翻转
    long newid = stol(id_s);//string转long
    string six = convert(newid);//10进制转62进制
    if (codeLength > six.size())
    {
        six.insert(0, codeLength - six.size(), SeqKey.front());//62进制前面补0，补成6位
    }
    return six;
}
/*62进制转10进制，复杂转换*/
long ProduceSurl::reconvert_complex(string num){
    if(num.length()>codeLength+1){
        cout << "Error:num's length is" << num.length() << ",too max" << endl;
        exit(1);
    }
    long ten_num = reconvert(num);//62进制转换成10进制
    string ten_s = to_string(ten_num); //10进制数转换成字符串
    if(maxLength - ten_s.length()>0){
        ten_s.insert(0, maxLength - ten_s.length(),'0');//将ten_s的前面补0，补成11位；
    }
    reverse(ten_s.begin(), ten_s.end());//翻转
    long newten = stol(ten_s);//string转long
    return newten;
}

long ProduceSurl::stol(string str)
{
    long result;
    istringstream is(str);
    is >> result;
    return result;
}

long ProduceSurl::power(int x,int n){
    long res = 1;
    for (int i = 0; i < n; ++i)
    {
        res *= x;
    }
    return res;
}

MyDB::MyDB(ProduceSurl* produceurl):m_produceurl(produceurl){
    m_id = 0;
}

MyDB::~MyDB(){
    if(m_produceurl!=nullptr){
        delete m_produceurl;
        m_produceurl = nullptr;
    }
}

MyDB* MyDB::getInstance(ProduceSurl* produceurl){
    static MyDB local_db(produceurl);
    return &local_db;
}

bool MyDB::initDB(MYSQL* mysql,string host, string user, string pwd, string db_name){
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

MYSQL_RES* MyDB::stateSQL(MYSQL* mysql,string sql){
    MYSQL_RES *result = nullptr;
    if (mysql_query(mysql,sql.c_str()))
    {
        cout<<"Query Error: "<<mysql_error(mysql);
        return nullptr;
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
                return nullptr;
            }
        }
    }

    return result;
}
long MyDB::lastId(){
    long ret;
    m_lock.lock();
    m_id++;
    ret = m_id;
    m_lock.unlock();
    return ret;
}

string MyDB::insertLongUrl(MYSQL* mysql,string url){
    long lastid = lastId();
    cout << "lasrid = " << lastid << endl;
    string short_url = m_produceurl->convert_complex(lastid);
    string lastid_s = to_string(lastid);
    string state = "INSERT INTO shorturls (short_url,url,creat_time,lastModfication_time) VALUES('"+
                    //lastid_s+ ",'" +
                   short_url + "','" +
                   url + "'," +
                   "now(),now());";
    stateSQL(mysql, state);
    return short_url;
}

string MyDB::getLongUrl(MYSQL* mysql,string short_url){
    string ret = "";
    long id = m_produceurl->reconvert_complex(short_url);
    string id_s = to_string(id);
    string state = "SELECT url FROM shorturls WHERE id = " + id_s + ";";
    MYSQL_RES *result = stateSQL(mysql, state);
    if (result==nullptr)
    {
        cout << "Error:short_url: "<<short_url<<" has't been inserted in mysql;" << endl;
        return ret;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    ret = row[0];
    return ret;
}

bool MyDB::initID(MYSQL *mysql){
    MYSQL_RES *result  = stateSQL(mysql,"SELECT Max(id) from shorturls;");//获取自增ID，存储在result中
    if (result != nullptr && mysql_num_rows(result) == 1 && mysql_num_fields(result) == 1)
    {
        MYSQL_ROW row = mysql_fetch_row(result); //该方法用于获取刚刚插入数据的id,赋值给x
        m_id = stol(row[0]);
        return true;
    }
    return false;
}
