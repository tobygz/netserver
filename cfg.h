#ifndef __HEADER_CFG__
#define __HEADER_CFG__
#include <stdio.h>
#include <string.h>
#include <vector>


/*

   21009
   gate1,"127.0.0.1",20000
   game1,"127.0.0.1",21020
   */

using namespace std;
namespace net{
    struct nodeCfg {
        char nodeName[32];
        char ip[32];
        char port[32];
    };

    class serverCfg {
        public:
            char listenPort[32];
            vector<nodeCfg*> nodeLst;

            static serverCfg *m_gInst;
            void tostring(char* p);
            serverCfg(){}
            void init();
            char* getListen(){ return listenPort; }
    };
}
#endif
