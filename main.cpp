//https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#include "net.h"
#include "tcpclient.h"
#include "cfg.h"
#include "log.h"

using namespace net;

pthread_t InitRpcThread(bool bread){
    pthread_t id;
    int ret;
    if(bread){
        ret=pthread_create(&id,NULL, &tcpclientMgr::readThread, NULL);
    }else{
        ret=pthread_create(&id,NULL, &tcpclientMgr::writeThread, NULL);
    }
    if(ret!=0){
        LOG("Create pthread error!");
        exit (1);
    }
    return id;
}

pthread_t InitSocketListen(char* port){
    if(netServer::g_netServer->initSock(port) != 0){
        exit(0);
    }

    for(int i=0; i<serverCfg::m_gInst->nodeLst.size(); i++){
        nodeCfg* p = serverCfg::m_gInst->nodeLst[i];
        int port = atoi(p->port);
        tcpclientMgr::m_sInst->createTcpclient( p->nodeName, p->ip, port );
    }

    pthread_t id;
    int i,ret;
    ret=pthread_create(&id,NULL, &netServer::netThreadFun , netServer::g_netServer);
    if(ret!=0){
        LOG("Create pthread error!");
        exit (1);
    }
    return id;
}

bool g_run = true;

void onQuit(int sigval){
    LOG("called onQuit sigval: %d", sigval);
    if(sigval == SIGINT || sigval == SIGQUIT ) {
        g_run = false;
    }
}

int main(int argc, char*argv[]){

    signal(SIGINT, onQuit);
    signal(SIGQUIT, onQuit);
    signal(SIGPIPE, SIG_IGN);

    //for log
    pthread_t log_pid = logger::m_inst->init();

    serverCfg::m_gInst->init();

    //for listen socket
    pthread_t id = InitSocketListen( serverCfg::m_gInst->getListen() );

    pthread_t rpcid_r = InitRpcThread(true);
    pthread_t rpcid_w = InitRpcThread(false);

    while(1){
        if(!g_run){
            break;
        }
        netServer::g_netServer->queueProcessFun();
        //netServer::g_netServer->queueProcessRpc();
        //tcpclientMgr::m_sInst->update();
        usleep(1000);
    }

    netServer::g_netServer->destroy();
    tcpclientMgr::m_sInst->destroy();
    pthread_cancel(rpcid_r);
    pthread_cancel(rpcid_w);
    pthread_cancel(log_pid);
    pthread_join(id, NULL);
    pthread_join(log_pid, NULL);
    pthread_join(rpcid_r, NULL);
    pthread_join(rpcid_w, NULL);
    return 0;
}

