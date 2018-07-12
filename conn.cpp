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


    int connObj::OnRead(){
        int bret = 1;
        int ret = _OnRead();
        if( ret ==0 || ret ==1 ){
            bret = 0;
        }

        parseBuff();
        dealMsgQueue();
        resetBuffer();
        return bret;
    }
    void connObj::resetBuffer(){
        if (m_NetOffset<=m_ReadOffset){
            m_NetOffset = 0;
            return;
        }
        memmove(m_NetBuffer,m_NetBuffer+m_ReadOffset,m_NetOffset-m_ReadOffset);
        m_NetOffset = m_NetOffset-m_ReadOffset;
    }
    void connObj::dealMsgQueue(){
        //qpsMgr::g_pQpsMgr->updateQps(4, m_queueRecvMsg.size());
        while(!m_queueRecvMsg.empty()){
            msgObj *p = m_queueRecvMsg.front();
            m_queueRecvMsg.pop();
            p->update();
            delete p;
        }
    }
    void connObj::parseBuff(){
        m_ReadOffset = 0;
        while(true){
            if(m_NetOffset-m_ReadOffset<sizeof(m_Msgid)*2){
                return;
            }
            memcpy(&m_packetSize, m_NetBuffer+m_ReadOffset, sizeof(m_packetSize));
            memcpy(&m_Msgid, m_NetBuffer+m_ReadOffset+sizeof(m_packetSize), sizeof(m_Msgid));
            if(m_NetOffset-sizeof(m_packetSize)-sizeof(m_Msgid)-m_ReadOffset < m_packetSize ){
                return;
            }
            msgObj *p = new msgObj(m_Msgid, m_packetSize, m_NetBuffer+m_ReadOffset+sizeof(m_packetSize)+sizeof(m_Msgid));
            qpsMgr::g_pQpsMgr->updateQps(1, p->size());
            m_queueRecvMsg.push(p);
            m_ReadOffset += m_packetSize+sizeof(m_packetSize)+sizeof(m_Msgid);
        }
    }
    int connObj::_OnRead(){
        ssize_t count;
        count = read (fd, m_NetBuffer+m_NetOffset, BUFFER_SIZE-m_NetOffset);
        if (count == -1)
        {
            //   If errno == EAGAIN, that means we have read all
            //   data. So go back to the main loop. 
            if (errno != EAGAIN){
                netServer::g_netServer->appendConnClose(fd);
                return 0;
            }
            m_bChkReadZero = true;
            return 1;
        }
        else if (count == 0&&m_bChkReadZero)
        {
            // End of file. The remote has closed the
            //   connection. 
            netServer::g_netServer->appendConnClose(fd);
            return 0;
        }else if (count + m_NetOffset< BUFFER_SIZE){
            m_bChkReadZero = false;
            m_NetOffset += count;
            return 3;
        }else if (count + m_NetOffset== BUFFER_SIZE){
            m_bChkReadZero = false;
            m_NetOffset += count;
            return 4;
        }
    }

    connObj::connObj(int _fd){
        fd = _fd;
        rid = 0;
        pid = 0;
        m_NetOffset = 0;
        m_bChkReadZero = true;


        ResetVars();
        memset(m_NetBuffer,0,BUFFER_SIZE);

        mutexRecv = new pthread_mutex_t;
        pthread_mutex_init( mutexRecv, NULL );
    }

    void connObj::ResetVars(){
        //packet size
        m_packetSize = 0;
        //msgid 
        m_Msgid = -1;
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
