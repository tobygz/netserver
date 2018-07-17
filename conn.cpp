#include "connmgr.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


#include "qps.h"
#include "net.h"
#include "tcpclient.h"

namespace net{

    pthread_mutex_t *mutexRecv ; 

    void connObj::OnClose(bool btimeOut){
        printf("called onclose fd: %d pid: %d \r\n", this->GetFd(), this->GetPid());
        close(GetFd());
        if(btimeOut){
            tcpclientMgr::m_sInst->rpcCallGate((char*)"RegConn", m_pid, 3, NULL, 0);    
        }else{
            tcpclientMgr::m_sInst->rpcCallGate((char*)"RegConn", m_pid, 2, NULL, 0);
        }
    }

    void connObj::OnInit(char* paddr){
        strcpy(m_remoteAddr, paddr);
        unsigned int byteLen = strlen(m_remoteAddr);
        tcpclientMgr::m_sInst->rpcCallGate((char*)"RegConn", m_pid, 1, (unsigned char*)m_remoteAddr, byteLen);
    }

    int connObj::send(unsigned char* buf, size_t size){
        if( size == 0 && m_sendBufLen == 0){
            return 0;
        }
        if( buf != NULL ){
            memcpy(m_sendBuf+m_sendBufLen, buf, size );
        }
        m_sendBufLen += size;
        assert(m_sendBufOffset>=0);
        assert(m_sendBufLen>=0);
        size_t s;
        size_t nowSize=0;
        const size_t SEND_SIZE=64*1024;
        while(1){
            if(m_sendBufLen-m_sendBufOffset>SEND_SIZE){
                nowSize = SEND_SIZE;
            }else{
                nowSize = m_sendBufLen-m_sendBufOffset;
            }
            s = write(m_fd, m_sendBuf+m_sendBufOffset, nowSize);
            if( s == -1 && errno == EAGAIN ){
                connObjMgr::g_pConnMgr->AddWriteConn(m_fd, this);
                return -1;
            }
            if(s==-1&&errno != EAGAIN ){
                netServer::g_netServer->appendConnClose(m_fd);
                return 0;
            }
            assert(s>0);
            assert(m_sendBufOffset>=0);
            assert(m_sendBufLen>=0);
            m_sendBufOffset += s;
            if(m_sendBufOffset >= m_sendBufLen){
                m_sendBufOffset = 0;
                m_sendBufLen =0;
                return 0;
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
    void connObj::dealMsg(msgObj *p){
        assert(p);
        int len = 0;
        unsigned int msgid = 0;
        msgid = p->getMsgid();
        if( msgid >=0 && msgid<1999){
            tcpclientMgr::m_sInst->rpcCallGate((char*)"Msg2gate", m_pid, msgid, p->getBodyPtr(), p->getBodylen());
        }else if(msgid >=2000 && msgid <2999){
            tcpclientMgr::m_sInst->rpcCallGate((char*)"Msg2game", m_pid, msgid, p->getBodyPtr(), p->getBodylen());
        }else{
            printf("[ERROR] invalid msgid: %d\n", msgid);
        }
        p->update();
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
        unsigned int *psize = NULL;
        unsigned int *pmsgid = NULL;
        while(true){
            if(m_NetOffset-m_ReadOffset<sizeof(int)*2){
                return true;
            }

            psize = (unsigned int*)m_NetBuffer;
            pmsgid = (unsigned int*)(m_NetBuffer+sizeof(int));
            if(m_NetOffset-sizeof(int)*2-m_ReadOffset < *psize){
                return true;
            }

            msgObj *p = new msgObj(pmsgid, psize, (unsigned char*)(m_NetBuffer+m_ReadOffset+sizeof(int)*2) );
            qpsMgr::g_pQpsMgr->updateQps(1, p->size());
            dealMsg(p);
            delete p;
            m_ReadOffset += *psize +sizeof(int)*2;
        }
    }
    int connObj::_OnRead(){
        ssize_t count;
        count = read (m_fd, m_NetBuffer+m_NetOffset, BUFFER_SIZE-m_NetOffset);
        if (count == -1)
        {
            //   If errno == EAGAIN, that means we have read all
            //   data. So go back to the main loop. 
            if (errno != EAGAIN){
                netServer::g_netServer->appendConnClose(m_fd);
                return 0;
            }
            m_bChkReadZero = true;
            return 1;
        }
        else if (count == 0&&m_bChkReadZero)
        {
            // End of file. The remote has closed the
            //   connection. 
            netServer::g_netServer->appendConnClose(m_fd);
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
        m_fd = _fd;
        m_pid = 0;
        m_lastSec = 0;
        m_NetOffset = 0;
        m_sendBufOffset= 0;
        m_sendBufLen = 0;
        m_bChkReadZero = true;


        memset(m_NetBuffer,0,BUFFER_SIZE);
        memset(m_sendBuf,0,BUFFER_SIZE);

        mutexRecv = new pthread_mutex_t;
        pthread_mutex_init( mutexRecv, NULL );
    }

    void connObj::SetPid(int _pid){
        m_pid = _pid;
    }
    int connObj::GetPid(){
        return m_pid;
    }
    int connObj::GetFd(){
        return m_fd;
    }

}
