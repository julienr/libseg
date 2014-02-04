CFLAGS=`pkg-config --cflags opencv` -Wall -Werror --std=c++11
LDFLAGS=`pkg-config --libs opencv` -L/home/julien/tm/v2/libs/_install/lib -lglog

all:
	g++ $(CFLAGS) -o main main.cc kde.cc $(LDFLAGS)
