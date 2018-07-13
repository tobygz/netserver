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
    };


    class netServer{
        std::queue<NET_OP_ST*> m_netQueue;
        std::map<int,bool> m_readFdMap;
        pthread_mutex_t *mutex ; 
        public:
        static netServer *g_netServer;
        netServer();
        int sockfd;
        int initSock(char *port);
        void destroy();

        void appendSt(NET_OP_ST *pst, bool bmtx=true);
        void appendDataIn(int fd);
        void appendConnNew(int fd);
        void appendConnClose(int fd, bool bmtx=true);

        static void* netThreadFun( void* );
        void queueProcessFun();
        char* GetOpType(NET_OP );

    };

}
#endif
