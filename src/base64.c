#include <grend/base64.h>

static const char *base64
	= "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	  "abcdefghijklmnopqrstuvwxyz"
	  "0123456789+/"
	  ;

static const unsigned base64_decode[] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
     0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
     0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
};

static inline size_t b64_getIndex(uint32_t bitbuf, unsigned i) {
	size_t off = 18 - 6*i;
	return (bitbuf & (63 << off)) >> off;
}

static inline size_t b64_get_decode_index(uint32_t bitbuf, unsigned i) {
	size_t off = 16 - 8*i;
	return (bitbuf & (0xff << off)) >> off;
}

void base64_encode_flush(struct base64_stream_encoder *enc) {
	char buf[5];

	for (size_t i = 0; i < 4; i++) {
		buf[i] = (i*6 < enc->bits)? base64[b64_getIndex(enc->meh, i)] : '=';
	}

	buf[4] = '\0';
	enc->store_chars(enc, buf);
	enc->offset += 4;
	enc->column_offset += 4;
	enc->meh = 0;
	enc->bits = 0;

	if (enc->column_width && enc->column_offset >= enc->column_width) {
		const char *nl = "\n";
		enc->store_chars(enc, nl);
		enc->column_offset = 0;
	}
}

void base64_decode_flush(struct base64_stream_decoder *dec) {
	uint8_t buf[3];

	for (size_t i = 0; i < 3; i++) {
		if (i*8 < dec->bits) {
			buf[i] = b64_get_decode_index(dec->meh, i);
		}
	}

	dec->store_bytes(dec, buf, dec->bits/8);
	dec->offset += 3;
	dec->meh = 0;
	dec->bits = 0;
}

void base64_encode_byte(struct base64_stream_encoder *enc, uint8_t data) {
	enc->meh |= (uint32_t)data << 8*(2 - (enc->bits >> 3));
	enc->bits += 8;

	if (enc->bits >= 24) {
		base64_encode_flush(enc);
	}
}

void base64_encode_chars(struct base64_stream_encoder *enc, const char *data) {
	for (size_t i = 0; data[i]; i++) {
		base64_encode_byte(enc, data[i]);
	}
}

void base64_encode_bytes(struct base64_stream_encoder *enc, const uint8_t *data, size_t len) {
	for (size_t i = 0; i < len; i++) {
		base64_encode_byte(enc, data[i]);
	}
}

void base64_decode_char(struct base64_stream_decoder *dec, char c) {
	if (!((c >= 'A' && c <= 'Z')
	   || (c >= 'a' && c <= 'z')
	   || (c >= '0' && c <= '9')
	   || c == '+'
	   || c == '/'))
	{
		// if the character isn't in the base64 character set, ignore it
		return;
	}

	unsigned idx = (unsigned)c & 0x7f;

	dec->meh |= (uint32_t)base64_decode[idx] << 6*(3 - (dec->bits / 6));
	dec->bits += 6;

	if (dec->bits >= 24) {
		base64_decode_flush(dec);
	}
}

void base64_decode_chars(struct base64_stream_decoder *dec, const char *chars) {
	for (size_t i = 0; chars[i]; i++) {
		base64_decode_char(dec, chars[i]);
	}
}

void base64_close_encoder(struct base64_stream_encoder *enc) {
	if (enc->bits > 0) {
		base64_encode_flush(enc);
	}

	enc->close_stream(enc);
}

void base64_close_decoder(struct base64_stream_decoder *dec) {
	if (dec->bits > 0) {
		base64_decode_flush(dec);
	}

	dec->close_stream(dec);
}

void store_chars(struct base64_stream_encoder *enc, const char *chars) {
	char *buf = (char *)enc->data;

	for (size_t i = 0; chars[i]; i++) {
		buf[enc->offset + i] = chars[i];
	}
}

void close_string(struct base64_stream_encoder *enc) {
	char *buf = (char *)enc->data;
	buf[enc->offset] = '\0';
}

void close_decoder_binary(struct base64_stream_decoder *dec) {
	// nop
}

void store_file_chars(struct base64_stream_encoder *enc, const char *chars) {
	FILE *fp = (FILE*)enc->data;

	for (size_t i = 0; chars[i]; i++) {
		fputc(chars[i], fp);
	}
}

void store_file_bytes(struct base64_stream_decoder *dec, const uint8_t *buf, size_t len) {
	FILE *fp = (FILE*)dec->data;
	fwrite(buf, len, 1, fp);
}

void store_bytes_decoder(struct base64_stream_decoder *dec, const uint8_t *buf, size_t len) {
	uint8_t *outbuf = dec->data;

	for (size_t i = 0; i < len; i++) {
		outbuf[dec->offset + i] = buf[i];
	}
}

void close_encoder_file(struct base64_stream_encoder *enc) {
	FILE *fp = (FILE*)enc->data;
	fflush(fp);
}

void close_decoder_file(struct base64_stream_decoder *dec) {
	FILE *fp = (FILE*)dec->data;
	fflush(fp);
}

void init_string_encoder(struct base64_stream_encoder *enc, char *str, unsigned column_width) {
	enc->meh    = 0;
	enc->bits   = 0;
	enc->offset = 0;
	enc->data   = str;

	enc->column_width  = column_width;
	enc->column_offset = 0;;

	enc->store_chars  = store_chars;
	enc->close_stream = close_string;
}

void init_file_encoder(struct base64_stream_encoder *enc, FILE *fp, unsigned column_width) {
	enc->meh    = 0;
	enc->bits   = 0;
	enc->offset = 0;
	enc->data   = fp;

	enc->column_width  = column_width;
	enc->column_offset = 0;

	enc->store_chars  = store_file_chars;
	enc->close_stream = close_encoder_file;
}

void init_binary_decoder(struct base64_stream_decoder *dec, uint8_t *outbuf) {
	dec->meh    = 0;
	dec->bits   = 0;
	dec->offset = 0;
	dec->data   = outbuf;

	dec->store_bytes  = store_bytes_decoder;
	dec->close_stream = close_decoder_binary;
}

void init_file_decoder(struct base64_stream_decoder *dec, FILE *fp) {
	dec->meh    = 0;
	dec->bits   = 0;
	dec->offset = 0;
	dec->data   = fp;

	dec->store_bytes  = store_file_bytes;
	dec->close_stream = close_decoder_file;
}

char *base64_encode_string(const char *str, unsigned column_width) {
	size_t len = strlen(str);
	float temp = ceil((len + len/3.0) / 4.0) * 4.0;
	size_t outSize = ceil(1 + temp + (column_width? temp / (float)column_width : 0));
	char *buf = calloc(1, sizeof(char[outSize]));

	struct base64_stream_encoder enc;
	init_string_encoder(&enc, buf, column_width);

	base64_encode_chars(&enc, str);
	base64_close_encoder(&enc);

	return buf;
}

char *base64_encode_binary(const uint8_t *inbuf, size_t len, unsigned column_width) {
	float temp = ceil((len + len/3.0) / 4.0) * 4.0;
	size_t outSize = ceil(1 + temp + (column_width? temp / (float)column_width : 0));
	char *outbuf = calloc(1, sizeof(char[outSize]));

	struct base64_stream_encoder enc;
	init_string_encoder(&enc, outbuf, column_width);

	base64_encode_bytes(&enc, inbuf, len);
	base64_close_encoder(&enc);

	return outbuf;
}

static inline
size_t count_base64_chars(const char *str) {
	size_t ret = 0;

	for (size_t i = 0; str[i]; i++) {
		char c = str[i];

		ret += (c >= 'A' && c <= 'Z')
		    || (c >= 'a' && c <= 'z')
		    || (c >= '0' && c <= '9')
		    ||  c == '+'
		    ||  c == '/'
		    ;
	}

	return ret;
}

void base64_decode_binary(const char *chars, uint8_t **outbuf, size_t *len) {
	size_t numchars = count_base64_chars(chars);
	*len    = ceil(numchars / 4.0) * 3;
	*outbuf = calloc(1, *len);

	struct base64_stream_decoder dec;
	init_binary_decoder(&dec, *outbuf);

	base64_decode_chars(&dec, chars);
	base64_close_decoder(&dec);
}

void base64_encode_file(FILE *input, FILE *output, unsigned column_width) {
	struct base64_stream_encoder enc;
	init_file_encoder(&enc, output, column_width);

	int c = fgetc(input);

	while (c != EOF) {
		base64_encode_byte(&enc, c);
		c = fgetc(input);
	}

	base64_close_encoder(&enc);
}

void base64_decode_file(FILE *input, FILE *output) {
	struct base64_stream_decoder dec;
	init_file_decoder(&dec, output);

	int c = fgetc(input);

	while (c != EOF) {
		base64_decode_char(&dec, c);
		c = fgetc(input);
	}

	base64_close_decoder(&dec);
}
