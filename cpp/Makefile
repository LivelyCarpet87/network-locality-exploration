debug:
	rm -f ./task.bin
	g++ -std=c++20 -g -Wall -I. main.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -l sqlite3 -fopenmp -D_DEBUG -o task.bin

build:
	rm -f ./task.bin
	g++ -std=c++20 -g -Wall -O3 -I. main.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -l sqlite3 -fopenmp -o task.bin

fig_1_locality_graph_gen:
	rm -f ./figure_1_locality_graph_generator.bin
	g++ -std=c++20 -g -Wall -O3 -I. figure_1_locality_graph_generator.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -l sqlite3 -fopenmp -o figure_1_locality_graph_generator.bin

figure_1_distance_frequency_gen:
	rm -f ./figure_1_distance_frequency_generator.bin
	g++ -std=c++20 -g -Wall -O3 -I. figure_1_distance_frequency_generator.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -l sqlite3 -fopenmp -o figure_1_distance_frequency_generator.bin

figure_2_s_avg_calc:
	rm -f ./figure_2_s_avg_calculator.bin
	g++ -std=c++20 -g -Wall -O3 -I. figure_2_s_avg_calculator.cpp utils.cpp edgelist.cpp funcs.cpp network_metrics.cpp -l sqlite3 -fopenmp -o figure_2_s_avg_calculator.bin