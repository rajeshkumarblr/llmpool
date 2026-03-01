CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I/usr/local/include -I/usr/local/include $(shell pkg-config --cflags libcurl jsoncpp)
LDFLAGS = -lcpr $(shell pkg-config --libs libcurl jsoncpp) -lssl -lcrypto
TARGET = pipeline
SRC = pipeline.cpp
HEADERS = llmpool.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
