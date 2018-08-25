# compiler to use.
CXX = g++

# compiler compilation flags.
CXXFLAGS = -std=c++11 -Isrc -IC:/Workspace/api/vorbis-1.3.6/include -IC:/Workspace/api/ogg-1.3.3/include -IC:/Workspace/api/openal-soft-1.18.2/include

# linker flags.
LDFLAGS = -LC:/Workspace/api/ogg-1.3.3/lib64 -LC:/Workspace/api/vorbis-1.3.6/lib64 -LC:/Workspace/api/openal-soft-1.18.2/lib64 -lopenal32 -logg.dll -lvorbis.dll -lvorbisfile.dll

# the path to source files.
SRC_PATH = .

# the path to build files and executable.
BUILD_PATH = build

# the set of source files from the source file folder.
SRCS = $(wildcard $(SRC_PATH)/*.cpp)

# the set of object files based on the resolved source files.
OBJS = $(SRCS:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)

# the rule to compile from source to object files.
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

# the rule to compile the executable.
all: $(OBJS)
	$(CXX) -o $(BUILD_PATH)/test.exe $(OBJS) $(CXXFLAGS) $(LDFLAGS)
