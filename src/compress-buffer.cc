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
#include "czstream.h"

// we don't compress less than that. Actually, zlib refuses to compress less than 18 bytes, but 32 looks
// like a better number :) 
#define MIN_BYTES 32 

zstream::CZStream<z_stream> strmCompress;
zstream::CZStream<z_stream> strmUncompress;

// FIXME make uncompress cyclic so it eats not so much memory

using namespace v8;
using namespace node;

Handle<Value> ThrowNodeError(const char* what = NULL)
{
    return ThrowException(Exception::Error(String::New(what)));
}

Handle<Value> compress(const Arguments& args) {
	HandleScope scope;
	size_t bytesIn=0;
	size_t bytesCompressed=0;
	char *dataPointer=NULL;
	
	if (args.Length() < 1) { 
		return Undefined();
	}
	
	if (args[0]->IsString()) {
		String::AsciiValue string(args[0]->ToString());
		bytesIn = string.length();
		
		if (bytesIn<=MIN_BYTES) {
			return scope.Close(args[0]);
		}
		
		dataPointer = strdup(*string);
		
	} else if (Buffer::HasInstance(args[0])) {
		Local<Object> bufferIn=args[0]->ToObject();
		bytesIn=Buffer::Length(bufferIn);
		
		if (bytesIn<=MIN_BYTES) {
			return scope.Close(args[0]);
		}
		
		dataPointer=Buffer::Data(bufferIn);
	}
	
	int compressionLevel = czstream::compression;
	if (args.Length() > 1) { 
		compressionLevel = args[1]->IntegerValue();
		if (compressionLevel <= 0 || compressionLevel > 9) {
			compressionLevel = czstream::compression;
		}
	}
	
    z_streamp stream = strmCompress;
	deflateParams(stream, compressionLevel, czstream::strategy);
	
	bytesCompressed=compressBound(bytesIn);
	char *bufferOut=(char*) malloc(bytesCompressed);

	stream->next_in=(Bytef*) dataPointer;
	stream->avail_in=bytesIn;
	stream->next_out=(Bytef*) bufferOut;
	stream->avail_out=bytesCompressed;
	
	if (deflate(stream, Z_FINISH) != Z_STREAM_END) {
		deflateReset(stream);
		if (dataPointer) {
			free(dataPointer);
		}
		return Undefined();
	}
	
	bytesCompressed=stream->total_out;
	deflateReset(stream);
	
	Buffer *BufferOut=Buffer::New(bufferOut, bytesCompressed);
	free(bufferOut);
	
	if (dataPointer) {
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

    z_streamp stream = strmUncompress;
	stream->next_in=(Bytef*) Buffer::Data(bufferIn);
	stream->avail_in=Buffer::Length(bufferIn);
	stream->next_out=(Bytef*) bufferOut;
	stream->avail_out=bytesUncompressed;
	
	if (inflate(stream, Z_FINISH) != Z_STREAM_END) {
		inflateReset(stream);
		free(bufferOut);
		return Undefined();
	}
	
	bytesUncompressed=stream->total_out;
	inflateReset(stream);

	Buffer *BufferOut=Buffer::New(bufferOut, bytesUncompressed);
	free(bufferOut);

	HandleScope scope;
	return scope.Close(BufferOut->handle_);
}

extern "C" void
init (Handle<Object> target) 
{
    int rcd = deflateInit2(strmCompress, zstream::compression, zstream::algorithm,  
                           zstream::wbits, zstream::memlevel,  zstream::strategy);
    int rci = inflateInit2(strmUncompress, zstream::wbits);

    if ( rcd != Z_OK || rci != Z_OK)
    {
        ThrowNodeError("zlib initialization error.");
    }

    NODE_SET_METHOD(target, "compress", compress);
    NODE_SET_METHOD(target, "uncompress", uncompress);
}

