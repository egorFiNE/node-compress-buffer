TESTS = test/*.js

all: test

build: clean configure compile

configure:
	node-waf configure

compile:
	node-waf build

test: build
	@nodeunit $(TESTS)

clean:
	rm -Rf build


.PHONY: clean test build
