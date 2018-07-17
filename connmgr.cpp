
#include "connmgr.h"
#include <pthread.h>
#include <stdio.h>

#include "qps.h"
#include "net.h"


namespace net{
    connObjMgr* connObjMgr::g_pConnMgr = new connObjMgr;

    pthread_mutex_t *mutex ; 
    pthread_mutex_t *mutexWrite ; 

    connObjMgr::connObjMgr(){
        maxSessid = 1;
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );

        mutexWrite = new pthread_mutex_t;
        pthread_mutex_init( mutexWrite, NULL );
    }

    void connObjMgr::ChkConnTimeout(){
        int sec = getsec();
        connObj *p = NULL;
        queue<connObj*> toLst;
        pthread_mutex_lock(mutex);
        for(map<int,int>::iterator iter = m_connFdMap.begin(); iter!= m_connFdMap.end(); iter++){
            p = rawGetConnByPid(iter->second);
            if(p==NULL){
                continue;
            }
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
            m_connFdMap[pconn->GetFd()] = pconn->GetPid();
            pconn->OnInit( (char*)pst->paddr );
            delete pst;
            printf("add pconn fd: %d pst->fd: %d pid: %d\n", pconn->GetFd(), pst->fd, pconn->GetPid() );
        }
        pthread_mutex_unlock(mutex);
    }

    connObj* connObjMgr::CreateConn(int fd ){
        connObj *pconn = new connObj(fd);
        pthread_mutex_lock(mutex);
        pconn->SetPid(maxSessid++);
        m_connMap[pconn->GetPid()] = pconn;
        m_connFdMap[pconn->GetFd()] = pconn->GetPid();
        pthread_mutex_unlock(mutex);
        return pconn;
    }

    int connObjMgr::GetOnline(){
        int val = 0;
        pthread_mutex_lock(mutex);
        val = m_connMap.size();
        pthread_mutex_unlock(mutex);
        return val;
    }

    connObj* connObjMgr::GetConn(int fd ){
        pthread_mutex_lock(mutex);

        connObj* p=NULL;
        map<int,int>::iterator iter = m_connFdMap.find(fd);
        if ( iter != m_connFdMap.end() ){
            p = rawGetConnByPid(iter->second);
            pthread_mutex_unlock(mutex);
            return p;
        }
        pthread_mutex_unlock(mutex);
        return NULL;
    }


    connObj* connObjMgr::rawGetConnByPid(int pid ){
        map<int,connObj*>::iterator iter = m_connMap.find(pid);
        if ( iter != m_connMap.end() ){
            return (connObj*) iter->second;
        }
        return NULL;
    }

    connObj* connObjMgr::GetConnByPid(int pid ){

        connObj* p=NULL;
        pthread_mutex_lock(mutex);
        p = rawGetConnByPid(pid);
        pthread_mutex_unlock(mutex);
        return p;
    }

    void connObjMgr::DelConn(int fd, bool btimeOut){
        connObj *p= NULL;
        pthread_mutex_lock(mutex);
        map<int,int>::iterator iter = m_connFdMap.find(fd);
        if ( iter == m_connFdMap.end() ) {
            pthread_mutex_unlock(mutex);
            return;
        }

        p = rawGetConnByPid(iter->second);
        if(!p){
            return;
        }

        m_connFdMap.erase(iter);
        
        iter = m_writeFdMap.find(p->GetFd());
        if( iter != m_writeFdMap.end()){
            m_writeFdMap.erase(iter);
        }

        map<int,connObj*>::iterator it = m_connMap.find(p->GetPid());
        if ( it != m_connMap.end() ){
            m_connMap.erase(it);
        }
        map<int,string>::iterator it1 = m_connPidGameMap.find(p->GetPid());
        if(it1 != m_connPidGameMap.end()){
            m_connPidGameMap.erase(it1);
        }
        pthread_mutex_unlock(mutex);
        p->OnClose(btimeOut);
        printf("connObjMgr delconn, fd: %d pid: %d\n", fd, p->GetPid());
        delete p;
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

    void connObjMgr::AddWriteConn(int fd, connObj* p){
        m_writeFdMap[fd] = p->GetPid();
    }

    void connObjMgr::processAllWrite(){
        pthread_mutex_lock(mutex);
        for(map<int,int>::iterator iter = m_writeFdMap.begin(); iter!=m_writeFdMap.end(); /*iter++*/){
            connObj* p = rawGetConnByPid(iter->second);
            if(!p){
                m_writeFdMap.erase(iter++);
                continue;
            }
            int ret = p->send( NULL, 0 );
            if( ret == 0 ){
                m_writeFdMap.erase(iter++);
                continue;
            }
            iter++;
        }
        pthread_mutex_unlock(mutex);
    }


    void connObjMgr::SendMsg(unsigned int pid, unsigned char* pmem, unsigned int size){
        connObj *p = GetConnByPid(pid);
        if(p==NULL){
            printf("[ERROR] connObjMgr::SendMsg failed pid: %d len: %d\n", pid, size );
        }else{
            p->send( pmem, size );
        }
    }

    void connObjMgr::SendMsgAll(unsigned char* pmem, unsigned int size){
        pthread_mutex_lock(mutex);
        for(map<int,int>::iterator iter = m_connFdMap.begin(); iter!=m_connFdMap.end(); /*iter++*/){
            connObj* p = rawGetConnByPid( iter->second );
            if(!p){
                m_connFdMap.erase(iter++);
                continue;
            }
            p->send( pmem, size );
            iter++;
        }            
        pthread_mutex_unlock(mutex);
    }

    void connObjMgr::RegGamePid(unsigned int pid, char* pgame){
        m_connPidGameMap[pid] = string(pgame);
    }

    string connObjMgr::getGameByPid(int pid){
        map<int, string>::iterator it = m_connPidGameMap.find(pid);
        if( it==m_connPidGameMap.end()){
            return NULL;
        }
        return it->second;
    }
}
