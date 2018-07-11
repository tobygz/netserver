#include "sendBuff.h"
#include <arpa/inet.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include <string>
//#include "zlib.h"

#define OUTPUT_FUN printf

#define CHUNK 1024

sendBuff::sendBuff()
:m_bCanWrite(true)
{
    memset( m_pMem, 0, DEFAULT_MEM_SIZE );
    m_pBuffer = m_pMem;
    m_iBuffSize = DEFAULT_MEM_SIZE;
    m_pPointer = m_pMem+4;
}

sendBuff::~sendBuff(){
    if(m_pBuffer && m_pBuffer != m_pMem){
        //printf("==============> delete pmem addr: 0x%08x\r\n", m_pBuffer );
        delete[] m_pBuffer;
        m_pBuffer = NULL;
    }
}

short   sendBuff::get_pt(){
    short *pt = (short*)(m_pBuffer + 4);
    return ntohs(*pt);
}

bool sendBuff::pack(int level){
    std::string output;
    
    int body_offset = 4 + sizeof(short);
    
    //Compress((unsigned char*)(m_pBuffer+body_offset), m_iBuffSize-body_offset, &output, level);
    
    short pt = *((short*) (m_pBuffer + 4));

    if(m_pBuffer != m_pMem){
        delete[] m_pBuffer;
    }
    
    m_iBuffSize = output.size() + body_offset;
    m_pBuffer = (char*)malloc(m_iBuffSize);
    
    memset(m_pBuffer, 0, body_offset );
    
    memcpy(m_pBuffer+4, &pt, sizeof(short));
    
    memcpy(m_pBuffer+body_offset, output.c_str(), output.size());
    
    m_pPointer = m_pBuffer + m_iBuffSize;
    
    m_bCanWrite = false;
    
    return true;
}

/*
 *  检查内存是否够用， 不够用就X2增长
 */
void sendBuff::check_mem( int add_size ){
    //内存够用
    if( (m_pPointer + add_size) < (m_pBuffer + m_iBuffSize) ){
        return;
    }

    //申请两倍内存
    m_iBuffSize = m_iBuffSize*2 + add_size;
    char *m_new= new char[m_iBuffSize];
    memset( m_new, 0, m_iBuffSize );
    //copy之前内部
    memcpy( m_new, m_pBuffer, m_pPointer-m_pBuffer ); 
    //清理内存
    if( m_pBuffer != m_pMem ) {
        delete[] m_pBuffer;
    }else{
        memset( m_pBuffer, 0, DEFAULT_MEM_SIZE );
    }
    //移指针
    m_pPointer = m_new + (m_pPointer-m_pBuffer);
    m_pBuffer = m_new;
}

void    sendBuff::get_mem( char* &pMem, int& size )
{
    //assert( m_pBuffer );
    size = m_pPointer - m_pBuffer;
    *(int*)m_pBuffer = htonl(size-4);
    pMem = m_pBuffer;
}

void    sendBuff::writeChar( const char val )
{
    if (!m_bCanWrite){
        //*
        OUTPUT_FUN("[NET] clientSocket 在打包之后调用了包写入！");
        return;
    }
    
    check_mem( sizeof(char) );
    memcpy(m_pPointer, &val, sizeof(char) );
    m_pPointer += sizeof(char);
}

void    sendBuff::writePt( const short val )
{
    if (!m_bCanWrite){
        //*
        OUTPUT_FUN("[NET] clientSocket 在打包之后调用了包写入！");
        return;
    }
    
    //m_iPt = val;
    writeShort( val );
}

void    sendBuff::writeShort( const short val )
{
    if (!m_bCanWrite){
        //*
        OUTPUT_FUN("[NET] clientSocket 在打包之后调用了包写入！");
        return;
    }
    
    check_mem( sizeof(short) );
    short rval = htons( val );
    memcpy(m_pPointer, &rval, sizeof(short) );
    m_pPointer += sizeof(short);
}

void    sendBuff::writeInt( const int val )
{
    if (!m_bCanWrite){
        //*
        OUTPUT_FUN("[NET] clientSocket 在打包之后调用了包写入！");
        return;
    }
    
    check_mem( sizeof(int) );
    int rval = htonl( val );
    memcpy(m_pPointer, &rval, sizeof(int) );
    m_pPointer += sizeof(int);
}

void    sendBuff::writeString( const char* pStr, short len )
{
    if (!m_bCanWrite){
        //*
        OUTPUT_FUN("[NET] clientSocket 在打包之后调用了包写入！");
        return;
    }
    
    writeShort( len );
    //printf("========>writeString len:%d\r\n", len );
    check_mem( len );
    memcpy(m_pPointer, pStr, len );
    m_pPointer += len;
}



