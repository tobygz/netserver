//https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#include "net.h"
#include "tcpclient.h"

using namespace net;

pthread_t InitRpc(){
    if(netServer::g_netRpcServer->initEpoll()!=0){
        exit(0);
    }
    pthread_t id;
    int ret =pthread_create(&id,NULL, &netServer::netThreadFun , netServer::g_netRpcServer);
    if(ret!=0){
        printf ("Create pthread error!\n");
        exit (1);
    }
    tcpclientMgr::m_sInst->createTcpclient((char*)"gate1", (char*)"127.0.0.1", 6011);
    return id;

}

pthread_t InitSocketListen(char* port){
    if(netServer::g_netServer->initSock(port) != 0){
        exit(0);
    }
    pthread_t id;
    int i,ret;
    ret=pthread_create(&id,NULL, &netServer::netThreadFun , netServer::g_netServer);
    if(ret!=0){
        printf ("Create pthread error!\n");
        exit (1);
    }
    return id;
}

bool g_run = true;

void onQuit(int sigval){
    printf("called onQuit sigval: %d\n", sigval);
    if(sigval == SIGINT || sigval == SIGQUIT ) {
        g_run = false;
    }

}

int main(){

    signal(SIGINT, onQuit);
    signal(SIGQUIT, onQuit);

    //for listen socket
    char* port = (char*)"6010";
    pthread_t id = InitSocketListen(port);
    pthread_t idRpc = InitRpc();

    while(1){
        if(!g_run){
            break;
        }
        netServer::g_netServer->queueProcessFun();
        netServer::g_netRpcServer->queueProcessRpc();
        usleep(1000000);
    }

    netServer::g_netServer->destroy();
    netServer::g_netRpcServer->destroy();
    pthread_cancel(id);
    pthread_cancel(idRpc);
    pthread_join(id, NULL);
    pthread_join(idRpc, NULL);
    return 0;
}

