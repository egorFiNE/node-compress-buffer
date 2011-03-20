/* node-compress-buffer (C) 2011 Egor Egorov <me@egorfine.com>  */

NodeCompressBufferLL = require('./compress-buffer-bindings');

exports.compress = function(buffer, compressionLevel) {
	if (!(buffer instanceof Buffer)) {
		buffer = new Buffer(buffer);
	}
	if (buffer.length<=18) {
		return buffer; // just don't compress 
	}
	compressionLevel = compressionLevel || 5;
	
	return NodeCompressBufferLL.compress(buffer, compressionLevel);
}

exports.uncompress = function(buffer) {
	if (!(buffer instanceof Buffer)) {
		buffer = new Buffer(buffer);
	}
	return NodeCompressBufferLL.uncompress(buffer);
}
