debug:
	rm -f ./task.bin
	g++ -std=c++20 -g -Wall -I. main.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -lpthread -l sqlite3 -fopenmp -D_DEBUG -o task.bin

build:
	rm -f ./task.bin
	g++ -std=c++20 -g -Wall -O3 -I. main.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -lpthread -l sqlite3 -fopenmp -o task.bin