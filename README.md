# node-compress-buffer 

A single-step Buffer compression library for Node.js.

## Synopsis

  compress = require('compress-buffer').compress;
	uncompress = require('compress-buffer').uncompress;
	
	var uncompressed = fs.readFileSync("/etc/passwd");
	var compressed = compress(uncompressed);

	uncompressed = uncompress(compressed);

## Why? 

For the sake of the KISS principle. Most of the time you don't need a streaming compression, you need to compress an existing and already complete data. 

## Options 

compress() takes two arguments: the data (either a String() or a Buffer()) and optional compression level which must be within 1..9. It returns compressed Buffer() or undefined on error.

uncompress() takes a single argument: the data (either a String() or a Buffer()) and returns uncompressed Buffer() or undefined on error.

## License

See LICENSE file. Basically, it's a do-whatever-you-want-for-free license.

## Author

Egor Egorov <me@egorfine.com>

