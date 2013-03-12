TESTS = test/*.js

all: test

build: clean configure compile

configure:
	node-gyp configure

compile: configure
	node-gyp build

test: build
	@nodeunit $(TESTS)

clean:
	node-gyp clean


.PHONY: clean test build
