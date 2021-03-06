
#include "qps.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "connmgr.h"

using namespace std;
namespace net{
    qpsMgr* qpsMgr::g_pQpsMgr = new qpsMgr;

    pthread_mutex_t *mutexQps ; 

    long long getms(){
		struct timeval te; 
		gettimeofday(&te, NULL); // get current time
		long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
		return milliseconds;
	}

    qpsMgr::qpsMgr(){
        m_lastMs = 0;
        mutexQps = new pthread_mutex_t;
        pthread_mutex_init( mutexQps, NULL );

        //create 1 qpsobj
        addQps(1,(char*)"network");//for network
        addQps(2,(char*)"mainloop");//for mainloop
        addQps(3,(char*)"readfd");//for readfd
        addQps(4,(char*)"dealrecv");//for recvBuff process
    }

    void qpsMgr::updateQps(int id, int _size){
        if(_size==0){
            return;
        }
        pthread_mutex_lock(mutexQps);
        map<int,qpsObj*>::iterator it = m_qpsMap.find(id);
        if( it == m_qpsMap.end() ){
            pthread_mutex_unlock(mutexQps);
            return;
        }
        pthread_mutex_unlock(mutexQps);
        qpsObj* tmp = (qpsObj*)it->second;
        tmp->count++;
        tmp->size += _size;
    }

    void qpsMgr::dumpQpsInfo(){
        long long nowMs = getms();
        if(nowMs - m_lastMs<1000){
            return;
        }
        m_lastMs = nowMs;
        memset(m_debugInfo,0,sizeof m_debugInfo);
        pthread_mutex_lock(mutexQps);
        map<int,qpsObj*>::iterator iter;
        for( iter=m_qpsMap.begin(); iter!=m_qpsMap.end(); iter++){
            qpsObj* tmp = (qpsObj*) iter->second;
            sprintf(m_debugInfo, "%s [type: %s count: %d size: %d]", m_debugInfo, tmp->info, tmp->count, tmp->size );
            tmp->Reset();
        }
        sprintf(m_debugInfo, "%s [online: %d]", m_debugInfo, connObjMgr::g_pConnMgr->GetOnline());

        pthread_mutex_unlock(mutexQps);
        printf("%f [QPS] dump info: %s\n",nowMs/1000.0, m_debugInfo );
    }
}
