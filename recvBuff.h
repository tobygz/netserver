#ifndef __HEADER_RECV_BUFFER__
#define __HEADER_RECV_BUFFER__

#define DEFAULT_RECV_SIZE 512
/*!
 * 内存由外部申请，
 * 释放在内部的析构函数中
 */

namespace net{
    enum eRecvType {
        eRecvType_lost = 0,
        eRecvType_connsucc,
        eRecvType_connfail,
        eRecvType_data,
    };

    class msgObj{
        public:
        unsigned long long m_uid;
        int m_msgId;
        int m_bodyLen;
        char* m_pBody;
        msgObj(int id, int len, char* p);
        int size();
        void update();
    };

    class recvBuff  {
        private:
            char    m_pMem[ DEFAULT_RECV_SIZE ];
            char*   m_pBuffer;
            int     m_iBuffSize;
            char*   m_pPointer;
            bool    m_bCanUnPack;

            int m_Msgid;

        public:
            ~recvBuff();
            recvBuff( int size );

            eRecvType m_eType;

            int     getBodyLen() {return m_iBuffSize;}
            int     getMsgid() {return m_Msgid;}
            void    setMsgid(int val) {m_Msgid = val;}

            void    getMem( char* &pMem ) const;

            char    readChar();
            short   readShort();
            int     readInt();
            int     readString( char* &pStr);

            bool    unpack();

            static recvBuff* createLostRecvBuff() {
                recvBuff *pBuff = new recvBuff( 0 );
                pBuff->m_eType = eRecvType_lost;
                return pBuff;
            }

            static recvBuff* createConnfailRecvBuff() {
                recvBuff *pBuff = new recvBuff( 0 );
                pBuff->m_eType = eRecvType_connfail;
                return pBuff;
            }

            static recvBuff* createConnsuccRecvBuff() {
                recvBuff *pBuff = new recvBuff( 0 );
                pBuff->m_eType = eRecvType_connsucc;
                return pBuff;
            }


    };

}
#endif

