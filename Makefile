
release:
	g++ main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets 

debug:
	g++ -g main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets 

profile:
	g++ -g -fno-inline  main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets 


clean:
	rm -rf nets
