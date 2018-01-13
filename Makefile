RM = rm -rf
MKDIR = mkdir -p

#CXX = g++
CXXFLAGS += -std=c++14 -c -Wall -O2
LDFLAGS = -ldl -lpthread -lrt -lgnutls

DEFS = POSIX
DEFINES = $(patsubst %, -D%, $(DEFS) )

SOURCEDIR = ./src
BUILDDIR = ./build
OBJDIR = $(BUILDDIR)/obj

SOURCES = $(shell find $(SOURCEDIR) -type f -name '*.cpp')
OBJECTS = $(patsubst $(SOURCEDIR)/%.cpp, $(OBJDIR)/%.o, $(SOURCES) )

EXECUTABLE = $(BUILDDIR)/httpserver

.PHONY: all clean
all: $(BUILDDIR) $(EXECUTABLE)

$(BUILDDIR):
	$(MKDIR) $@

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJECTS) : $(OBJDIR)/%.o : $(SOURCEDIR)/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(DEFINES) $(CXXFLAGS) $< -o $@

clean:
	$(RM) $(OBJDIR) $(EXECUTABLE)
