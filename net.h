#ifndef __net_header__
#define __net_header__

#include <queue>
#include <map>
#include <pthread.h>


namespace net{

    enum NET_OP {
        NONE,
        NEW_CONN,
        DATA_IN,
        QUIT_CONN,
    };

    struct NET_OP_ST {
        NET_OP op;
        int fd;
        char paddr[64];
    };


    int make_socket_non_blocking (int sfd);
    static int create_and_bind (char *port);

    class netServer{
        std::queue<NET_OP_ST*> m_netQueue;
        std::map<int,bool> m_readFdMap;
        pthread_mutex_t *mutex ; 
        public:
        static netServer *g_netServer; //for client
        static netServer *g_netRpcServer; //for other server rpc
        netServer();
        int m_sockfd;
        int m_epollfd;
        int epAddFd(int fd);
        int initSock(char *port);
        int initEpoll();
        void destroy();

        void appendSt(NET_OP_ST *pst, bool bmtx=true);
        void appendDataIn(int fd);
        void appendConnNew(int fd, char *pip, char* pport);
        void appendConnClose(int fd, bool bmtx=true);

        static void* netThreadFun( void* );
        void queueProcessFun(); //for clients
        char* GetOpType(NET_OP );

        void queueProcessRpc(); //for rpc

    };

}
#endif
