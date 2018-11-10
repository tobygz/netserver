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

        if(!parseBuff()){
            if( getsec() - m_lastSec >= 5){
                bret = 0;
            }
        }
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
        int len = 0;
        while(!m_queueRecvMsg.empty()){
            msgObj *p = m_queueRecvMsg.front();
            m_queueRecvMsg.pop();
            //*
            len = *(int*)(p->m_pbodyLen);
            send((char*)p->m_pbodyLen, sizeof(int)*2+ len);
            //*/
            p->update();
            delete p;
        }
    }

    bool connObj::IsTimeout(int nowsec){
        if(m_lastSec==0){
            return false;
        }
        if( nowsec - m_lastSec >= 11){
            printf("nowsec: %d lastsec: %d timeout\n", nowsec, m_lastSec);
            return true;
        }else{
            return false;
        }
    }

    bool connObj::parseBuff(){
        if(m_NetOffset!=0){
            m_lastSec = getsec();
        }else{
            return false;
        }
        m_ReadOffset = 0;
        int *psize = NULL;
        int *pmsgid = NULL;
        while(true){
            if(m_NetOffset-m_ReadOffset<sizeof(int)*2){
                return true;
            }

            psize = (int*)m_NetBuffer;
            pmsgid = (int*)(m_NetBuffer+sizeof(int));
            if(m_NetOffset-sizeof(int)*2-m_ReadOffset < *psize){
                return true;
            }

            msgObj *p = new msgObj(pmsgid, psize, m_NetBuffer+m_ReadOffset+sizeof(int)*2);
            qpsMgr::g_pQpsMgr->updateQps(1, p->size());
            m_queueRecvMsg.push(p);
            m_ReadOffset += *psize +sizeof(int)*2;
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
        m_lastSec = 0;
        m_NetOffset = 0;
        m_bChkReadZero = true;


        ResetVars();
        memset(m_NetBuffer,0,BUFFER_SIZE);

        mutexRecv = new pthread_mutex_t;
        pthread_mutex_init( mutexRecv, NULL );
    }

    void connObj::ResetVars(){
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
