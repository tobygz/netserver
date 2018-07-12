#ifndef __conn_header__
#define __conn_header__

#include <queue>
#include "recvBuff.h"

using namespace std;


namespace net{

#define PACKET_SIZE 4
#define MSGID_SIZE 4
#define BUFFER_SIZE 4096 
//typedef queue<recvBuff>
class connObj{
    private:
        int fd;
        unsigned long long rid;
        int pid;
        queue<msgObj*> m_queueRecvMsg;
        //packet size
        char packetSizeBuf[PACKET_SIZE];
        int m_packetSize;
        int m_Msgid;


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
        int OnRead();
        void OnClose();
        void parseBuffer();
        void dealMsgQueue();
        void resetBuffer();
        void parseBuff();
        void send(const char* buf, size_t size);

};

#endif
}
