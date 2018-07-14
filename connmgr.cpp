
#include "connmgr.h"
#include <pthread.h>
#include <stdio.h>

#include "qps.h"
#include "net.h"


namespace net{
    connObjMgr* connObjMgr::g_pConnMgr = new connObjMgr;

    pthread_mutex_t *mutex ; 

    connObjMgr::connObjMgr(){
        maxSessid = 0;
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );
    }

    void connObjMgr::ChkConnTimeout(){
        int sec = getsec();
        queue<connObj*> toLst;
        pthread_mutex_lock(mutex);
        for(map<int,connObj*>::iterator iter = m_connFdMap.begin(); iter!= m_connFdMap.end(); iter++){
            connObj* p = iter->second;
            if(p->IsTimeout(sec)){
                toLst.push(p);
            }
        }
        pthread_mutex_unlock(mutex);
        while(!toLst.empty()){
            connObj *p= toLst.front();
            toLst.pop();
            DelConn(p->GetFd(), true);
        }
    }

    void connObjMgr::CreateConnBatch(void* p) {
        queue<NET_OP_ST *> *pvec = (queue<NET_OP_ST *> *)p;
        pthread_mutex_lock(mutex);
        while(pvec->empty()==false){
            NET_OP_ST *pst = pvec->front();
            pvec->pop();
            connObj *pconn = new connObj(pst->fd);
            pconn->SetPid(maxSessid++);
            m_connMap[pconn->GetPid()] = pconn;
            m_connFdMap[pconn->GetFd()] = pconn;
            pconn->OnInit( (char*)pst->paddr );
        }
        pthread_mutex_unlock(mutex);
    }

    connObj* connObjMgr::CreateConn(int fd ){
        connObj *pconn = new connObj(fd);
        pthread_mutex_lock(mutex);
        pconn->SetPid(maxSessid++);
        m_connMap[pconn->GetPid()] = pconn;
        m_connFdMap[pconn->GetFd()] = pconn;
        pthread_mutex_unlock(mutex);
        return pconn;
    }

    int connObjMgr::GetOnline(){
        int val = 0;
        pthread_mutex_lock(mutex);
        val = m_connFdMap.size();
        pthread_mutex_unlock(mutex);
        return val;
    }

    connObj* connObjMgr::GetConn(int fd ){
        pthread_mutex_lock(mutex);
        map<int,connObj*>::iterator iter = m_connFdMap.find(fd);
        if ( iter != m_connFdMap.end() ){
            connObj* p = iter->second;
            pthread_mutex_unlock(mutex);
            return p;
        }
        pthread_mutex_unlock(mutex);
        return NULL;
    }


    connObj* connObjMgr::GetConnByPid(int pid ){
        pthread_mutex_lock(mutex);
        map<int,connObj*>::iterator iter = m_connMap.find(pid);
        pthread_mutex_unlock(mutex);
        if ( iter != m_connMap.end() ){
            return (connObj*) iter->second;
        }
        return NULL;
    }

    void connObjMgr::DelConn(int fd, bool btimeOut){
        pthread_mutex_lock(mutex);
        connObj *pst = NULL;
        map<int,connObj*>::iterator iter = m_connFdMap.find(fd);
        if ( iter == m_connFdMap.end() ) {
            pthread_mutex_unlock(mutex);
            return;
        }else{
            pst = (connObj*) iter->second;
            m_connFdMap.erase(iter);
            iter = m_connMap.find(pst->GetPid());
            if ( iter != m_connMap.end() ){
                m_connMap.erase(iter);
            }
            pthread_mutex_unlock(mutex);
            pst->OnClose(btimeOut);
            printf("connObjMgr delconn, fd: %d pid: %d\n", fd, pst->GetPid());
            delete pst;
        }
    }

    void connObjMgr::destroy(){
        pthread_mutex_lock(mutex);
        map<int,connObj*>::iterator iter;
        for( iter=m_connMap.begin(); iter!=m_connMap.end(); iter++){
            connObj* tmp = (connObj*) iter->second;
            tmp->OnClose();
        }
        pthread_mutex_unlock(mutex);

    }
    void connObjMgr::ProcessAllConn(){

    }


    void connObjMgr::SendMsg(unsigned int pid, unsigned char* pmem, unsigned int size){
        connObj *p = GetConnByPid(pid);
        if(p!=NULL){
            p->send( pmem, size );
        }
    }

    void connObjMgr::SendMsgAll(unsigned char* pmem, unsigned int size){
        pthread_mutex_lock(mutex);
        for(map<int,connObj*>::iterator iter = m_connFdMap.begin(); iter!=m_connFdMap.end(); iter++){
            connObj* p = iter->second;
            p->send( pmem, size );
        }            
        pthread_mutex_unlock(mutex);
    }

    void connObjMgr::RegGamePid(unsigned int pid, char* pgame){
        m_connPidGameMap[pid] = pgame;
    }

    char* connObjMgr::getGameByPid(int pid){
        map<int, char*>::iterator it = m_connPidGameMap.find(pid);
        if( it==m_connPidGameMap.end()){
            return NULL;
        }
        return it->second;
    }
}
