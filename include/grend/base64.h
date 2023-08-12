#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

struct base64_stream_encoder {
	uint32_t meh;
	int bits;
	size_t offset;
	void *data;

	unsigned column_width;
	unsigned column_offset;

	void (*store_chars)(struct base64_stream_encoder*, const char *);
	void (*close_stream)(struct base64_stream_encoder*);
};

struct base64_stream_decoder {
	uint32_t meh;
	int bits;
	size_t offset;
	void *data;

	void (*store_bytes)(struct base64_stream_decoder*, const uint8_t *buf, size_t len);
	void (*close_stream)(struct base64_stream_decoder*);
};


/* TODO: do these need to be visible?
void base64_encode_flush(struct base64_stream_encoder *enc);
void base64_decode_flush(struct base64_stream_decoder *dec);
void base64_encode_byte(struct base64_stream_encoder *enc, uint8_t data);
void base64_encode_chars(struct base64_stream_encoder *enc, const char *data);
void base64_encode_bytes(struct base64_stream_encoder *enc, const uint8_t *data, size_t len);
void base64_decode_char(struct base64_stream_decoder *dec, char c);
void base64_decode_chars(struct base64_stream_decoder *dec, const char *chars);

void init_string_encoder(struct base64_stream_encoder *enc, char *str, unsigned column_width);
void init_file_encoder(struct base64_stream_encoder *enc, FILE *fp, unsigned column_width);
void init_binary_decoder(struct base64_stream_decoder *dec, uint8_t *outbuf);
void init_file_decoder(struct base64_stream_decoder *dec, FILE *fp);
*/

void base64_close_encoder(struct base64_stream_encoder *enc);
void base64_close_decoder(struct base64_stream_decoder *dec);

char *base64_encode_string(const char *str, unsigned column_width);
char *base64_encode_binary(const uint8_t *inbuf, size_t len, unsigned column_width);
void base64_decode_binary(const char *chars, uint8_t **outbuf, size_t *len);
void base64_encode_file(FILE *input, FILE *output, unsigned column_width);
void base64_decode_file(FILE *input, FILE *output);

#ifdef __cplusplus
}
#endif
