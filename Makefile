# Compiler
CC = g++ --std=c++17
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = 
LIBS = -lcurl
USE_MONGO ?= 0

# Detect OS
UNAME_S := $(shell uname -s)

ifeq ($(USE_MONGO),1)
    # Auto-detect MongoDB paths using pkg-config if available.
    ifneq ($(shell which pkg-config),)
        CFLAGS += $(shell pkg-config --cflags libmongoc-1.0 libbson-1.0 2>/dev/null)
        LDFLAGS += $(shell pkg-config --libs libmongoc-1.0 libbson-1.0 2>/dev/null)
    endif

    # Manual fallback if pkg-config is missing.
    ifeq ($(UNAME_S),Linux)
        CFLAGS += -I/usr/include/libmongoc-1.0 -I/usr/include/libbson-1.0
        LDFLAGS += -L/usr/lib
        LIBS += -lmongoc-1.0 -lbson-1.0
    endif

    ifeq ($(UNAME_S),Darwin)
        # Use brew to get correct paths dynamically.
        MONGO_PATH := $(shell brew --prefix mongo-c-driver 2>/dev/null)

        ifneq ($(MONGO_PATH),)
            CFLAGS += -I$(MONGO_PATH)/include/libmongoc-1.0 -I$(MONGO_PATH)/include/libbson-1.0
            LDFLAGS += -L$(MONGO_PATH)/lib -Wl,-rpath,$(MONGO_PATH)/lib
        else
            CFLAGS += -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0
            LDFLAGS += -L/usr/local/lib -Wl,-rpath,/usr/local/lib
        endif
        LIBS += -lmongoc-1.0 -lbson-1.0
    endif

    ifeq ($(OS),Windows_NT)
        CFLAGS += -I"C:/Program Files/mongo-c-driver/include/libmongoc-1.0" -I"C:/Program Files/mongo-c-driver/include/libbson-1.0"
        LDFLAGS += -L"C:/Program Files/mongo-c-driver/lib"
        LIBS += -lmongoc-1.0 -lbson-1.0 -lws2_32
    endif
endif

# Source files
SRC = src/caller/main.cpp \
    src/core/globals.cpp \
    src/core/database.cpp \
    src/core/table.cpp \
    src/handlers/sqlparser.cpp \
    src/handlers/join.cpp
OBJ = $(SRC:.cpp=.o)
OUT = main

# Build target
all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT) $(OBJ) $(LIBS)

ifeq ($(UNAME_S),Linux)
	@chmod +x $(OUT)
endif

ifeq ($(UNAME_S),Darwin)
	@chmod +x $(OUT)
endif

# Object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OUT) $(OBJ)

# Reset
reset:
	rm -rf Databases
