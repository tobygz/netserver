
#include "rpchandle.h"


#include <stdio.h>
#include <string.h>

#include "net.h"
#include "recvBuff.h"
#include "connmgr.h"

#define RPC_PING "ping"
#define RPC_PushMsg2Client "PushMsg2Client"
#define RPC_PushMsg2ClientAll "PushMsg2ClientAll"
#define RPC_ForceCloseCliConn "ForceCloseCliConn"
#define RPC_RegGamePid "RegGamePid"


namespace net{


    rpcHandle* rpcHandle::m_pInst = new rpcHandle();

    rpcHandle::rpcHandle(){
        m_mapHandler[(char*) RPC_PING] = &net::rpcHandle::onPing;
        m_mapHandler[(char*) RPC_PushMsg2Client] = &net::rpcHandle::onPushMsg2Client;
        m_mapHandler[(char*) RPC_PushMsg2ClientAll] = &net::rpcHandle::onPushMsg2ClientAll;
        m_mapHandler[(char*) RPC_ForceCloseCliConn] = &net::rpcHandle::onForceCloseCliConn;
        m_mapHandler[(char*) RPC_RegGamePid] = &net::rpcHandle::onRegGamePid;
    }

	void rpcHandle::process(rpcObj* pobj){
		map<char*,PTRFUN>::iterator it ;
        const char* ptar = (const char*)pobj->getTarget();
		if( strcmp( ptar, RPC_PING) == 0) {
			it = m_mapHandler.find((char*)RPC_PING);			
		}else if( strcmp(ptar, RPC_PushMsg2Client) == 0) {
			it = m_mapHandler.find((char*)RPC_PushMsg2Client);			
		}else if( strcmp(ptar, RPC_PushMsg2ClientAll) == 0) {
			it = m_mapHandler.find((char*)RPC_PushMsg2ClientAll);			
		}else if( strcmp(ptar, RPC_ForceCloseCliConn) == 0) {
			it = m_mapHandler.find((char*)RPC_ForceCloseCliConn);			
		}else if( strcmp(ptar, RPC_RegGamePid) == 0) {
			it = m_mapHandler.find((char*)RPC_RegGamePid);			
		}else{
			printf("[ERROR] invalied rpc name: %s\n", pobj->getTarget() );
            return;
		}

        printf("[DEBUG] process rpc name: %s\n", pobj->getTarget() );

		if(it!= m_mapHandler.end()){
			(this->*it->second)(pobj);
		}
	}

    int rpcHandle::onPing(rpcObj*){
    	printf("called onPing\n");
	}

    int rpcHandle::onPushMsg2Client(rpcObj* pobj){
    	printf("called onPushMsg2Client pid: %d bodylen: %d msgid: %d\n", pobj->getPid(), pobj->getBodylen(), pobj->getMsgid());
    	connObjMgr::g_pConnMgr->SendMsg( (unsigned int)pobj->getPid(), pobj->getBodyPtr(), pobj->getBodylen() );
    	/*
    		pid := request.GetPid()
			core.GlobalConnMgr.SendMsg(uint32(pid), request.GetData())
    	*/

	}

    int rpcHandle::onPushMsg2ClientAll(rpcObj* pobj){
    	printf("called onPushMsg2ClientAll\n");
    	connObjMgr::g_pConnMgr->SendMsgAll( pobj->getBodyPtr(), pobj->getBodylen() );
		/*
			core.GlobalConnMgr.SendMsgAll(request.GetData())
		*/
    }

    int rpcHandle::onForceCloseCliConn(rpcObj* pobj){
    	printf("called onForceCloseCliConn\n");
    	connObjMgr::g_pConnMgr->DelConn((unsigned int)pobj->getPid());
		/*
			core.GlobalConnMgr.ForceStop(uint32(request.GetPid()))
		*/
    }

    int rpcHandle::onRegGamePid(rpcObj* pobj){
    	printf("called onRegGamePid\n");
    	connObjMgr::g_pConnMgr->RegGamePid((unsigned int)pobj->getPid(), (char*)pobj->getParam());
		/*
			core.GlobalConnMgr.UpdatePidGame(uint32(request.GetPid()), request.GetParam())
		 */
    }
}
