main : main.cpp  ./locker/locker.cpp ./sql/sqlpool.cpp ./sql/myDB.cpp server.cpp ./threadpool/threadpool.cpp ./timer/timermanage.cpp ./http_conn/http_conn.cpp
	g++ -o main $^ -lpthread -std=gnu++0x `mysql_config --cflags --libs`
clean:
	rm -r mian
