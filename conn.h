#ifndef __conn_header__
#define __conn_header__

#include <queue>
#include "recvBuff.h"

using namespace std;


namespace net{

#define PACKET_SIZE 4
#define MSGID_SIZE 4
#define BUFFER_SIZE 64*1024
    //typedef queue<recvBuff>
    class connObj{
        private:
            int m_fd;
            unsigned long long rid;
            int m_pid;
            queue<msgObj*> m_queueRecvMsg;
            //packet size
            char packetSizeBuf[PACKET_SIZE];
            
            int m_lastSec; //for timeout process
            char m_remoteAddr[64];

            void ResetVars();

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
            void dealMsgQueue();
            void resetBuffer();
            bool parseBuff();
            void send(unsigned char* buf, size_t size);

    };

}
#endif
