#ifndef __conn_header__
#define __conn_header__

#include <queue>
#include "recvBuff.h"

using namespace std;


namespace net{

#define PACKET_SIZE 4
#define MSGID_SIZE 4
#define BUFFER_SIZE 10*1024*1024
    class connObj{
        private:
            int m_fd;
            int m_pid;
            
            int m_lastSec; //for timeout process
            char m_remoteAddr[64];

            char m_sendBuf[BUFFER_SIZE];
            int m_sendBufOffset;
            int m_sendBufLen; //amount

            //buffer
            char m_NetBuffer[BUFFER_SIZE];
            int m_NetOffset;    //write offset
            int m_ReadOffset;    //read offset
            bool m_bChkReadZero;
            int _OnRead();
        public:
            connObj(int _fd);
            void SetPid(int _pid);
            int GetPid();
            int GetFd();
            bool IsTimeout(int sec);
            int OnRead();
            void OnClose(bool btimeOut=false);
            void OnInit(char* paddr);
            void dealMsg(msgObj *p);
            void resetBuffer();
            bool parseBuff();
            int send(unsigned char* buf, size_t size);

    };

}
#endif
