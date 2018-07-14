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
    msgObj::msgObj(unsigned int* pid, unsigned int* plen, unsigned char* p){
        m_uid = g_uid++;
        m_pmsgId = pid;
        m_pbodyLen = plen;
        m_pBody = p;
    }
    void msgObj::update(){
        //OUTPUT_FUN("[NET] msgObj uid: %d update msgid: %d bodylen: %d\n",m_uid, m_msgId, m_bodyLen );
    }
    int msgObj::size(){
        return *m_pbodyLen+8;
    }

    /* rpcObj */
    rpcObj::rpcObj(){
        m_pLen = NULL;
        m_pbody = NULL;

        m_pMsgType = NULL;
        m_pTarget = NULL; //end with 0
        m_pKey = NULL;    //end with 0
        m_pParam= NULL;  //end with 0
        m_pResult=NULL; //end with 0
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

    void rpcObj::decodeBuffer(char* p){
        m_pLen = (int*)p;
        m_pbody = (unsigned char*)(p+ sizeof(int));

        //msg
        m_pMsgType = m_pbody;

        //key
        unsigned char *plen = (unsigned char*)(m_pbody+1);
        m_pKey = (unsigned char*)(m_pbody+2);

        //target
        unsigned char *plenTar = (unsigned char*)(m_pbody+2+*plen);
        m_pTarget = (unsigned char*)(m_pbody+3+*plen);

        //param
        unsigned char *plenPa = (unsigned char*)(m_pbody+3+*plen + *plenTar);
        m_pParam = (unsigned char*)(m_pbody+4+*plen + *plenTar);

        //result
        unsigned char *plenRes = (unsigned char*)(m_pbody+4+*plen + *plenTar + *plenPa);
        m_pResult = (unsigned char*)(m_pbody+5+*plen + *plenTar + *plenPa);

        m_pPid = (unsigned long long*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes);
        m_pMsgid= (unsigned int*)(m_pbody+5+*plen + *plenTar + *plenPa +*plenRes+ sizeof(unsigned long long));

        m_pBodyLen = (unsigned int*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes + sizeof(unsigned long long) + sizeof(unsigned int));
        m_pBody= (unsigned char*)(m_pbody+5+*plen + *plenTar + *plenPa + *plenRes + sizeof(unsigned long long) + sizeof(unsigned int) + sizeof(unsigned int) );


    }

    unsigned int rpcObj::encodeBuffer(unsigned char* p,char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen){
        unsigned char *pSize = (unsigned char* )p;

        unsigned char* param = (unsigned char*)"defparam";
        unsigned char* result = (unsigned char*)"defres";        

        unsigned char msgtp = 0;
        //msgtype
        memcpy(p+sizeof(int), &msgtp, 1);

        //key
        memcpy( p+sizeof(int)+1, &msgtp, 1 );
        //ignore cpy keyval, for it's empty
        
        //target
        unsigned char len = strlen(target);
        memcpy( p+sizeof(int)+2, &len, 1 );
        memcpy( p+sizeof(int)+3, target, len );

        //param
        unsigned char lenpa = strlen((const char*)param);
        memcpy( p+sizeof(int)+3+len, &lenpa, 1 );   
        memcpy( p+sizeof(int)+4+len, param, lenpa );  

        //res
        unsigned char lenres = strlen((const char*)result);
        memcpy( p+sizeof(int)+4+len+lenpa, &lenres, 1 );   
        memcpy( p+sizeof(int)+5+len+lenpa, result, lenres );  

        //pid
        memcpy( p+sizeof(int)+5+len+lenpa+lenres, &pid, sizeof(pid) );  
        //msgid        
        memcpy( p+sizeof(int)+5+len+lenpa+lenres+sizeof(pid), &msgid, sizeof(msgid) );  
        
        //bodylen
        memcpy( p+sizeof(int)+5+len+lenpa+lenres+sizeof(pid)+sizeof(msgid), &byteLen, sizeof(byteLen) ); 
        //body
        memcpy( p+sizeof(int)+5+len+lenpa+lenres+sizeof(pid)+sizeof(msgid)+sizeof(byteLen), pbyte, byteLen ); 

        unsigned int size = sizeof(int)+5+len+lenpa+lenres+sizeof(pid)+sizeof(msgid)+sizeof(byteLen)+byteLen;
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

        //memcpy(m_pBuffer, &size, sizeof(int));

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
