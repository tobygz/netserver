#ifndef __HEADER_SEND_BUFFER__
#define __HEADER_SEND_BUFFER__

#define DEFAULT_MEM_SIZE 512

/*!
 * 内存统一在内部申请
 * 在析构函数中统一释放
 */

class sendBuff {
    private:
        char    m_pMem[ DEFAULT_MEM_SIZE ];
        char*   m_pBuffer;
        int     m_iBuffSize;
        char*   m_pPointer;
        bool    m_bCanWrite;
        //short   m_iPt;
        void    check_mem( int add_size );

    public:
        ~sendBuff();
        sendBuff();
    
        short   get_pt();

        void    get_mem( char* &pMem, int& size );
        void    writePt( const short val );
        void    writeChar( const char val );
        void    writeShort( const short val );
        void    writeInt( const int val );
        void    writeString( const char* pStr, short len );
    
        bool    pack(int level = 1);
};


#endif



