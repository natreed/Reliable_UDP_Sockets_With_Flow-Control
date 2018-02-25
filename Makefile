Main: Client.cpp Server.cpp utilities.h
	g++ -g -o Client -std=c++11 Client.cpp 
	g++ -g -o Server -std=c++11 Server.cpp
	g++ -std=c++11 utilities.h

