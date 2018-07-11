#ifndef __conn_header__
#define __conn_header__

#include <queue>
#include "recvBuff.h"

using namespace std;


namespace net{

#define PACKET_SIZE 4
#define MSGID_SIZE 4
//typedef queue<recvBuff>
class connObj{
    private:
        int fd;
        unsigned long long rid;
        int pid;
        queue<recvBuff*> m_queueRecv;
        recvBuff *m_RecvBuf;
        //packet size
        char packetSizeBuf[PACKET_SIZE];
        int packetSize;
        int packetOffset;

        //msgid 
        char m_MsgIdBuf[MSGID_SIZE];
        int m_Msgid;
        int m_MsgidOffset;


        //body
        int writeOffset;

        int readSize();
        int readMsgid();
        int readBody();
        void ResetVars();
    public:
        connObj(int _fd);
        void SetPid(int _pid);
        int GetPid();
        int GetFd();
        int OnRead();
        int OnProcess();
        void OnClose();
        void send(const char* buf, size_t size);

        //temp
        int GetMsgid(){ return m_Msgid;}
        int GetMsgLen(){ return packetSize;}
        int GetMsgBodyLen(){ return writeOffset;}

};

#endif
}
