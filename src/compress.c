#include <stdio.h>
#include <strings.h>
#include <zlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define SIZE 1024*1024*20

int decompress_memory(char *dest, size_t *bytesUncompressed, char *source, size_t sourceLen) {
	int err;

	z_stream strm;
	strm.zalloc = 0;
	strm.zfree = 0;
	strm.next_in = source;
	strm.avail_in = sourceLen;
	strm.next_out = dest;
	strm.avail_out = *bytesUncompressed;
	if ((uLong)strm.avail_out != *bytesUncompressed) {
		return Z_BUF_ERROR;
	}

	err = inflateInit2(&strm, 16 + MAX_WBITS);
	if (err != Z_OK) return err;

	err = inflate(&strm, Z_FINISH);
	if (err != Z_STREAM_END) {
		inflateEnd(&strm);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && strm.avail_in == 0)) { 
			return Z_DATA_ERROR;
		}
		return err;
	}
	*bytesUncompressed = strm.total_out;

	err = inflateEnd(&strm);
	return err;
}

int compress_memory(char *inBuffer, char *outBuffer, size_t inBufferSize, size_t *bytesCompressed) {
	int err;
	
	z_stream strm;
	strm.zalloc = 0;
	strm.zfree = 0;
	strm.next_in = inBuffer;
	strm.avail_in = inBufferSize;
	strm.next_out = outBuffer;
	strm.avail_out = *bytesCompressed;
	if ((uLong)strm.avail_out != *bytesCompressed) {
		return Z_BUF_ERROR;
	}

	err = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,  16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		return err;
	}

	err = deflate(&strm, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&strm);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*bytesCompressed = strm.total_out;

	err = deflateEnd(&strm);
	return err;
}

int main(void) {
	char *buffer = malloc(SIZE);
	FILE *in = fopen("/Users/egor/testdata", "r");
	size_t bytesRead = fread(buffer, 1, SIZE, in);
	printf("read %d bytes\n", bytesRead);
	
	size_t bytesCompressed=compressBound(bytesRead);
	char *bufferOut=malloc(bytesCompressed);

	compress_memory(buffer, bufferOut, bytesRead, &bytesCompressed);
	bufferOut = realloc(bufferOut, bytesCompressed);
	
	printf("Total comp size = %d bytes\n", bytesCompressed);
	FILE *out = fopen("/Users/egor/testdata.gz", "w");
	size_t bytesWritten = fwrite(bufferOut, 1, bytesCompressed, out);
	fclose(out);

	size_t bytesDest=SIZE;
	char *bufferDest = malloc(SIZE);
	
	int res = decompress_memory(bufferDest, &bytesDest, bufferOut, bytesCompressed);
	printf("REs=%d, uncompressed size=%d\n", res, bytesDest);
	out = fopen("/Users/egor/testdata-u", "w");
	bytesWritten = fwrite(bufferDest, 1, bytesDest, out);
	fclose(out);
	
}
