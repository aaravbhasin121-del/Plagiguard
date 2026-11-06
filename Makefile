CXX = clang++
CXXFLAGS = -x objective-c++ -std=c++20 -O3 -Wall -Wextra -Icpp/include
LDFLAGS = -lsqlite3 -lpthread -framework Foundation -framework PDFKit -framework Vision -framework AppKit -framework CoreGraphics

SRCDIR = cpp
BINDIR = bin

TARGET_SERVER = $(BINDIR)/plagiguard
TARGET_CLI = $(BINDIR)/plagiguard-cli
TARGET_TESTS = $(BINDIR)/plagiguard-tests

.PHONY: all clean test run cli

all: $(TARGET_SERVER) $(TARGET_CLI) $(TARGET_TESTS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET_SERVER): cpp/main.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) cpp/main.cpp $(LDFLAGS) -o $(TARGET_SERVER)

$(TARGET_CLI): cpp/cli_main.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) cpp/cli_main.cpp $(LDFLAGS) -o $(TARGET_CLI)

$(TARGET_TESTS): cpp/tests/test_runner.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) cpp/tests/test_runner.cpp $(LDFLAGS) -o $(TARGET_TESTS)

test: $(TARGET_TESTS)
	@echo "Running PlagiGuard Unit Test Suite..."
	@./$(TARGET_TESTS)

run: $(TARGET_SERVER)
	@echo "Launching PlagiGuard Desktop Server..."
	@./$(TARGET_SERVER)

cli: $(TARGET_CLI)
	@./$(TARGET_CLI)

clean:
	rm -rf $(BINDIR) plagiguard.db plagiguard.log

