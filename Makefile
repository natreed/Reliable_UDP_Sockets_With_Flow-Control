Main: Client.cpp Server.cpp utilities.h
	g++ -g -o Client -std=c++11 -pthread Client.cpp 
	g++ -g -o Server -std=c++11 -pthread Server.cpp
	g++ -std=c++11 -pthread utilities.h
	g++ -std=c++11 -pthread ctrl_win.h
	g++ -std=c++11 -pthread Server.h


