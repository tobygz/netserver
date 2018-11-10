release:
	g++ -O2 main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets  -ltcmalloc -L./lib

debug:
	g++ -g main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets 

clean:
	rm -rf nets
