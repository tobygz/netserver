
#include "tcpclient.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "net.h"
#include "recvBuff.h"
#include "connmgr.h"
#include "rpchandle.h"


namespace net{

    int make_socket_non_blocking (int sfd);

    tcpclientMgr* tcpclientMgr::m_sInst = new tcpclientMgr();

    tcpclientMgr::tcpclientMgr(){
        //init epoll_obj

    }

    void tcpclientMgr::update(){
        for( map<char*, tcpclient*>::iterator it = m_mapTcpClient.begin(); it!=m_mapTcpClient.end(); it++){
            it->second->dealSend();
        }
    }

    tcpclient* tcpclientMgr::createTcpclient(char* name, char* ip, int port){
        tcpclient* pclient = new tcpclient(name, ip, port);
        pclient->initSock();
        pclient->doconnect();
        m_mapTcpClient[name] = pclient;
        m_mapFdTcpClient[pclient->GetFd()] = pclient;
        printf("[tcpclientMgr] createTcpclient name: %s ip: %s fd: %d\n", name, ip, pclient->GetFd() );
    }


    void tcpclientMgr::rpcCallGate(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        tcpclient* pclient = getTcpClient((char*)"gate1");
        if(!pclient ){
            printf("[ERROR] rpcCallGate failed, find gate1 failed\n");
            return;
        }
        pclient->AppendSend( target, pid, msgid, pbyte, byteLen );
    }

    void tcpclientMgr::rpcCallGame(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        char *name = connObjMgr::g_pConnMgr->getGameByPid((unsigned int)pid);
        tcpclient* pclient = getTcpClient(name);
        if(!pclient ){
            printf("[ERROR] rpcCallGame rpc: %s failed, find game: %s failed\n", target, name);
            return;
        }
        pclient->AppendSend( target, pid, msgid, pbyte, byteLen );               
    }

    tcpclient* tcpclientMgr::getTcpClient(char* name){
        for( map<char*, tcpclient*>::iterator it = m_mapTcpClient.begin(); it!= m_mapTcpClient.end(); it++ ){
            if( strcmp( name, it->first ) == 0 ){
                return it->second;
            }
        }
        return NULL;

    }
    tcpclient* tcpclientMgr::getTcpClientByFd(int fd){
        map<int, tcpclient*>::iterator it = m_mapFdTcpClient.find(fd);
        if( it == m_mapFdTcpClient.end() ){
            return NULL;
        }
        return it->second;
    }
    bool tcpclientMgr::DelConn(int fd){
        map<int, tcpclient*>::iterator it = m_mapFdTcpClient.find(fd);
        if( it == m_mapFdTcpClient.end() ){
            return false;
        }
        tcpclient *pconn = it->second;
        pconn->OnClose();
        m_mapFdTcpClient.erase( it );   
        delete pconn;
        return true;            
    }    

    tcpclient::tcpclient(char* name, char* ip, int port){
        strcpy(m_name, name);
        strcpy(m_ip, ip);
        m_port = port;
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
            printf("could not create socket\n");
            return 1;
        }
        return 0;

    }

    int tcpclient::doconnect(){
        if (connect(m_sock, (struct sockaddr*)m_pAddr,sizeof(sockaddr_in)) < 0) {
            printf("%s could not connect to server ip: %s:%d\n",m_name, m_ip, m_port);
            raise(SIGINT);
            return 1;
        }
        printf("%s succ connect to server ip: %s:%d\n",m_name, m_ip, m_port);
        make_socket_non_blocking(m_sock);

        netServer::g_netRpcServer->epAddFd(m_sock);

        //send takeproxy
        AppendSend((char*)"TakeProxy", 0, 0, NULL, 0);    
        return 0;
    }

    int tcpclient::OnRead(){
        if(RPC_BUFF_SIZE-m_recvOffset<=0){
            //add error here
            return 1;
        }
        ssize_t count = read (m_sock, m_recvBuffer+m_recvOffset, RPC_BUFF_SIZE-m_recvOffset);
        printf("tcpclient::OnRead read size: %d\n", count);
        if (count == -1)
        {
            //   If errno == EAGAIN, that means we have read all
            //   data. So go back to the main loop. 
            if (errno != EAGAIN){
                netServer::g_netRpcServer->appendConnClose(m_sock);
                return 0;
            }
            return 1;
        }
        else if (count == 0)
        {
            // End of file. The remote has closed the
            //   connection. 
            netServer::g_netRpcServer->appendConnClose(m_sock);
            return 0;
        }else {
            //todo add fatal error here!!!
            m_recvOffset += count;
        }

        return 2;
    }

    int tcpclient::dealReadBuffer(){
        if(m_recvOffset==0){
            return 0;
        }
        int offset=0, headlen = 0;
        //parse m_recvBuffer , reset offset
        rpcObj *p = NULL;
        while(true){
            if( m_recvOffset-offset<4 ){
                //todo, error here, rpc not allowed combine package
                break;
            }
            headlen = *(int*)(m_recvBuffer+offset);
            p = new rpcObj();
            p->decodeBuffer(m_recvBuffer+offset );
            p->ToString();
            m_queRpcObj.push(p);
            offset += sizeof(int) + headlen;
        }
        memmove(m_recvBuffer, m_recvBuffer+offset, m_recvOffset-offset);
        m_recvOffset = m_recvOffset - offset;

        while(!m_queRpcObj.empty()){
            p = m_queRpcObj.front();
            m_queRpcObj.pop();

            handleRpcObj(p);

            delete p;
        }

        return 0;
    }

    int tcpclient::handleRpcObj(rpcObj *p){
        //p->update();
        rpcHandle::m_pInst->process(p);
        return 0;
    }

    void tcpclient::AppendSend(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){                
        int offset = rpcObj::encodeBuffer((unsigned char*)m_sendBuffer+m_sendOffset, target, pid, msgid, pbyte, byteLen);
        m_sendOffset += offset;
        printf("tcpclient::AppendSend mname: %s target: %s offset: %d m_offset: %d\n",m_name, target, offset, m_sendOffset );
    }

    int tcpclient::dealSend(){
        if(m_sendOffset==0){
            return 0;
        }
        int ret = dosend((unsigned char*)m_sendBuffer, m_sendOffset);
        if(ret == 0){
            m_sendOffset =0;
        }else{
            printf("[ERROR] handleSend ret fail\n");
        }
        return 0;
    }

    int tcpclient::dosend(unsigned char* pbuf, size_t size){
        int sendSize = 0;
        size_t s;
        while(1){
            s = write(m_sock, pbuf+sendSize, size-sendSize);
            if(s==-1){
                //netServer::g_netServer->appendConnClose(m_sock);
                return 1;
            }
            sendSize += s;
            if(sendSize == size ){
                return 0;
            }
        }        
        printf("dosend size: %d m_name: %s\n", size, m_name );
        return 0;
    }

    void tcpclient::OnClose(){
        printf("[ERROR] tcpclient: %s closed \n", m_name);
    }

}
