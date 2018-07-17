#ifndef __HEADER_RPCHANDLER_BUFFER__
#define __HEADER_RPCHANDLER_BUFFER__

#include <map>
#include "recvBuff.h"

using namespace std;
namespace net{


	class rpcHandle {

        typedef int (rpcHandle::*PTRFUN)(rpcObj*);
        map<char*,PTRFUN> m_mapHandler;
		public:
        rpcHandle();

        static rpcHandle *m_pInst;
        
        void process(rpcObj*);
        
        int onPing(rpcObj*);
        int onPushMsg2Client(rpcObj*);
        int onPushMsg2ClientAll(rpcObj*);
        int onForceCloseCliConn(rpcObj*);
        int onRegGamePid(rpcObj*);
        
	};
}

#endif
