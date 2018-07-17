#ifndef __HEADER_TCPCLIENT_BUFFER__
#define __HEADER_TCPCLIENT_BUFFER__

#include <map>
#include <queue>
#include <string>
#include "recvBuff.h"

#define RPC_BUFF_SIZE 32*1024*1024
using namespace std;
namespace net{

	class tcpclient;
	class tcpclientMgr {
        private:
            map<string, int> m_mapTcpClient; //name -> fd
            map<int, tcpclient*> m_mapFdTcpClient;
            tcpclientMgr(); 
        public:
            tcpclient* createTcpclient(char* name, char* ip, int port);
            tcpclient* getTcpClient(const char* name);
            tcpclient* getTcpClientByFd(int fd);
            bool DelConn(int fd);

            void update();

            static tcpclientMgr *m_sInst;

            void rpcCallGate(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen);
            void rpcCallGame(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen);

	};

	class tcpclient {

        char m_name[128];
        char m_ip[128];
        int m_port;

        queue<rpcObj*> m_queSendRpcobj;
        queue<rpcObj*> m_queRpcObj;

        char *m_pAddr;

            unsigned char m_sendBuffer[RPC_BUFF_SIZE];
            int m_sendOffset;
            int m_sock;
		public:
            tcpclient(char* pname, char* ip, int port);
			
			char m_sName[512];

            char m_recvBuffer[RPC_BUFF_SIZE];
            int m_recvOffset;

            char* getName(){ return (char*) m_sName; }

			int GetFd(){return m_sock;}
			int initSock();
			int doconnect();
			int OnRead();
			int dealReadBuffer();
			int handleRpcObj(rpcObj*);


            int dealSend();
            void AppendSend(char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen);
			int dosend(unsigned char*, size_t size);

            void OnClose();
	};
}

#endif
