
#include "connmgr.h"
#include <pthread.h>
#include <stdio.h>


namespace net{
    connObjMgr* connObjMgr::g_pConnMgr = new connObjMgr;

    pthread_mutex_t *mutex ; 

    connObjMgr::connObjMgr(){
        maxSessid = 0;
        mutex = new pthread_mutex_t;
        pthread_mutex_init( mutex, NULL );
    }

    void connObjMgr::CreateConnBatch(queue<int>* vec) {
        pthread_mutex_lock(mutex);
        while(vec->empty()==false){
            int fd = (int) vec->front();
            vec->pop();
            connObj *pconn = new connObj(fd);
            pconn->SetPid(maxSessid++);
            m_connMap[pconn->GetPid()] = pconn;
            m_connFdMap[pconn->GetFd()] = pconn;
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
        pthread_mutex_unlock(mutex);
        if ( iter != m_connFdMap.end() ){
            return (connObj*) iter->second;
        }
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

    void connObjMgr::DelConn(int fd ){
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
            pst->OnClose();
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

}
