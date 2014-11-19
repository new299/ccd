
rx_ccd: *.cpp
	g++ rx_ccd.cpp -luhd -lSDL -lpthread -o rx_ccd

rx: *.cpp
	g++ rx.cpp -luhd -o rx

rx_samples_to_file: *.cpp
	g++ rx_samples_to_file.cpp -luhd -lboost_system -lboost_program_options -lboost_thread -lpthread -o rx_samples_to_file
