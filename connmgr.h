#ifndef __conn_mgr_header__
#define __conn_mgr_header__

#include <map>
#include <queue>
#include "conn.h"

namespace net{
    class connObjMgr{
        map<int,connObj*> m_connMap; //sessid
        map<int,connObj*> m_connFdMap; //fd
        int maxSessid;
        public:
        connObjMgr();
        int GetOnline();
        void CreateConnBatch(queue<int>*);

        connObj* CreateConn(int fd );
        connObj* GetConn(int fd);
        connObj* GetConnByPid(int pid );
        void DelConn(int pid );
        void destroy();
        void ProcessAllConn();
        static connObjMgr *g_pConnMgr ;
    };
}
#endif
