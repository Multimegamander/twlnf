
#include <stdio.h>
#include <inttypes.h>
#include <nds.h>
#include "utils.h"

static inline int htoi(char a){
	if(a >= '0' && a <= '9'){
		return a - '0';
	}else if(a >= 'a' && a <= 'f'){
		return a - ('a' - 0xa);
	}else if(a >= 'A' && a <= 'F'){
		return a - ('A' - 0xa);
	}else{
		return -1;
	}
}

int hexToBytes(u8 *out, unsigned byte_len, const char *in){
	if (strlen(in) < byte_len << 1){
		iprintf("%s: invalid input length, expecting %u, got %u.\n",
			__FUNCTION__, (unsigned)byte_len << 1, (unsigned)strlen(in));
		return -1;
	}
	for(unsigned i = 0; i < byte_len; ++i){
		int h = htoi(*in++), l = htoi(*in++);
		if(h == -1 || l == -1){
			iprintf("%s: invalid input \"%c%c\"\n",
				__FUNCTION__, *(in - 2), *(in - 1));
			return -2;
		}
		*out++ = (h << 4) + l;
	}
	return 0;
}

static char str_buf[0x10];
const char *toMebi(size_t size) {
	if (size % (1024 * 1024)) {
		sprintf(str_buf, "%.2f", (float)(((double)size) / 1024 / 1024));
	} else {
		siprintf(str_buf, "%u", (unsigned)(size >> 20));
	}
	return str_buf;
}

extern swiSHA1context_t sha1ctx;

//---------------------------------------------------------------------------------
int saveToFile(const char *filename, u8 *buffer, size_t size, int saveSHA1) {
//---------------------------------------------------------------------------------
	FILE *f = fopen(filename, "wb");
	if (NULL==f) return -1;
	size_t written = fwrite(buffer, 1, size, f);
	fclose(f);
	if (written != size) {
		iprintf("Error saving %s\n", filename);
		return -2;
	} else {
		iprintf("saved %s\n", filename);
	}
	if (saveSHA1) {
		sha1ctx.sha_block = 0;
		swiSHA1Init(&sha1ctx);
		swiSHA1Update(&sha1ctx, buffer, size);
		saveSHA1File(filename);
	}
	return 0;
}

//---------------------------------------------------------------------------------
int loadFromFile(void **pbuf, size_t *psize, const char *filename, int verifySHA1, int align) {
//---------------------------------------------------------------------------------
	FILE *f = fopen(filename, "rb");
	if (NULL == f)return -1;
	fseek(f, 0, SEEK_END);
	*psize = ftell(f);
	if (!*psize) {
		*pbuf = 0;
		fclose(f);
		return 1;
	}else{
		if (align) {
			*pbuf = memalign(align, *psize);
		} else {
			*pbuf = malloc(*psize);
		}
		fseek(f, 0, SEEK_SET);
		size_t read = fread(*pbuf, 1, *psize, f);
		if (read != *psize) {
			iprintf("Error loading %s\n", filename);
			free(*pbuf);
			*pbuf = 0;
			fclose(f);
			return -2;
		} else {
			iprintf("loaded %s(%u)\n", filename, read);
		}
		if (verifySHA1) {
			//TODO:
		}
		fclose(f);
		return 0;
	}
}

//---------------------------------------------------------------------------------
int saveSHA1File(const char *filename) {
//---------------------------------------------------------------------------------
	size_t len_fn = strlen(filename);
	char *sha1_fn = (char *)malloc(len_fn + 6);
	siprintf(sha1_fn, "%s.sha1", filename);
	// 20 bytes each use 2 chars, space, asterisk, filename, new line
	size_t len_buf = 2 * 20 + 1 + 1 + len_fn + 1;
	char *sha1_buf = (char *)malloc(len_buf + 1); // extra for \0
	char *p = sha1_buf;
	char *digest = (char *)malloc(20);
	swiSHA1Final(digest, &sha1ctx);
	for (int i = 0; i < 20; ++i) {
		p += siprintf(p, "%02X", digest[i]);
	}
	free(digest);
	siprintf(p, " *%s\n", filename);
	int ret = saveToFile(sha1_fn, (u8*)sha1_buf, len_buf, false);
	free(sha1_fn);
	free(sha1_buf);
	return ret;
}

void printBytes(const void *buf, size_t len) {
	const unsigned char *p = (const unsigned char *)buf;
	for(size_t i = 0; i < len; ++i) {
		iprintf("%02" PRIx8, *p++);
	}
}

