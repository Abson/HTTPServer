run: web_server.o criticalsection.o event.o http_conn.o ipaddress.o socketaddress.o 
	clang++ -std=c++11 -pthread -g criticalsection.o event.o http_conn.o ipaddress.o socketaddress.o  web_server.o -o web_server.out

web_server.o: web_server.cpp
	clang++ -std=c++11 -g -c web_server.cpp

criticalsection.o: criticalsection.cpp
	clang++ -std=c++11 -g -c criticalsection.cpp

event.o: event.cpp
	clang++ -std=c++11 -g -c event.cpp

http_conn.o: http_conn.cpp
	clang++ -std=c++11 -g -c http_conn.cpp

ipaddress.o: ipaddress.cpp
	clang++ -std=c++11 -g -c ipaddress.cpp
	
socketaddress.o: socketaddress.cpp
	clang++ -std=c++11 -g -c socketaddress.cpp

clean: 
	rm *.o
	rm *out
