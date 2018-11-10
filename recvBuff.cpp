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
    unsigned long long g_uid = 0;
    msgObj::msgObj(int* pid, int* plen, char* p){
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
