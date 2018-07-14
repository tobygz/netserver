#ifndef __conn_mgr_header__
#define __conn_mgr_header__

#include <map>
#include <queue>
#include "conn.h"

namespace net{
    class connObjMgr{
        map<int,connObj*> m_connMap; //sessid
        map<int,connObj*> m_connFdMap; //fd
        map<int, char*> m_connPidGameMap; // pid->game1
        int maxSessid;
    public:
        connObjMgr();
        int GetOnline();
        void CreateConnBatch(void*);
        void ChkConnTimeout();

        connObj* CreateConn(int fd );
        connObj* GetConn(int fd);
        connObj* GetConnByPid(int pid );
        void DelConn(int pid, bool btimeOut=false );
        void destroy();
        void ProcessAllConn();
        static connObjMgr *g_pConnMgr ;

        void SendMsg(unsigned int pid, unsigned char* pmem, unsigned int size);
        void SendMsgAll(unsigned char* pmem, unsigned int size);
        void RegGamePid(unsigned int pid, char* pgame);
        char* getGameByPid(int pid);
    };
}
#endif
