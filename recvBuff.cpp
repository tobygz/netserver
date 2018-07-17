#include "recvBuff.h"
#include <arpa/inet.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <string>


#define OUTPUT_FUN printf
#define CHUNK 1024

namespace net{

    /* msgObj */
    unsigned long long g_uid = 0;
    msgObj::msgObj(unsigned int* msgid, unsigned int* plen, unsigned char* p){
        m_uid = g_uid++;
        m_pmsgId = msgid;
        m_pbodyLen = plen;
        m_pBody = p;
    }
    msgObj::~msgObj(){
    }

    void msgObj::update(){
        OUTPUT_FUN("[NET] msgObj uid: %d update msgid: %d bodylen: %d\n",m_uid, *m_pmsgId, *m_pbodyLen );
    }

    int msgObj::size(){
        return *m_pbodyLen+8;
    }

    /* rpcObj */
    rpcObj::rpcObj(){
        m_pLen = NULL;
        m_pbody = NULL;

        m_pMsgType = NULL;

        memset(m_pTarget,0,sizeof(m_pTarget));
        memset(m_pKey,0,sizeof(m_pKey));
        memset(m_pParam,0,sizeof(m_pParam));
        memset(m_pResult,0,sizeof(m_pResult));
        
        m_pPid = NULL;
        m_pMsgid = NULL;
        m_pBody = NULL; 
        m_pBodyLen = NULL;
    }

    void rpcObj::update(){
        if(strcmp((const char*)m_pTarget,"ping")==0){
            printf("recv ping rpc\n");
        }else{
            printf("recv %s rpc\n", m_pTarget);
        }
    }

    void rpcObj::ToString(){
        char info[1024] = {0};
        sprintf(info,"target: %s key: %s param: %s result: %s msgid: %d bodylen: %d\n", m_pTarget, m_pKey,m_pParam,m_pResult, *m_pMsgid,*m_pBodyLen);
        printf("[rpcobj] tostring: %s", info);

    }
    void rpcObj::decodeBuffer(char* p){
        m_pLen = (int*)p;
        m_pbody = (unsigned char*)(p+ sizeof(int));

        //msgtype
        m_pMsgType = m_pbody;

        //key
        unsigned char *plen = (unsigned char*)(m_pbody+1);
        memcpy(m_pKey, m_pbody+2, *plen);

        //target
        unsigned char *plenTar = (unsigned char*)(m_pbody+2+*plen);
        memcpy(m_pTarget,m_pbody+3+*plen, *plenTar);

        //param
        unsigned char *plenPa = (unsigned char*)(m_pbody+3+*plen + *plenTar);
        memcpy(m_pParam,m_pbody+4+*plen+*plenTar, *plenPa);

        //result
        unsigned char *plenRes = (unsigned char*)(m_pbody+4+*plen + *plenTar + *plenPa);
        memcpy(m_pResult,m_pbody+4+*plen+*plenTar+*plenPa, *plenRes);

        m_pPid = (unsigned long long*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes);
        m_pMsgid= (unsigned int*)(m_pbody+5+*plen + *plenTar + *plenPa +*plenRes+ sizeof(unsigned long long));

        m_pBodyLen = (unsigned int*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes + sizeof(unsigned long long) + sizeof(unsigned int));
        m_pBody= (unsigned char*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes + sizeof(unsigned long long) + sizeof(unsigned int) + sizeof(unsigned int) );

    }

    unsigned int rpcObj::encodeBuffer(unsigned char* p,char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        unsigned char *pSize = (unsigned char* )p;

        const char* param = (const char*)"net1";
        const char* result = (const char*)"defres";        
        const char* key = (const char*)"";        

        unsigned char msgtp = 0;
        //msgtype
        memcpy(p+sizeof(int), &msgtp, 1);

        //key
        unsigned char len = strlen(key);
        memcpy( p+sizeof(int)+1, &len, 1 );
        memcpy( p+sizeof(int)+2, key, len );
        //ignore cpy keyval, for it's empty
        
        //target
        unsigned char len1 = strlen(target);
        memcpy( p+sizeof(int)+2+len, &len1, 1 );
        memcpy( p+sizeof(int)+3+len, target, len1 );

        //param
        unsigned char lenpa = strlen(param);
        memcpy( p+sizeof(int)+3+len+len1, &lenpa, 1 );   
        memcpy( p+sizeof(int)+4+len+len1, param, lenpa );  

        //res
        unsigned char lenres = strlen(result);
        memcpy( p+sizeof(int)+4+len1+len+lenpa, &lenres, 1 );   
        memcpy( p+sizeof(int)+5+len1+len+lenpa, result, lenres );  

        //pid
        memcpy( p+sizeof(int)+5+len1+len+lenpa+lenres, &pid, sizeof(pid) );  
        //msgid        
        memcpy( p+sizeof(int)+5+len1+len+lenpa+lenres+sizeof(pid), &msgid, sizeof(msgid) );  
        
        //bodylen
        memcpy( p+sizeof(int)+5+len1+len+lenpa+lenres+sizeof(pid)+sizeof(msgid), &byteLen, sizeof(byteLen) ); 
        //body
        memcpy( p+sizeof(int)+5+len1+len+lenpa+lenres+sizeof(pid)+sizeof(msgid)+sizeof(byteLen), pbyte, byteLen ); 

        unsigned int size = sizeof(int)+5+len1+len+lenpa+lenres+sizeof(pid)+sizeof(msgid)+sizeof(byteLen)+byteLen;
        memcpy(pSize, &size, sizeof(size));
        return size + sizeof(size);
    }


    recvBuff::~recvBuff(){
        memset( m_pMem, 0, DEFAULT_RECV_SIZE );
        if(m_pBuffer != m_pMem){
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
    }

    //内存部分不包括包的大小
    recvBuff::recvBuff( int size )
        :m_bCanUnPack(true)
    {
        memset( m_pMem, 0, DEFAULT_RECV_SIZE );
        if( size > DEFAULT_RECV_SIZE )
        {
            m_pBuffer = new char[size];
        }else{
            m_pBuffer = m_pMem;
        }
        m_iBuffSize = size;
        m_pPointer = m_pBuffer;
        m_eType = eRecvType_data;
    }



    bool recvBuff::unpack(){
        if(!m_bCanUnPack){
            OUTPUT_FUN("[NET] clientSocket 重复解包！");
            return false;
        }
        m_bCanUnPack = false;

        std::string output;

        int body_offset = sizeof(short);

        short pt = *((short*) (m_pBuffer + 4));


        if(m_pBuffer != m_pMem){
            delete[] m_pBuffer;
        }

        m_iBuffSize = output.size() + body_offset;

        m_pBuffer = (char*)malloc(m_iBuffSize);

        m_pPointer = m_pBuffer + body_offset;

        memcpy(m_pBuffer, &pt, sizeof(short));

        memcpy(m_pBuffer + body_offset, output.c_str(), output.size());

        return true;
    }

    void    recvBuff::getMem( char* &pMem ) const
    {
        pMem = m_pBuffer;
    }


    char    recvBuff::readChar()
    {
        m_bCanUnPack = false;
        assert( m_pPointer+sizeof(char) <= m_pBuffer+m_iBuffSize );
        char *bret = (char*) m_pPointer;
        m_pPointer += sizeof( char );
        return *bret;
    }

    short   recvBuff::readShort()
    {
        assert( m_pPointer+sizeof(short) <= m_pBuffer+m_iBuffSize );
        short *bret = (short*) m_pPointer;
        m_pPointer += sizeof( short );
        return ntohs(*bret);
    }

    int     recvBuff::readInt()
    {
        assert( m_pPointer+sizeof(int) <= m_pBuffer+m_iBuffSize );
        int *bret = (int*) m_pPointer;
        m_pPointer += sizeof( int );
        return ntohl(*bret);
    }

    int     recvBuff::readString( char* &pStr )
    {
        short len = readShort();
        assert( m_pPointer+len <= m_pBuffer+m_iBuffSize );
        pStr = m_pPointer;
        m_pPointer += len;
        return len;
    }

}
