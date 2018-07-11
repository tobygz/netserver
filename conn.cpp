#include "connmgr.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


#include "qps.h"
#include "net.h"

namespace net{

    pthread_mutex_t *mutexRecv ; 

    void connObj::OnClose(){
        printf("called onclose fd: %d pid: %d \r\n", this->GetFd(), this->GetPid());
        close(GetFd());
    }

    void connObj::send(const char* buf, size_t size){
        int sendSize = 0;
        size_t s;
        while(1){
            s = write(fd, buf+sendSize, size-sendSize);
            if(s==-1){
                //connObjMgr::g_pConnMgr->DelConn(fd);

                netServer::g_netServer->appendConnClose(fd);
                return;
            }
            sendSize += s;
            if(sendSize == size ){
                return;
            }
        }
    }


    int connObj::OnProcess(){
        pthread_mutex_lock(mutexRecv);
        qpsMgr::g_pQpsMgr->updateQps(4, m_queueRecv.size());
        while(!m_queueRecv.empty()){
            recvBuff *pbuff = m_queueRecv.front();
            m_queueRecv.pop();
            //printf("[OnProcess] pid: %d fd: %d msgid: %d bodylen: %d\n", GetPid(), GetFd(), pbuff->getMsgid(), pbuff->getBodyLen());
            delete pbuff;
        }
        pthread_mutex_unlock(mutexRecv);
    }

    int connObj::readSize(){
        if(packetSize!=0){
            return -1;
        }

        ssize_t count;
        count = read (fd, packetSizeBuf+packetOffset, PACKET_SIZE-packetOffset);
        if (count == -1)
        {
            //   If errno == EAGAIN, that means we have read all
            //   data. So go back to the main loop. 
            if (errno != EAGAIN){
                netServer::g_netServer->appendConnClose(fd);
                return 0;
            }
            return 1;
        }
        else if (count == 0)
        {
            // End of file. The remote has closed the
            //   connection. 
            netServer::g_netServer->appendConnClose(fd);
            return 0;
        }else if (count + packetOffset < PACKET_SIZE ){
            packetOffset = packetOffset + count;
            return 3;
        }else if (count + packetOffset == PACKET_SIZE ){
            packetOffset = PACKET_SIZE;
            //read body
            memcpy(&packetSize, packetSizeBuf, PACKET_SIZE);
            return 4;
        }
        printf("invalled called readSize\n");
    }

    int connObj::readMsgid(){
        if(m_Msgid!=-1){
            return -1;
        }

        ssize_t count;
        count = read (fd, m_MsgIdBuf+m_MsgidOffset, MSGID_SIZE-m_MsgidOffset);
        if (count == -1)
        {
            //   If errno == EAGAIN, that means we have read all
            //   data. So go back to the main loop. 
            if (errno != EAGAIN){
                netServer::g_netServer->appendConnClose(fd);
                return 0;
            }
            return 1;
        }
        else if (count == 0)
        {
            // End of file. The remote has closed the
            //   connection. 
            netServer::g_netServer->appendConnClose(fd);
            return 0;
        }else if (count + m_MsgidOffset < MSGID_SIZE){
            m_MsgidOffset = m_MsgidOffset + count;
            return 3;
        }else if (count + m_MsgidOffset == MSGID_SIZE){
            m_MsgidOffset = PACKET_SIZE;
            //read body
            memcpy(&m_Msgid, m_MsgIdBuf, MSGID_SIZE);
            return 4;
        }
        printf("invalled called readSize\n");
    }

    int connObj::readBody(){
        if(packetSize==0){
            return -1;
        }
        if(m_Msgid==-1){
            return -2;
        }
        if( m_RecvBuf == NULL){
            m_RecvBuf = new recvBuff(packetSize);
        }

        char* pbuf;
        ssize_t count;
        m_RecvBuf->getMem(pbuf);
        count = read(fd, pbuf+writeOffset, packetSize-writeOffset);
        if(count == -1){
            if(errno != EAGAIN){
                netServer::g_netServer->appendConnClose(fd);
                return 0;
            }
            return 1;
        }else if(count ==0){
            netServer::g_netServer->appendConnClose(fd);
            return 0;
        }else if(count + writeOffset<packetSize) {
            writeOffset = writeOffset + count;
            return 2;
        }else if(count +writeOffset == packetSize ){
            m_RecvBuf->setMsgid(GetMsgid());
            pthread_mutex_lock(mutexRecv);
            m_queueRecv.push(m_RecvBuf);
            pthread_mutex_unlock(mutexRecv);
            int nowSize = m_RecvBuf->getBodyLen()+8;
            qpsMgr::g_pQpsMgr->updateQps(1, nowSize);
            //printf("=======> append to m_queueRecv len: %d\n", m_queueRecv.size());
            ResetVars();
            return 3;
        }

    }

    int connObj::OnRead(){
        int ret, ret1, ret2;
        int count = 0;
        while(count<20){
            ret = readSize();
            if(ret == 0 || ret == 1){
                return 0;
            }
            ret1 = readMsgid();
            if(ret1 == 0 || ret1 == 1){
                return 0;
            }
            ret2 = readBody();
            if(ret2 == 0 || ret2 == 1){
                return 0;
            }
            count++;
        }
        //printf("called OnRead read ret size: %d msgid: %d body len: %d ret2: %d\n", GetMsgLen(), GetMsgid(), GetMsgBodyLen(), ret2);
        return 1;
    }

    connObj::connObj(int _fd){
        fd = _fd;
        rid = 0;
        pid = 0;

        ResetVars();

        mutexRecv = new pthread_mutex_t;
        pthread_mutex_init( mutexRecv, NULL );
    }

    void connObj::ResetVars(){
        //packet size
        packetOffset = 0;
        memset(packetSizeBuf,0,sizeof packetSizeBuf);
        packetSize = 0;

        //msgid 
        memset(m_MsgIdBuf,0, MSGID_SIZE);
        m_Msgid = -1;
        m_MsgidOffset = 0;

        //body
        writeOffset = 0;
        m_RecvBuf = NULL;
    }

    void connObj::SetPid(int _pid){
        pid = _pid;
    }
    int connObj::GetPid(){
        return pid;
    }
    int connObj::GetFd(){
        return fd;
    }

}
