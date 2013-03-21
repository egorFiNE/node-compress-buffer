{
	"targets": [
		{
			"target_name": "compress_buffer_bindings",
			"sources": [ "src/compress-buffer.cc" ],
			'link_settings': {
				'libraries': [
				'-lz'
				]
			}
		}
	]
}
