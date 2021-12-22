/*
  haxdiff format diff and patch utility reference implementation.

  (C) 2021 rofl0r, licensed as MIT. see COPYING for full license text.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#endif

static int hval(int c) {
	switch(c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': case 'A': return 10;
		case 'b': case 'B': return 11;
		case 'c': case 'C': return 12;
		case 'd': case 'D': return 13;
		case 'e': case 'E': return 14;
		case 'f': case 'F': return 15;
	}
	return -1;
}

static size_t hex2raw(const char *hex, unsigned char* out) {
	size_t l = strlen(hex);
	int c;
	if(l&1) {
		*(out++) = hval(*hex);
		hex++;
		l++;
	}
	while(*hex) {
		c = hval(*hex);
		hex++;
		c = c << 4;
		c |= hval(*hex);
		hex++;
		*out = c;
		out++;
	}
	return l/2;
}

static void raw2hex(char *dst, const unsigned char *src, size_t len) {
	size_t i, o;
	static const char hextab[] = "0123456789abcdef";
	for (i = 0, o=0; i < len; i++, o+=2) {
		dst[o + 0] = hextab[0xf & (src[i] >> 4)];
		dst[o + 1] = hextab[0xf & src[i]];
	}
	dst[o] = 0;
}

enum difftype {
	DT_NONE = 0,
	DT_DIFF = 1,
	DT_TRUNC = 2,
	DT_EXP = 3,
};
struct diff {
	enum difftype type;
	off_t off;
	off_t n;
};

static void p_diff(FILE *f, off_t n, int chr) {
#define LINE_CHARS 80
	unsigned char buf[LINE_CHARS/2 -2];
	char hexbuf[2+ sizeof(buf)*2 + 2];
	hexbuf[0] = chr;
	hexbuf[1] = ' ';
	hexbuf[sizeof(hexbuf)-1] = 0;
	while(n > 0) {
		off_t cnt = MIN(sizeof(buf), n);
		if(cnt != fread(buf, 1, cnt, f)) abort();
		raw2hex(hexbuf+2, buf, cnt);
		hexbuf[2+cnt*2] = '\n';
		fwrite(hexbuf, 1, cnt*2+2+1, stdout);
		n -= cnt;
	}
}

#include "sblist.h"

static int diff(char *fn_old, char *fn_new) {
	FILE *f[2];
	f[0] = fopen(fn_old, "r");
	if(!f[0]) return 1;
	f[1] = fopen(fn_new, "r");
	if(!f[1]) return 1;
	unsigned char buf[2][16*1024];
	size_t n[2];
	off_t fs[2];
	off_t off = 0;
	size_t i, j;
	sblist *difflist = sblist_new(sizeof(struct diff), 64);
	while(1) {
		for(i=0;i<2;++i) n[i] = fread(buf[i], 1, sizeof(buf[i]), f[i]);
		size_t min = MIN(n[0], n[1]), max = MAX(n[0], n[1]), o;
		if(max == 0) break;
		for(i = o = 0; i < min; o = ++i) {
			if(buf[0][i] != buf[1][i]) {
			fwd:
				while(buf[0][i] != buf[1][i] && i < min) ++i;
				j = i;
				while(j < i+16 && j < min)
					if(buf[0][j] != buf[1][j]) { i = j; goto fwd; }
					else ++j;
				struct diff d = {.type = DT_DIFF, .off = off+o, .n = i-o};
				sblist_add(difflist, &d);
			}
		}
		off += min;
		struct diff d = {.type = DT_DIFF, .off = off, .n = 0};
		if(n[0] < n[1])
			d.type = DT_EXP;
		else if (n[1] < n[0])
			d.type = DT_TRUNC;
		if(d.type != DT_DIFF) {
			sblist_add(difflist, &d);
			break;
		}
	}
	for(i = 0; i < 2; ++i) {
		fseeko(f[i], 0, SEEK_END);
		fs[i] = ftello(f[i]);
	}
	if(sblist_getsize(difflist)) fprintf(stdout, "haxdiff/1.0\n");
	for(i = 0; i < sblist_getsize(difflist); ++i) {
		struct diff *d = sblist_get(difflist, i);
		for(j = 0; j < 2; ++j) fseeko(f[j], d->off, SEEK_SET);
		off_t np, nm;
		switch(d->type) {
		case DT_DIFF: np = d->n; nm = d->n; break;
		case DT_EXP: np = fs[1]-fs[0]; nm = 0; break;
		case DT_TRUNC: np = 0; nm = fs[0]-fs[1]; break;
		}
		fprintf(stdout, "@@ %llx,-%llx,+%llx\n",
				(long long) d->off,
				(long long) nm, (long long) np);
		if(nm) p_diff(f[0], nm, '-');
		if(np) p_diff(f[1], np, '+');
	}
	for(i = 0; i < 2; ++i) fclose(f[i]);
	sblist_free(difflist);
	fflush(stdout);
	return 0;
}
static int malformed(off_t lineno) {
	fprintf(stderr, "malformed patch at line %lld\n", (long long) lineno);
	return 1;
}

static int getdiffdata(char *p, struct diff *d) {
	off_t n1, n2;
	char *q = strchr(p, ',');
	if(!q) return 1;
	d->off = strtoll(p, 0, 16);
	p = q+1;
	if(*(p++) != '-') return 1;
	q = strchr(p, ',');
	n1 = strtoll(p, 0, 16);
	p = q+1;
	if(*(p++) != '+') return 1;
	n2 = strtoll(p, 0, 16);
	d->type = DT_DIFF;
	if(n1 != n2) {
		if(!n1 && n2) d->type = DT_EXP;
		if(n1 && !n2) d->type = DT_TRUNC;
		if(d->type == DT_DIFF) {
			fprintf(stderr, "using chunks of different length not implemented!\n");
			return 1;
		}
		d->n = MAX(n1, n2);
	} else d->n = n1;
	return 0;
}

static int copybytes(FILE *in, FILE *out, off_t start, off_t end) {
	if(fseeko(in, start, SEEK_SET)) return 1;
	if(fseeko(out, start, SEEK_SET)) return 1;
	unsigned char buf[16*1024];
	off_t n = end - start;
	while(n > 0) {
		size_t cnt = MIN(n, sizeof buf);
		size_t r = fread(buf, 1, cnt, in);
		if(r == 0) return 1;
		if(r != fwrite(buf, 1, r, out)) return 1;
		n -= r;
	}
	return 0;
}

static int decode_hex(char *hex, unsigned char* out, size_t *outlen) {
	size_t l = strlen(hex);
	while(l && (hex[l-1] == '\n' || hex[l-1] == '\r')) --l;
	if(!l) return -1;
	if(l % 2) return -1;
	if(l / 2 >= *outlen) return -1;
	hex[l] = 0;
	if(l/2 != hex2raw(hex, out)) return -1;
	*outlen = l/2;
	return 0;
}

static int checkbytes(FILE *in, off_t off, char *hex, off_t *processed) {
	unsigned char hbytes[1024], rbuf[1024];
	size_t l = sizeof(hbytes);
	if(decode_hex(hex, hbytes, &l)) return -1;
	*processed = l;
	if(fseeko(in, off, SEEK_SET)) return 1;
	if(l != fread(rbuf, 1, l, in)) return 1;
	if(memcmp(hbytes, rbuf, l)) return 2;
	return 0;
}

static int writebytes(FILE *out, off_t off, char *hex, off_t *processed) {
	unsigned char hbytes[1024];
	size_t l = sizeof(hbytes);
	if(decode_hex(hex, hbytes, &l)) return -1;
	*processed = l;
	if(fseeko(out, off, SEEK_SET)) return 1;
	if(l != fwrite(hbytes, 1, l, out)) return 1;
	return 0;
}

static int patch(char *fn, char *fn_out, int force) {
	FILE *f[2];
	f[0] = fopen(fn, "r");
	if(!f[0]) return 1;
	f[1] = fopen(fn_out, "w");
	if(!f[1]) return 1;
	off_t off[2] = {0}, r;
	off_t lineno = 0;
	char pbuf[1024], *p;
	int ret;
	struct diff d = {0};
	while(fgets(pbuf, sizeof pbuf, stdin)) {
		++lineno;
		switch(pbuf[0]) {
		case '@': {
			if(pbuf[1] != '@' && pbuf[2] != ' ')
				return malformed(lineno);
			if(getdiffdata(pbuf+3, &d))
				return malformed(lineno);
			if(d.off && copybytes(f[0], f[1], off[1], d.off)) {
			copy_err:
				fprintf(stderr, "failure copying bytes to output file\n");
				return 1;
			}
			off[0] = off[1] = d.off;
			} break;
		case '-':
			if(d.type == DT_NONE) return malformed(lineno);
			if((ret = checkbytes(f[0], off[0], pbuf+2, &r))) {
				if(ret == -1) return malformed(lineno);
				else if(ret == 1) {
					fprintf(stderr, "error occured while checking patch\n");
					return 1;
				} else if(ret == 2) {
					fprintf(stderr, "%s: input file doesn't match data on line %lld%s\n",
						force ? "warning" : "error",
						(long long) lineno,
						force ? "" : ", use force mode if you want to apply it anyway");
					if(!force) return 1;
				}
			}
			off[0] += r;
			break;
		case '+':
			if((ret = writebytes(f[1], off[1], pbuf+2, &r))) {
				if(ret == -1) return malformed(lineno);
				fprintf(stderr, "failure writing to output file\n");
				return 1;
			}
			off[1] += r;
			break;
		}
	}
	if(d.type == DT_DIFF) {
		off_t fs;
		if(fseeko(f[0], 0, SEEK_END)) goto copy_err;
		fs = ftello(f[0]);
		if(copybytes(f[0], f[1], off[0], fs)) goto copy_err;
	}
	for(ret = 0; ret < 2; ++ret) fclose(f[ret]);
	return 0;
}

static int usage(char *a0) {
	printf(
		"usage: %s MODE FILE1 FILE2\n"
		"creates or applies a binary diff.\n"
		"MODE: d - diff, p - patch, P - force patch\n"
		"in mode d, create a diff between files FILE1 and FILE2,\n"
		"and write a patch to stdout.\n"
		"the generated patch describes how to generate FILE2 from FILE1.\n"
		"in patch mode, FILE1 denotes the file to be patched, and FILE2\n"
		"the patched file that will be written (e.g. output file).\n"
		"the patch itself is read from stdin.\n"
		"if mode is p, the replaced bytes must match those described in\n"
		"the patch. if mode is P, the replaced bytes will be ignored.\n"
		"if mode is d, the patch will be written to stdout.\n"
		, a0);
	return 1;
}

enum workmode {
	WM_DIFF = 0,
	WM_PATCH = 1,
	WM_PATCH_FORCE = 2,
};

int main(int argc, char **argv) {
	enum workmode mode;
	if(argc != 4) return usage(argv[0]);
	switch(argv[1][0]) {
	case 'p': mode = WM_PATCH; break;
	case 'P': mode = WM_PATCH_FORCE; break;
	case 'd': mode = WM_DIFF; break;
	default: return usage(argv[0]);
	}
	if(mode == WM_DIFF) return diff(argv[2], argv[3]);
	else return patch(argv[2], argv[3], mode == WM_PATCH_FORCE);
}
