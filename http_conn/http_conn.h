#ifndef HTTP_CONN_H_
#define HTTP_CONN_H_
#include <unistd.h>
#include <netinet/in.h>//包含sockaddr_in
#include <string.h> //包含memset
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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
    static int m_epollfd;
    static int m_user_count;

private:
    int m_sockfd;//套接字
    sockaddr_in m_address;
    char m_read_buff[READ_BUFFER_SIZE];//读缓冲
    char m_write_buff[WRITE_BUFFER_SIZE];//写缓冲
    int m_startline_index;//报文中每一行的开始在read_buff中的位置
    int m_check_index;//从状态机正在检查的字节在read_buff中的位置
    int m_read_index;

    MASTER_STATE m_curstate_master;//主状态机器当前的状态


public:
    http_conn(){}
    ~http_conn(){}
    void init(int sockfd,const sockaddr_in& addr);
    /*工作线程的工作函数*/
    void process();
    /*读取缓冲区*/
    REQUEST_RESULT process_read();
};



#endif