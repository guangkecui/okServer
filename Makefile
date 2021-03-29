main : main.cpp  ./locker/locker.cpp server.cpp ./threadpool/threadpool.cpp ./http_conn/http_conn.cpp
	g++ -o main $^ -lpthread -std=gnu++0x
clean :
	rm -r main

