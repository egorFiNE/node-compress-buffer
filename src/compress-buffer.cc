/* node-compress-buffer (C) 2011 Egor Egorov <me@egorfine.com>  */

#include <node.h>
#include <node_events.h>
#include <node_buffer.h>
#include <string.h>
#include <v8.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#include <zlib.h>

// zlib magic something
#define WBITS 16+MAX_WBITS

z_stream strmCompress;
z_stream strmUncompress;

// FIXME make uncompress cyclic so it eats not so much memory

using namespace v8;
using namespace node;

Handle<Value> ThrowNodeError(const char* what = NULL) {
	return ThrowException(Exception::Error(String::New(what)));
}

Handle<Value> compress(const Arguments& args) {
	HandleScope scope;
	size_t bytesIn=0;
	size_t bytesCompressed=0;
	char *dataPointer=NULL;
	bool shouldFreeDataPointer=false;
	
	if (args.Length() < 1) { 
		return Undefined();

	} else if (Buffer::HasInstance(args[0])) {
		Local<Object> bufferIn=args[0]->ToObject();
		bytesIn=Buffer::Length(bufferIn);
		
		dataPointer=Buffer::Data(bufferIn);

	} else if (args[0]->IsString()) {
#ifdef SUPPORT_STRINGS
		String::AsciiValue string(args[0]->ToString());
		bytesIn = string.length();

		dataPointer = strdup(*string); 
		shouldFreeDataPointer = true;
#else 
		ThrowNodeError("First argument must be a Buffer");
		return Undefined();
#endif
	} 

	int compressionLevel = Z_DEFAULT_COMPRESSION;
	if (args.Length() > 1) { 
		compressionLevel = args[1]->IntegerValue();
		if (compressionLevel <= 0 || compressionLevel > 9) {
			compressionLevel = Z_DEFAULT_COMPRESSION;
		}
	}
	
	deflateParams(&strmCompress, compressionLevel, Z_DEFAULT_STRATEGY);
	
	bytesCompressed=compressBound(bytesIn);

	// compressBound mistakes when estimating extremely small data blocks (like few bytes), so 
	// here is the stub. Otherwise smaller buffers (like 10 bytes) would not compress.
	if (bytesCompressed<1024) {
		bytesCompressed=1024;
	}

	char *bufferOut=(char*) malloc(bytesCompressed);

	strmCompress.next_in=(Bytef*) dataPointer;
	strmCompress.avail_in=bytesIn;
	strmCompress.next_out=(Bytef*) bufferOut;
	strmCompress.avail_out=bytesCompressed;
	
	if (deflate(&strmCompress, Z_FINISH) != Z_STREAM_END) {
		deflateReset(&strmCompress);
		if (shouldFreeDataPointer) {
			free(dataPointer); 
			dataPointer = NULL;
		}
		return Undefined();
	}
	
	bytesCompressed=strmCompress.total_out;
	deflateReset(&strmCompress);
	
	Buffer *BufferOut=Buffer::New(bufferOut, bytesCompressed);
	free(bufferOut);
	
	if (shouldFreeDataPointer) {
		free(dataPointer); 
		dataPointer = NULL;
	}

	return scope.Close(BufferOut->handle_);
}
	

Handle<Value> uncompress(const Arguments &args) {
	if (args.Length() < 1) {
		return Undefined();
	}
	
	if (!Buffer::HasInstance(args[0])) {
		ThrowNodeError("First argument must be a Buffer");
		return Undefined();
	}
	
	Local<Object> bufferIn=args[0]->ToObject();

	size_t bytesUncompressed=999*1024*1024; // it's about max size that V8 supports
	char *bufferOut=(char*) malloc(bytesUncompressed);

	strmUncompress.next_in=(Bytef*) Buffer::Data(bufferIn);
	strmUncompress.avail_in=Buffer::Length(bufferIn);
	strmUncompress.next_out=(Bytef*) bufferOut;
	strmUncompress.avail_out=bytesUncompressed;
	
	if (inflate(&strmUncompress, Z_FINISH) != Z_STREAM_END) {
		inflateReset(&strmUncompress);
		free(bufferOut);
		return Undefined();
	}
	
	bytesUncompressed=strmUncompress.total_out;
	inflateReset(&strmUncompress);

	Buffer *BufferOut=Buffer::New(bufferOut, bytesUncompressed);
	free(bufferOut);

	HandleScope scope;
	return scope.Close(BufferOut->handle_);
}

extern "C" void
init (Handle<Object> target) {
	strmCompress.zalloc=Z_NULL;
	strmCompress.zfree=Z_NULL;
	strmCompress.opaque=Z_NULL;

	strmUncompress.zalloc=Z_NULL;
	strmUncompress.zfree=Z_NULL;
	strmUncompress.opaque=Z_NULL;

	int rcd = deflateInit2(&strmCompress, Z_DEFAULT_COMPRESSION, Z_DEFLATED,  
		WBITS, 8L,  Z_DEFAULT_STRATEGY);
	int rci = inflateInit2(&strmUncompress, WBITS);

	if (rcd != Z_OK || rci != Z_OK) {
		ThrowNodeError("zlib initialization error.");
		return;
	}

	NODE_SET_METHOD(target, "compress", compress);
	NODE_SET_METHOD(target, "uncompress", uncompress);
}
