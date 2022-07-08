#ifndef __LOGIN_HPP__
#define __LOGIN_HPP__

#include <arpa/inet.h> //for htons
#include <fcntl.h>
#include <json/json.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <chrono> //for time in logfile
#include <condition_variable>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include<algorithm>

#include "local_msg_type.hpp"
#include "process_msg.hpp"

extern Json::Value json_config;

struct aes_key_item_t {
    unsigned char key[AES_256_KEY_LEN];
    unsigned char iv[AES_256_IV_LEN];
};

struct client_socket_t {
    int socket;
    struct in_addr addr;
    SSL* ssl_fd = nullptr;
    unsigned char time = 0;
    unsigned char tried_time = 0;
    unsigned char is_closed = false;
};

class login {
private:
    //用于与 openssl 库交互的结构体
    static SSL_CTX* ssl_ctx_fd;

    static std::queue<local_msg_type_t>* local_msg_queue;
    static std::mutex* local_msg_queue_mtx;
    static std::condition_variable* local_msg_queue_cv;

    char success_tag;
    //! meanings:
    //!  0: success
    //! -1: open listen socket error
    //! -2: bind listen socket to address error
    //! -4: can't set listen socket to non-blocking mode
    //! -5: can't start listening on listen socket
    //! -6: can't add listen socket to epoll
    //! -7: can't open log file
    //! -8: can't connct to database server
    //! -9: can't open server key file
    //! -10: can't read from server key file

    static sqlite3* db_sqlite;
    static sqlite3_stmt* db_sqlite_stmt_getPassword; //获得用户密码的 sql 对象
    static sqlite3_stmt* db_sqlite_stmt_getUserlist; //获得用户名字列表的 sql 对象

    static struct aes_key_item_t* server_keys_ptr;

    static std::mutex continue_tag_mtx;
    /*volatile提醒编译器它后面所定义的变量随时都有可能改变，
    因此编译后的程序每次需要存储或读取这个变量的时候，
    告诉编译器对该变量不做优化，都会直接从变量内存地址中读取数据，
    从而可以提供对特殊地址的稳定访问。*/
    volatile static bool continue_tag; //是否继续

    static int listen_socket;

    //日志文件
    static std::ofstream log_file;
    static std::mutex write_log_mtx;

    static int epoll_fd;
    static struct epoll_event* ready_sockets_ptr;

    static std::mutex socket_list_mtx;
    //维持一个聊天室所有用户的 socket 描述符
    static std::vector<int> socket_catalogue;
    //用一个哈希表存储文件描述符到用户连接的struct，内含各种信息包括文件描述符，SSL描述符，尝试连接次数等
    static std::unordered_map<int, client_socket_t> sockets;
    static std::vector<int> to_be_cleaned_val;
    static std::vector<unsigned short int> to_be_cleaned_pos;

    static std::time_t tmp_time_t;
    static std::chrono::system_clock::time_point tmp_now_time;

public:
    login(SSL_CTX* tmp_ssl_ctx_fd, std::condition_variable* tmp_local_msg_queue_cv,
          std::mutex* tmp_local_msg_queue_mtx,
          std::queue<local_msg_type_t>* tmp_local_msg_queue);

    //初始化 login 类
    void init();

    //获取标签，用于返回程序运行信息
    char get_tag(void);

    void send_userlist_to_server();

    static void set_continue_tag(bool tmp_tag);

    //! 线程例程作为成员函数必须为 static
    static void listener(void);

    static void cleaner(void);

private:
    //! 数据库相关函数
    static bool db_open();
    static bool db_init();
    static bool db_if_opened();
    static std::string db_get_userlist();
    static bool db_verify(const char* name, const char* passwd);
    static void db_close();
    static char* now_time(void);
};

#endif