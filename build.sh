#!/bin/bash

node-waf configure $*
node-waf build
cp ./build/default/compress-buffer-bindings.node ./lib/compress-buffer/
