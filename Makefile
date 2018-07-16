debug:
	rm -rf nets
	g++ -g *.cpp -lpthread -o nets 

release:
	g++ *.cpp -lpthread -o nets 


profile:
	g++ -g -fno-inline  main.cpp net.cpp conn.cpp connmgr.cpp recvBuff.cpp qps.cpp -lpthread -o nets 


clean:
	rm -rf nets
