/* node-compress-buffer (C) 2011 Egor Egorov <me@egorfine.com>  */

#include <node.h>
#include <node_events.h>
#include <node_buffer.h>
#include <string.h>
#include <v8.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "zlib.h"
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

// we don't compress less than that. Actually, zlib refuses to compress less than 18 bytes, but 32 looks
// like a better number :) 
#define MIN_BYTES 32 

z_stream strmCompress;
z_stream strmUncompress;

// FIXME make uncompress cyclic so it eats not so much memory

using namespace v8;
using namespace node;

Handle<Value> compress(const Arguments& args) {
	HandleScope scope;
	size_t bytesIn=0;
	size_t bytesCompressed=0;
	char *dataPointer=NULL;
	int shouldFreeDataPointer=0;
	
	if (args.Length() < 1) { 
		return Undefined();
	}
	
	if (args[0]->IsString()) {
		String::AsciiValue string(args[0]->ToString());
		bytesIn = strlen(*string);
		
		if (bytesIn<=MIN_BYTES) {
			return scope.Close(args[0]);
		}
		
		// FIXME why do I have to copy data, why just this doesn't work: dataPointer = (char *) *string ? 
		dataPointer = (char*)malloc(bytesIn);
		strncpy(dataPointer, *string, bytesIn);
		shouldFreeDataPointer=1;
		
	} else if (Buffer::HasInstance(args[0])) {
		Local<Object> bufferIn=args[0]->ToObject();
		bytesIn=Buffer::Length(bufferIn);
		
		if (bytesIn<=MIN_BYTES) {
			return scope.Close(args[0]);
		}
		
		dataPointer=Buffer::Data(bufferIn);
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
	char *bufferOut=(char*) malloc(bytesCompressed);

	strmCompress.next_in=(Bytef*) dataPointer;
	strmCompress.avail_in=bytesIn;
	strmCompress.next_out=(Bytef*) bufferOut;
	strmCompress.avail_out=bytesCompressed;
	
	if (deflate(&strmCompress, Z_FINISH) != Z_STREAM_END) {
		deflateReset(&strmCompress);
		if (shouldFreeDataPointer) { 
			free(dataPointer);
		}
		return Undefined();
	}
	
	bytesCompressed=strmCompress.total_out;	
	deflateReset(&strmCompress);
	
	Buffer *BufferOut=Buffer::New(bufferOut, bytesCompressed);
	free(bufferOut);
	
	if (shouldFreeDataPointer) { 
		free(dataPointer);
	}

	return scope.Close(BufferOut->handle_);
}
	

Handle<Value> uncompress(const Arguments &args) {
	if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {
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

	deflateInit2(&strmCompress, Z_DEFAULT_COMPRESSION, Z_DEFLATED,  16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
	inflateInit2(&strmUncompress, 16 + MAX_WBITS);
	
	NODE_SET_METHOD(target, "compress", compress);
	NODE_SET_METHOD(target, "uncompress", uncompress);
}
