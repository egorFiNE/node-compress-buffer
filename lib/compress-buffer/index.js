/* node-compress-buffer (C) 2011 Egor Egorov <me@egorfine.com>  */

// FIXME: how do I get rid of this instance and just create functions? 
NodeCompressBufferLL = require('./compress-buffer-bindings').NodeCompressBufferLL;
var __nodeCompressBufferLL = new NodeCompressBufferLL();

exports.compress = function(buffer, compressionLevel) {
	if (!(buffer instanceof Buffer)) {
		buffer = new Buffer(buffer);
	}
	if (buffer.length<=18) {
		return buffer; // just don't compress 
	}
	compressionLevel = compressionLevel || 5;
	
	return __nodeCompressBufferLL.compress(buffer, compressionLevel);
}

exports.uncompress = function(buffer) {
	if (!(buffer instanceof Buffer)) {
		buffer = new Buffer(buffer);
	}
	return __nodeCompressBufferLL.uncompress(buffer);
}
