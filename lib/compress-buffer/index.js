/* node-compress-buffer (C) 2011 Egor Egorov <me@egorfine.com>  */

NodeCompressBufferLL = require('./compress-buffer-bindings');

exports.compress = function(buffer, compressionLevel) {
	return NodeCompressBufferLL.compress(buffer, compressionLevel);
}

exports.uncompress = function(buffer) {
	return NodeCompressBufferLL.uncompress(buffer);
}
