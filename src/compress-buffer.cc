/* node-compress-buffer (C) 2011, 2012 Egor Egorov <me@egorfine.com>  */

#include <node.h>
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

#define CHUNK 1024*100

z_stream strmCompress;

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
	}

	if (!Buffer::HasInstance(args[0])) {
		ThrowNodeError("First argument must be a Buffer");
		return Undefined();
	}

	Local<Object> bufferIn=args[0]->ToObject();
	bytesIn=Buffer::Length(bufferIn);

	dataPointer=Buffer::Data(bufferIn);

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

	z_stream strmUncompress;

	strmUncompress.zalloc=Z_NULL;
	strmUncompress.zfree=Z_NULL;
	strmUncompress.opaque=Z_NULL;
	strmUncompress.avail_in = 0;
	strmUncompress.next_in = Z_NULL;

	int rci = inflateInit2(&strmUncompress, WBITS);

	if (rci != Z_OK) {
		ThrowNodeError("zlib initialization error.");
		return Undefined();
	}

	Local<Object> bufferIn=args[0]->ToObject();

	strmUncompress.next_in = (Bytef*) Buffer::Data(bufferIn);
	strmUncompress.avail_in = Buffer::Length(bufferIn);

	Bytef  *bufferOut = NULL;
	uint32_t malloc_size=0;
	uint32_t currentPosition=0;

	int ret;

	do {
		Bytef *tmp = (Bytef*)malloc(CHUNK);
		strmUncompress.avail_out = CHUNK;
		strmUncompress.next_out = tmp;

		ret = inflate(&strmUncompress, Z_NO_FLUSH);
		assert(ret != Z_STREAM_ERROR);
		switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strmUncompress);
				if (bufferOut!=NULL) {
					free(bufferOut);
				}
				if (tmp!=NULL) {
					free(tmp);
				}
				return Undefined();
		}

		uint32_t have = CHUNK - strmUncompress.avail_out;
		if (have>0) {
			bufferOut = (Bytef *) realloc(bufferOut, malloc_size+have);
			malloc_size=malloc_size+have;
		}

		memcpy(bufferOut+currentPosition, tmp, have);
		currentPosition+=have;
		free(tmp);
	} while (strmUncompress.avail_out == 0 && ret != Z_STREAM_END);

	inflateEnd(&strmUncompress);

	if (ret != Z_STREAM_END) {
		if (bufferOut!=NULL) {
			free(bufferOut);
		}
		return Undefined();
	}

	Buffer *BufferOut=Buffer::New((char *)bufferOut, malloc_size);
	free(bufferOut);

	HandleScope scope;
	return scope.Close(BufferOut->handle_);
}

extern "C" void
init (Handle<Object> target) {
	strmCompress.zalloc=Z_NULL;
	strmCompress.zfree=Z_NULL;
	strmCompress.opaque=Z_NULL;

	int rcd = deflateInit2(&strmCompress, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
		WBITS, 8L,  Z_DEFAULT_STRATEGY);

	if (rcd != Z_OK) {
		ThrowNodeError("zlib initialization error.");
		return;
	}

	NODE_SET_METHOD(target, "compress", compress);
	NODE_SET_METHOD(target, "uncompress", uncompress);
}

NODE_MODULE(compress_buffer_bindings, init);
