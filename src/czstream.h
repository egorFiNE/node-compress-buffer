#ifndef _CZSTREAM_H_
#define _CZSTREAM_H_

#include <zlib.h>

namespace zstream {

enum {compression =   Z_DEFAULT_COMPRESSION};
enum {algorithm   =   Z_DEFLATED};
enum {strategy    =   Z_DEFAULT_STRATEGY};
enum {wbits       =   16 + MAX_WBITS};
enum {memlevel    =   8L};

template<typename Stream> class CZStream {
public:
	CZStream() {
		stream_.zalloc = Z_NULL;
		stream_.zfree  = Z_NULL;
		stream_.opaque = Z_NULL;
	}

	~CZStream() {
	}

	operator Stream*() const { 
		return const_cast<Stream*>(&stream_); 
	}

private:
	Stream stream_;
};

} //namespace zstream

#endif //_CZSTREAM_H_

