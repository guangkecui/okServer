main : main.cpp  ./locker/locker.cpp server.cpp ./threadpool/threadpool.cpp ./timer/timermanage.cpp ./http_conn/http_conn.cpp
	g++ -o main $^ -lpthread -std=gnu++0x -g
clean:
	rm -r mian
