//https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#include "net.h"

using namespace net;

pthread_t InitSocketListen(char* port){
    if(netServer::g_netServer->initSock(port) != 0){
        exit(0);
    }
	pthread_t id;
	int i,ret;
	ret=pthread_create(&id,NULL, &netServer::netThreadFun , netServer::g_netServer);
	if(ret!=0){
		printf ("Create pthread error!\n");
		exit (1);
	}
	//pthread_join(id,NULL);	
	return id;
}

bool g_run = true;

void onQuit(int sigval){
    printf("called onQuit sigval: %d\n", sigval);
    if(sigval == SIGINT || sigval == SIGQUIT ) {
        g_run = false;
    }

}

int main(){

    signal(SIGINT, onQuit);
    signal(SIGQUIT, onQuit);

    char* port = (char*)"6010";
	pthread_t id = InitSocketListen(port);

	while(1){
        if(!g_run){
            break;
        }
        netServer::g_netServer->queueProcessFun();
		//usleep(100);
	}

    netServer::g_netServer->destroy();
	pthread_cancel(id);
	pthread_join(id, NULL);
	return 0;
}

