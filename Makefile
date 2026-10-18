SYSROOT  = $(shell xcrun --show-sdk-path)
CXX      = clang++
CXXFLAGS += -std=c++17
LIBS += -lraylib
LDFLAGS  = -arch arm64 -isysroot $(SYSROOT) -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

main:
	$(CXX) $(CXXFLAGS) *.cpp $(LDFLAGS) -o main

SRCS = main.cpp Cactus.cpp Mushroom.cpp Knife.cpp SaveSystem.cpp Animation.cpp AssetManager.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)