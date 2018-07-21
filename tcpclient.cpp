
#include "tcpclient.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "net.h"
#include "recvBuff.h"
#include "connmgr.h"
#include "rpchandle.h"
#include "log.h"
#include "qps.h"

namespace net{

    int make_socket_non_blocking (int sfd);

    tcpclientMgr* tcpclientMgr::m_sInst = new tcpclientMgr();
    unsigned int sendCache::g_guid = 0;

    sendCache::sendCache(){
        memset(m_mem,0,RPC_BUFF_SIZE);
        m_offset = 0;
        m_uid = g_guid++;
    }

    tcpclientMgr::tcpclientMgr(){
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );
    }

    //send rpc result to client; called by main thread;
    void tcpclientMgr::processAllRpcobj(){
        pthread_mutex_lock(mutex);
        queue<int> fdQue;
        for( map<string, int>::iterator it = m_mapTcpClient.begin(); it!=m_mapTcpClient.end(); it++){
            fdQue.push(it->second);
        }
        pthread_mutex_unlock(mutex);
        while(!fdQue.empty()){
            int fd = fdQue.front();
            fdQue.pop();
            tcpclient *p = getTcpClientByFd(fd);
            if(!p){
                continue;
            }
            p->handleRpcObj();
        }
    }

    void* tcpclientMgr::readThread(void *){
        queue<int> fdQue;
        while(true){
            pthread_mutex_lock(m_sInst->mutex);
            for( map<string, int>::iterator it = m_sInst->m_mapTcpClient.begin(); it!=m_sInst->m_mapTcpClient.end(); it++){
                fdQue.push(it->second);
            }
            pthread_mutex_unlock(m_sInst->mutex);
            while(!fdQue.empty()){
                int fd = fdQue.front();
                fdQue.pop();
                tcpclient *p = m_sInst->getTcpClientByFd(fd);
                if(!p){
                    continue;
                }
                if( p->OnRead() < 0 ){
                    continue;
                }
                p->dealReadBuffer();
            }
            usleep(1000);
        }
    }

    void* tcpclientMgr::writeThread(void *){
        queue<int> fdQue;
        while(true){
            pthread_mutex_lock(m_sInst->mutex);
            for( map<string, int>::iterator it = m_sInst->m_mapTcpClient.begin(); it!=m_sInst->m_mapTcpClient.end(); it++){
                fdQue.push(it->second);
            }
            pthread_mutex_unlock(m_sInst->mutex);
            while(!fdQue.empty()){
                int fd = fdQue.front();
                fdQue.pop();
                tcpclient *p = m_sInst->getTcpClientByFd(fd);
                if(!p){
                    continue;
                }
                p->dealSend();
            }
            usleep(1000);
        }
    }

    tcpclient* tcpclientMgr::createTcpclient(char* name, char* ip, int port){
        tcpclient* pclient = new tcpclient(name, ip, port);
        pclient->initSock();
        pclient->doconnect();
        m_mapTcpClient[string(name)] = pclient->GetFd();
        m_mapFdTcpClient[pclient->GetFd()] = pclient;
        LOG("[TCPCLIENTMGR] createTcpclient name: %s ip: %s fd: %d", name, ip, pclient->GetFd() );
    }


    void tcpclientMgr::rpcCallGate(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        tcpclient* pclient = getTcpClient((const char*)"gate1");
        if(!pclient ){
            LOG("[ERROR] rpcCallGate failed, find gate1 failed");
            return;
        }
        pclient->AppendSend( target, pid, msgid, pbyte, byteLen );
        LOG("[TCPCLIENTMGR] rpcCallGate target: %s pid: %u msgid: %d bodylen: %d", target, pid, msgid, byteLen );
    }

    void tcpclientMgr::rpcCallGame(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        string sname = connObjMgr::g_pConnMgr->getGameByPid((unsigned int)pid);
        tcpclient* pclient = getTcpClient(sname.c_str());
        if(!pclient ){
            LOG("[ERROR] rpcCallGame rpc: %s failed, find game: %s failed", target, sname.c_str());
            return;
        }
        pclient->AppendSend( target, pid, msgid, pbyte, byteLen );               
        LOG("[TCPCLIENTMGR] rpcCallGame game: %s target: %s pid: %u msgid: %d bodylen: %d", sname.c_str(), target, pid, msgid, byteLen );
    }

    tcpclient* tcpclientMgr::getTcpClient(const char* name){
        string find(name);
        map<string, int>::iterator it = m_mapTcpClient.find(find);
        if( it == m_mapTcpClient.end()){
            return NULL;
        }
        return getTcpClientByFd(it->second);
    }

    tcpclient* tcpclientMgr::getTcpClientByFd(int fd){
        cautoLock m(mutex);
        map<int, tcpclient*>::iterator it = m_mapFdTcpClient.find(fd);
        if( it == m_mapFdTcpClient.end() ){
            return NULL;
        }
        return it->second;
    }
    bool tcpclientMgr::DelConn(int fd){
        cautoLock m(mutex);
        map<int, tcpclient*>::iterator it = m_mapFdTcpClient.find(fd);
        if( it == m_mapFdTcpClient.end() ){
            return false;
        }
        tcpclient *pconn = it->second;
        pconn->OnClose();
        m_mapFdTcpClient.erase( it );   
        m_mapTcpClient.erase( string( pconn->getName() ) );
        delete pconn;
        return true;            
    }    

    void tcpclientMgr::destroy(){
        for( map<string, int>::iterator it = m_mapTcpClient.begin(); it!=m_mapTcpClient.end(); it++){
            tcpclient *p = getTcpClientByFd(it->second);
            if(!p){
                continue;
            }
            p->OnClose();
        }
    }

    tcpclient::tcpclient(char* name, char* ip, int port){
        strcpy(m_name, name);
        strcpy(m_ip, ip);
        m_port = port;
        m_pSendCache = new sendCache;
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );

        mutexRecv = new pthread_mutex_t;
        pthread_mutex_init( mutexRecv, NULL );
    }

    int tcpclient::initSock(){
        struct sockaddr_in *paddr = new sockaddr_in;
        memset(paddr, 0, sizeof(sockaddr_in));
        paddr->sin_family = AF_INET;

        inet_pton(AF_INET, m_ip, &paddr->sin_addr);

        paddr->sin_port = htons(m_port);
        m_pAddr = (char*)paddr;

        // open a stream socket
        if ((m_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            LOG("could not create socket");
            return 1;
        }
        return 0;

    }

    int tcpclient::doconnect(){
        while(true){
            if (connect(m_sock, (struct sockaddr*)m_pAddr,sizeof(sockaddr_in)) >= 0) {
                break;
            }
            usleep(10000);
        }
        /*
        if (connect(m_sock, (struct sockaddr*)m_pAddr,sizeof(sockaddr_in)) < 0) {
            LOG("%s could not connect to server ip: %s:%d",m_name, m_ip, m_port);
            raise(SIGINT);
            return 1;
        }
        */
        LOG("[TCPCLIENT] %s succ connect to server ip: %s:%d",m_name, m_ip, m_port);

        make_socket_non_blocking(m_sock);
        //netServer::g_netServer->epAddFd(m_sock,(char*)m_name);

        //send takeproxy
        AppendSend((char*)"TakeProxy", 0, 0, NULL, 0);    
        return 0;
    }

    int tcpclient::OnRead(){
        if(RPC_BUFF_SIZE-m_recvOffset<=0){
            //add error here
            LOG("[FATAL] tcpclient: %s buffer full: %d RPC_BUFF_SIZE: %d", m_name, m_recvOffset, RPC_BUFF_SIZE);
            return 1;
        }
        ssize_t count = read (m_sock, m_recvBuffer+m_recvOffset, RPC_BUFF_SIZE-m_recvOffset);
        if (count == -1)
        {
            if (errno != EAGAIN){
                tcpclientMgr::m_sInst->DelConn(m_sock);
                return -1;
            }
            return 1;
        }
        else if (count == 0)
        {
            tcpclientMgr::m_sInst->DelConn(m_sock);
            return -1;
        }else {
            m_recvOffset += count;
        }

        return 2;
    }

    int tcpclient::dealReadBuffer(){
        if(m_recvOffset==0){
            return 0;
        }
        int offset=0, bodylen= 0, left=0;
        //parse m_recvBuffer , reset offset
        rpcObj *p = NULL;
        while(true){
            left = m_recvOffset-offset;
            if( left<4 ){
                if( left != 0 ){
                    LOG("[INFO] rpc make combine package m_recvOffset:%d offset:%d", m_recvOffset, offset);
                }
                break;
            }
            bodylen = *(int*)(m_recvBuffer+offset);
            if( left < 4+bodylen){
                LOG("[INFO] rpc make combine package m_recvOffset:%d offset:%d", m_recvOffset, offset);
                break;
            }
            p = new rpcObj();
            p->decodeBuffer(m_recvBuffer+offset );
            p->ToString();
            pthread_mutex_lock(mutexRecv);
            m_queRpcObj.push(p);
            pthread_mutex_unlock(mutexRecv);

            qpsMgr::g_pQpsMgr->updateQps(1, 4+bodylen);
            offset += sizeof(int) + bodylen;
        }

        memmove(m_recvBuffer, m_recvBuffer+offset, m_recvOffset-offset);
        m_recvOffset = m_recvOffset - offset;

        return 0;
    }

    void tcpclient::handleRpcObj(){
        cautoLock mm(mutexRecv);
        while(!m_queRpcObj.empty()){
            rpcObj *p = m_queRpcObj.front();
            m_queRpcObj.pop();
            rpcHandle::m_pInst->process(p);
            delete p;
        }
    }

    void tcpclient::AppendSend(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){                
        //check size enough
        int needSize = rpcObj::getRpcSize(target, pid, msgid, pbyte, byteLen);
        cautoLock autolock(mutex);
        if( !m_pSendCache ) {
            m_pSendCache = new sendCache;
        }
        qpsMgr::g_pQpsMgr->updateQps(2, needSize );
        if(needSize > m_pSendCache->getLeftSize()){
            m_sendCacheQueue.push( m_pSendCache );
            m_pSendCache = new sendCache;
        }
        assert( needSize < m_pSendCache->getLeftSize() );

        int offset = rpcObj::encodeBuffer(m_pSendCache->getWritePtr(), target, pid, msgid, pbyte, byteLen);
        m_pSendCache->updateOffset(offset);
    }

    int tcpclient::dealSend(){
        cautoLock alock(mutex);
        if(m_sendCacheQueue.size() == 0 && m_pSendCache->getOffset() == 0){
            return 0;
        }
        if( m_pSendCache  ){
            m_sendCacheQueue.push( m_pSendCache );
            m_pSendCache = NULL;
        }
        sendCache *p = NULL;
        int ret = 0;
        if(m_sendCacheQueue.size()!=0){
            LOG("deal send size: %d", m_sendCacheQueue.size());
        }
        while(!m_sendCacheQueue.empty()){
            p = m_sendCacheQueue.front();
            m_sendCacheQueue.pop();
            LOG(" send to gate before size: %d limitsize: %d", p->getOffset(), RPC_BUFF_SIZE );
            assert(p->getOffset()<=RPC_BUFF_SIZE);
            if(p->getOffset() == 0){
                delete p;
                continue;
            }
            if(ret != -1 ){
                ret = dosend(p);
                if( ret == -1 ){
                    LOG("[ERROR] handleSend ret fail");
                    return -1;
                }else{
                    LOG(" send to gate size: %d", p->getOffset() );
                }
            }
            delete p;
        }
        m_pSendCache = NULL;
        return 0;
    }

    int tcpclient::dosend(sendCache *ps){
        unsigned char* pbuf = ps->getPtr();
        size_t size = ps->getOffset();
        size_t s = 0;
        size_t nowSize=0;
        const size_t SEND_SIZE=64*1024;
        int offset = 0;
        int bret = 0;

        bool eagain = false;

        while(1){
            if(size-offset>SEND_SIZE){
                nowSize = SEND_SIZE;
            }else{
                nowSize = size-offset;
            }
            if( nowSize <= 0 ){
                bret = 0;
                break;
            }
            s = write(m_sock, pbuf+offset, nowSize);
            if( s == -1 && errno == EAGAIN ){
                usleep(10);
                eagain = true;
                continue;
            }
            if(s==-1&&errno != EAGAIN ){
                tcpclientMgr::m_sInst->DelConn(m_sock);
                return -1;
            }
            if( eagain ){
                LOG("send eagain size: %d", s );
            }
            offset += s;
        }
        ps->updateOffset(offset);
        return bret;
    }

    void tcpclient::OnClose(){
        //assert(false);
        close(m_sock);
        LOG("[ERROR] tcpclient: %s closed ", m_name);
    }

}
