#ifndef HTTP_CONN_H_
#define HTTP_CONN_H_

class http_conn
{
public:
    /*读缓存区的大小*/
    static const int READ_BUFFER_SIZE = 2048;
    /*写缓存区的大小*/
    static const int WRITE_BUFFER_SIZE = 1024;
    /*文件名的长度*/
    static const int FILENAME_LEN = 200;
    /*HTTP请求方法*/
    enum REQUEST_METHOD{
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    /*HTTP请求的结果*/
    enum REQUEST_RESULT{
        NO_REQUEST,/*没有请求*/
        GET_REQUEST,/*获得一个请求*/
        BAD_REQUEST,/*错误的请求*/
        NO_RESOURCE,/*没有此资源*/
        FORBIDDEN_REQUEST,/*禁止访问资源*/
        FILE_REQUEST,
        INTERNAL_ERROR,/*服务器内部错误*/
        CLOSE_CONNECTION/*关闭连接*/
    };
    //主状态机返回状态
    enum MASTER_STATE{
        MASTER_STATE_REQUESTLINE = 0,/*请求行,初始状态*/
        MASTER_STATE_HEADER,           /*报头*/
        MASTER_STATE_BODY               /*请求主体*/
    };
    //从状态机器返回状态
    enum SLAVE_STATE{
        SLAVE_STATE_LINEOK = 0,/*获得一个完整行（如请求行或报头的某一行）*/
        SLAVE_STATE_LINEOPEN,/*无法提取一个完整行，继续等待数据输入*/
        SLAVE_STATE_LINEBAD
    };
public:
    http_conn(){}
    ~http_conn(){}
};



#endif