main : main.cpp locker/locker.cpp task/task.cpp threadpool/threadpool.cpp
	g++ -o main $^ -lpthread
clean :
	rm -r main

