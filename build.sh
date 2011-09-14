#!/bin/bash

rm -rf build/*

node-waf configure $*
node-waf build

# different paths for node 0.4 and 0.5 ?.. 
for i in default Release
do
	[ -f ./build/$i/compress-buffer-bindings.node ] && cp ./build/$i/compress-buffer-bindings.node ./lib/compress-buffer/
done
