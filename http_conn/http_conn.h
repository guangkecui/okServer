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
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/uio.h>
#include <unordered_map>
#include "../timer/timermanage.h"
using std::string;
class timerNode;
class http_conn
{
public:
    /*读缓存区的大小*/
    static const int READ_BUFFER_SIZE = 2048;
    /*写缓存区的大小*/
    static const int WRITE_BUFFER_SIZE = 10240;
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
        FILE_REQUEST,//请求获取一个文件
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
    int m_read_index;//指向read_buff中有效数据的尾字节的下一字节

    int m_master_state; //主状态机的状态
    MASTER_STATE m_curstate_master;//主状态机器当前的状态

    /*请求行信息*/
    char *m_url; //url
    REQUEST_METHOD m_method;//请求方法
    char *m_version;//http版本号
    /*请求头信息*/
    int m_content_length;//请求体的长度，GET方法为0，POST不为0
    int m_keep_alive;//是否保持连结
    char *m_host;//主机地址
    char *m_string;//存储请求体数据

    /*请求的目标文件的全路径*/
    string m_targetfile_path;
    /*目标文件的状态，是否存在，是否为目录，是否可读，文件大小等*/
    struct stat m_file_stat;
    /*mmap返回的文件与内存映射的内存起始地址*/
    char *m_file_address;
    int m_write_index;/*指向write_buff中有效字符，即还未向buff中填写的第一个位置*/
    /*向量表：iv[0].base:writebuff起始地址；
            iv[0].len:writebuff长度；
            iv[1].base:文件映射的内存起始地址；
            iv[1].len:文件映射的内存长度；*/
    struct iovec m_iv[2];
    int m_iv_count;
    /*将要发送的字节个数*/
    int bytes_to_send;
    /*已经发送的字节个数*/
    int bytes_have_send;
    std::unordered_map<string, string> m_name_password;
    /*绑定的定时器*/
    timerNode *m_timer;                                                                               

public:
    static void setnoblock(int fd);
    static void addfd(int epollfd, int fd, int isShot = false,bool ET = false);
    static void removefd(int epollfd, int fd);
    static void modfd(int epollfd, int fd, int old_option);
public:
    http_conn(){}
    ~http_conn(){}
    /*http类初始化函数*/
    void init(int sockfd,const sockaddr_in& addr);
    /*http类关闭函数*/
    void http_close();
    /*工作线程的工作函数*/
    void process();
    
    /*读取缓冲区*/
    REQUEST_RESULT process_read();
    /*工作线程每次执行process之前，都要执行read_once，
    即读取缓冲区内的数据*/
    bool read_once();
    /*向TCP缓冲区中写数据*/
    bool write();
    /*向http类绑定定时器*/
    void linktimer(timerNode *timer);
    /*定时器解绑*/
    void unlinktimer();
    /*获得定时器*/
    timerNode *get_timer() const { return m_timer; }
    /*获得socket*/
    int get_socket() const { return m_sockfd; }

private:
    /*主状态机解析请求行*/
    REQUEST_RESULT master_parse_line(char* text);
    /*主状态机解析头部*/
    REQUEST_RESULT master_parse_header(char* text);
    /*主状态机解析请求体*/
    REQUEST_RESULT master_parse_body(char* text);
    /*主状态机对完整的请求作出响应*/
    REQUEST_RESULT do_request();
    /*从状态机解析一行*/
    SLAVE_STATE slave_parse_line();
    /*取消mmap的内存区映射*/
    void unmap();
    /*根据process_read返回的状态码，决定是否向write——buff中写数据*/
    bool process_write(REQUEST_RESULT read_ret);
    /*向http响应报文中添加状态码和状态描述*/
    bool add_status_line(int status_code, const string &status_discrip);
    /*向http响应报文中添加头部信息*/
    bool add_header(int content_len);
    /*向http响应报文中添加响应体*/
    bool add_body(const char* body);
    /*向http响应报文中添加响应*/
    bool add_response(const char* format, ...);
    /*向http响应报文中添加空行*/
    bool add_blankline();
    
    /*私有初始化函数*/
    void init();
    /*cgi处理程序,若用户名存在-密码对返回true
            用户名存在-密码错误返回false
            用户名不存在-且为登陆，返回false
            用户名不存在-且为注册，返回true
            @state:注册：1；登陆：2；*/
    void process_cgi(string &name, string &password);
    /*判断用户名-密码是否有效*/
    bool user_is_valid(int state,const string& name,const string& password);

};

#endif