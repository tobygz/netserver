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
        unsigned long long m_uid;
        unsigned int *m_pmsgId;
        unsigned int *m_pbodyLen;
        unsigned char* m_pBody;        
        public:

        msgObj(unsigned int* pid, unsigned int* plen, unsigned char* p);
        unsigned int getMsgid(){ return *m_pmsgId;}
        unsigned int getBodylen(){ return *m_pbodyLen;}
        unsigned char* getBodyPtr(){return m_pBody;}
        int size();
        void update();
    };

    class rpcObj{
            int *m_pLen;
            unsigned char *m_pbody;

            unsigned char *m_pMsgType;
            unsigned char *m_pTarget; //end with 0
            unsigned char *m_pKey;    //end with 0
            unsigned char *m_pParam;  //end with 0
            unsigned char *m_pResult; //end with 0
            unsigned long long *m_pPid;
            unsigned int *m_pMsgid;
            unsigned char *m_pBody; 
            unsigned int *m_pBodyLen;

        public:
            rpcObj();
            void decodeBuffer(char* p);
            static unsigned int encodeBuffer(unsigned char* p,char* target, unsigned long long pid, unsigned int msgid, unsigned char* pbyte, unsigned int byteLen);
            void update();

            unsigned char getMsgType(){return *m_pMsgType;}
            unsigned char* getTarget(){ return m_pTarget;}
            unsigned char* getParam(){ return m_pParam;}
            unsigned char* getResult(){ return m_pResult;}
            unsigned long long getPid(){ return *m_pPid;}
            unsigned int getMsgid(){ return *m_pMsgid;}
            unsigned int getBodylen(){ return *m_pBodyLen;}
            unsigned char* getBodyPtr(){ return m_pBody;}
            
            
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
    };

}
#endif

