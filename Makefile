main : main.cpp locker.cpp task.cpp threadpool.cpp
	g++ -o main $^ -lpthread
clean :
	rm -r main

