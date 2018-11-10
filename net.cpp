#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

#include "connmgr.h"
#include "qps.h"

#define MAXEVENTS 1024

namespace net{

    netServer* netServer::g_netServer = new netServer;

    netServer::netServer(){
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );
    }

    static int make_socket_non_blocking (int sfd)
    {
        int flags, s;

        flags = fcntl (sfd, F_GETFL, 0);
        if (flags == -1)
        {
            perror ("fcntl");
            return -1;
        }

        flags |= O_NONBLOCK;
        s = fcntl (sfd, F_SETFL, flags);
        if (s == -1)
        {
            perror ("fcntl");
            return -1;
        }

        return 0;
    }

    static int create_and_bind (char *port)
    {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int s, sfd;


        //signal
        struct sigaction act;
        act.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &act, NULL) ;

        memset (&hints, 0, sizeof (struct addrinfo));
        hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices 
        hints.ai_socktype = SOCK_STREAM; // We want a TCP socket 
        hints.ai_flags = AI_PASSIVE;     // All interfaces 

        s = getaddrinfo (NULL, port, &hints, &result);
        if (s != 0)
        {
            fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
            return -1;
        }

        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1)
                continue;

            s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
            if (s == 0)
            {
                // We managed to bind successfully! 
                break;
            }

            close (sfd);
        }

        if (rp == NULL)
        {
            fprintf (stderr, "Could not bind: %d\n", errno);
            return -1;
        }

        freeaddrinfo (result);

        return sfd;
    }

    void netServer::destroy() {
        connObjMgr::g_pConnMgr->destroy();
        usleep(1000);
        close (sockfd);
    }

    int netServer::initSock(char *port) {
        int sfd, s;
        int efd;

        sfd = create_and_bind (port);
        if (sfd == -1)
            return -1;

        s = make_socket_non_blocking (sfd);
        if (s == -1)
            return -2;
        this->sockfd = sfd;
        return 0;
    }

    void* netServer::netThreadFun( void *param) {
        netServer *pthis = (netServer*) param;
        struct epoll_event event;
        struct epoll_event *events;

        int s = listen (pthis->sockfd, SOMAXCONN);
        if (s == -1)
        {
            perror ("listen");
            return NULL;
        }

        int efd = epoll_create1 (0);
        if (efd == -1)
        {
            perror ("epoll_create");
            return NULL;
        }

        event.data.fd = pthis->sockfd;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl (efd, EPOLL_CTL_ADD, pthis->sockfd, &event);
        if (s == -1)
        {
            perror ("epoll_ctl");
            return NULL;
        }

        // Buffer where events are returned 
        events = (epoll_event*) calloc (MAXEVENTS, sizeof event);

        // The event loop 
        while (1)
        {
            int n, i;

            n = epoll_wait (efd, events, MAXEVENTS, -1);
            if(n<0){
                printf("epoll_wait error:%d\r\n", errno);
                break;
            }
            qpsMgr::g_pQpsMgr->updateQps(2, n);
            for (i = 0; i < n; i++)
            {
                if ((events[i].events & EPOLLERR) ||
                        (events[i].events & EPOLLHUP) ||
                        (!(events[i].events & EPOLLIN)))
                {
                    // An error has occured on this fd, or the socket is not
                    //   ready for reading (why were we notified then?) 
                    pthis->appendConnClose(events[i].data.fd);
                    continue;
                }

                else if (pthis->sockfd == events[i].data.fd)
                {
                    // We have a notification on the listening socket, which
                    //   means one or more incoming connections. 
                    while (1)
                    {
                        struct sockaddr in_addr;
                        socklen_t in_len;
                        int infd;
                        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                        in_len = sizeof in_addr;
                        infd = accept (pthis->sockfd, &in_addr, &in_len);
                        if (infd == -1)
                        {
                            if ((errno == EAGAIN) ||
                                    (errno == EWOULDBLOCK))
                            {
                                // We have processed all incoming
                                //  connections. 
                                break;
                            }
                            else
                            {
                                perror ("accept");
                                break;
                            }
                        }

                        s = getnameinfo (&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                        if (s == 0)
                        {
                            printf("%.1f Accepted connection on descriptor %d "
                                    "(host=%s, port=%s)\r\n",getms()/1000.0, infd, hbuf, sbuf);
                        }

                        // Make the incoming socket non-blocking and add it to the
                        // list of fds to monitor. 
                        s = make_socket_non_blocking (infd);
                        if (s == -1)
                            abort ();

                        event.data.fd = infd;
                        event.events = EPOLLIN | EPOLLET;
                        s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror ("epoll_ctl");
                            abort ();
                        }
                        //new connection maked
                        pthis->appendConnNew(infd);
                        //break;
                    }
                    continue;
                }
                else
                {
                    // We have data on the fd waiting to be read. Read and
                    //   display it. We must read whatever data is available
                    //   completely, as we are running in edge-triggered mode
                    //   and won't get a notification again for the same
                    //   data. 
                    //int done = 0;
                    pthis->appendDataIn(events[i].data.fd);

                }
            }
        }

        free (events);

        close (pthis->sockfd);
        printf("exit epoll thread\n");

        return NULL;
    }

    void netServer::queueProcessFun(){
        //*
        queue<int> queNew;
        pthread_mutex_lock(mutex);

        while(!this->m_netQueue.empty()){
            NET_OP_ST *pst = m_netQueue.front();
            m_netQueue.pop();
            if(pst==NULL){
                continue;
            }
            if(pst->op == NEW_CONN){
                queNew.push(pst->fd);
            }else if(pst->op == DATA_IN){
                m_readFdMap[pst->fd] = true;
            }else if(pst->op == QUIT_CONN){
                connObjMgr::g_pConnMgr->DelConn(pst->fd);
            }
            delete pst;
        }

        pthread_mutex_unlock(mutex);
        connObjMgr::g_pConnMgr->CreateConnBatch(&queNew);
        qpsMgr::g_pQpsMgr->updateQps(3, m_readFdMap.size());

        queue<int> delLst;
        //process all read event
        int fd, ret;
        for(map<int,bool>::iterator iter = m_readFdMap.begin(); iter != m_readFdMap.end(); iter++){
            fd = (int) iter->first;
            connObj *pconn = connObjMgr::g_pConnMgr->GetConn( fd );
            if (pconn == NULL){
                delLst.push(fd);
                continue;
            }
            ret = pconn->OnRead();
            if( ret == 0 ){
                delLst.push(fd);
            }
        }
        while(!delLst.empty()){
            int nfd = delLst.front();
            delLst.pop();
            m_readFdMap.erase(nfd);
        }

        qpsMgr::g_pQpsMgr->dumpQpsInfo();
        connObjMgr::g_pConnMgr->ChkConnTimeout();
    }

    void netServer::appendSt(NET_OP_ST *pst, bool bmtx){
        if(bmtx){
            pthread_mutex_lock(mutex);
        }
        m_netQueue.push(pst);
        if(bmtx){
            pthread_mutex_unlock(mutex);
        }
    }

    void netServer::appendDataIn(int fd){
        NET_OP_ST *pst = new NET_OP_ST();
        pst->op = DATA_IN;
        pst->fd = fd;
        this->appendSt(pst);
    }

    void netServer::appendConnNew(int fd){
        NET_OP_ST *pst = new NET_OP_ST();
        pst->op = NEW_CONN;
        pst->fd = fd;
        this->appendSt(pst);
    }
    void netServer::appendConnClose(int fd, bool bmtx){
        NET_OP_ST *pst = new NET_OP_ST();
        pst->op = QUIT_CONN;
        pst->fd = fd;
        this->appendSt(pst, bmtx);
    }


    char* netServer::GetOpType(NET_OP op){
        if( op == NEW_CONN){
            return (char*)"NEW_CONN";
        }else if( op == DATA_IN ) {
            return (char*)"DATA_IN";
        }else if( op == QUIT_CONN) {
            return (char*)"QUIT_CONN";
        }
        return (char*)"";
    }
}
