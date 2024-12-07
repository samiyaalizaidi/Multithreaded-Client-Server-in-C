# Compiler and flags
CC = gcc
CFLAGS = -Wall -lm -lpthread -lcrypto

# Executable names
CLIENT_EXEC = client
SERVER_EXEC = server

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Test script
TEST_SCRIPT = tests.sh

# Default target
.PHONY: all
all: build

# Compile individual components
compile-client:
	$(CC) $(CLIENT_SRC) -o $(CLIENT_EXEC) $(CFLAGS)

compile-server:
	$(CC) $(SERVER_SRC) -o $(SERVER_EXEC) $(CFLAGS)

# Build both client and server
.PHONY: build
build: compile-client compile-server
	@echo "Build completed successfully!"

# Run the application and tests
.PHONY: run
run: build
	@chmod +x $(TEST_SCRIPT)
	@echo "Running tests..."
	./$(TEST_SCRIPT)

# Clean up generated files
.PHONY: clean
clean:
	@echo "Cleaning up build files..."
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)
	@echo "Cleanup completed!"
