CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lssl -lcrypto

TARGET = bff
SOURCES = main.cpp bff_commands.cpp bff_data_load.cpp bff_scan.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
