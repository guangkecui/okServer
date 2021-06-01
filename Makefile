main : main.cpp  ./locker/locker.cpp ./sql/sqlpool.cpp ./hash/MurmurHash3.cpp ./redis/myredis.cpp ./sql/myDB.cpp server.cpp ./threadpool/threadpool.cpp ./timer/timermanage.cpp ./http_conn/http_conn.cpp
	g++ -o main $^ -lpthread -std=gnu++0x `mysql_config --cflags --libs` -L/usr/local/lib/ -lhiredis
clean:
	rm -r mian
