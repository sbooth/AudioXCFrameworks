/*
 * tta.cpp
 *
 * Description: TTA simple console frontend
 * Copyright (c) 1999-2015 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#include "../libtta.h"
#include "../config.h"
#include "tta.h"

using namespace tta;

//////////////////////// Constants and definitions //////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define TTA_VERSION L(VERSION)

#define RIFF_SIGN (0x46464952)
#define WAVE_SIGN (0x45564157)
#define fmt_SIGN  (0x20746D66)
#define data_SIGN (0x61746164)

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE 
#define PCM_BUFFER_LENGTH 5120

typedef struct {
	TTAuint32 chunk_id;
	TTAuint32 chunk_size;
	TTAuint32 format;
	TTAuint32 subchunk_id;
	TTAuint32 subchunk_size;
	TTAuint16 audio_format;
	TTAuint16 num_channels;
	TTAuint32 sample_rate;
	TTAuint32 byte_rate;
	TTAuint16 block_align;
	TTAuint16 bits_per_sample;
} WAVE_hdr;

typedef struct {
	TTAuint32 subchunk_id;
	TTAuint32 subchunk_size;
} WAVE_subchunk_hdr;

typedef struct {
	TTAuint32 f1;
	TTAuint16 f2;
	TTAuint16 f3;
	TTAuint8 f4[8];
} WAVE_subformat;

typedef struct {
	TTAuint16 cb_size;
	TTAuint16 valid_bits;
	TTAuint32 ch_mask;
	WAVE_subformat est;
} WAVE_ext_hdr;

typedef struct {
	TTA_io_callback iocb;
	HANDLE handle;
} TTA_io_callback_wrapper;

static TTAwchar *myname = NULL;

/////////////////////////// TTA common functions ////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void tta_strerror(TTA_CODEC_STATUS e) {
	switch(e) {
		case TTA_OPEN_ERROR: tta_print("\r%s: can't open file\n", myname); break;
		case TTA_FORMAT_ERROR: tta_print("\r%s: not compatible file format\n", myname); break;
		case TTA_FILE_ERROR: tta_print("\r%s: file is corrupted\n", myname); break;
		case TTA_READ_ERROR: tta_print("\r%s: can't read from input file\n", myname); break;
		case TTA_WRITE_ERROR: tta_print("\r%s: can't write to output file\n", myname); break;
		case TTA_MEMORY_ERROR: tta_print("\r%s: insufficient memory available\n", myname); break;
		case TTA_SEEK_ERROR: tta_print("\r%s: file seek error\n", myname); break;
		case TTA_PASSWORD_ERROR: tta_print("\r%s: password protected file\n", myname); break;
		case TTA_NOT_SUPPORTED: tta_print("\r%s: unsupported architecture type\n", myname); break;
		default: tta_print("\rUnknown error\n"); break;
	}
} // tta_strerror

void usage() {
	tta_print("\rUsage:\ttta [-hebd][p password] input_file output_file\n\n");

	tta_print("\t-h\tprint this help\n");
	tta_print("\t-e\tencode file\n");
	tta_print("\t-eb\tblindly mode (ignore data size info)\n");
	tta_print("\t-ep|dp\tpassword protection\n");
	tta_print("\t-d\tdecode file\n\n");

	tta_print("when file is '-', use standard input/output.\n\n");
	tta_print("Project site: http://www.true-audio.com/\n");
} // usage

int read_wav_hdr(HANDLE infile, WAVE_hdr *wave_hdr, TTAuint32 *subchunk_size) {
	WAVE_subchunk_hdr subchunk_hdr;
	TTAuint32 result;
	TTAuint32 def_subchunk_size = 16;

	// Read WAVE header
	if (!tta_read(infile, wave_hdr, sizeof(WAVE_hdr), result) || !result)
		return -1;

	if (wave_hdr->audio_format == WAVE_FORMAT_EXTENSIBLE) {
		WAVE_ext_hdr wave_hdr_ex;

		if (!tta_read(infile, &wave_hdr_ex, sizeof(WAVE_ext_hdr), result) || !result)
			return -1;

		def_subchunk_size += sizeof(WAVE_ext_hdr);
		wave_hdr->audio_format = wave_hdr_ex.est.f1;
	}

	// Skip extra format bytes
	if (wave_hdr->subchunk_size > def_subchunk_size) {
		TTAuint32 extra_len = wave_hdr->subchunk_size - def_subchunk_size;

		if (tta_seek(infile, extra_len) == INVALID_SET_FILE_POINTER)
			return -1;
	}

	// Skip unsupported chunks
	while (1) {
		TTAuint8 chunk_id[5];

		if (!tta_read(infile, &subchunk_hdr, sizeof(WAVE_subchunk_hdr), result) || !result)
			return -1;

		if (subchunk_hdr.subchunk_id == data_SIGN) break;
		if (tta_seek(infile, subchunk_hdr.subchunk_size) == INVALID_SET_FILE_POINTER)
			return -1;

		tta_memcpy(chunk_id, &subchunk_hdr.subchunk_id, 4);
		chunk_id[4] = 0;
	}

	*subchunk_size = subchunk_hdr.subchunk_size;
	return 0;
} // read_wav_hdr

int write_wav_hdr(HANDLE outfile, WAVE_hdr *wave_hdr, TTAuint32 data_size) {
	TTAuint32 result;
	WAVE_subchunk_hdr subchunk_hdr;

	subchunk_hdr.subchunk_id = data_SIGN;
	subchunk_hdr.subchunk_size = data_size;

	// Write WAVE header
	if (!tta_write(outfile, wave_hdr, sizeof(WAVE_hdr), result) ||
		result != sizeof(WAVE_hdr)) return -1;

	// Write Subchunk header
	if (!tta_write(outfile, &subchunk_hdr, sizeof(WAVE_subchunk_hdr), result) ||
		result != sizeof(WAVE_subchunk_hdr)) return -1;

	return 0;
} // write_wav_hdr

#ifdef __GNUC__
TTAuint8 *convert_password(const TTAwchar *src, int *len) {
	TTAuint8 *dst;
	int n;

	dst = (TTAuint8*) tta_malloc(*len + 1);
	if (dst == NULL) return NULL;

	for (n = 0; n != *len; ++n) {
		if ((src[n] & 0xf0) == 0xf0)
			dst[n] = src[n] & 0x0f;
		else if ((src[n] & 0xe0) == 0xe0)
			dst[n] = src[n] & 0x1f;
		else if ((src[n] & 0xc0) == 0xc0)
			dst[n] = src[n] & 0x3f;
		else if ((src[n] & 0x80) == 0x80)
			dst[n] = src[n] & 0x7f;
		else
			dst[n] = src[n];
	}

	dst[n] = '\0';
	*len = n;

	return dst;
} // convert_passwd

#else // MSVC
TTAuint8 *convert_password(const TTAwchar *src, int *len) {
	TTAuint8 *dst;
	int i, n;

	dst = (TTAuint8 *) tta_malloc(*len * 6 + 1);
	if (dst == NULL) return NULL;

	for (i = 0, n = 0; i != *len; ++i) {
		TTAwchar c = src[i];

		if (c < 0x80) {
			dst[n++] = (c & 0xff);
		} else if (c < 0x800) {
			dst[n++] = ((c >> 6) & 0x1f);
			dst[n++] = (c & 0x3f);
		} else if (c < 0x10000) {
			dst[n++] = ((c >> 12) & 0x0f);
			dst[n++] = ((c >> 6) & 0x3f);
			dst[n++] = (c & 0x3f);
		} else if (c < 0x200000) {
			dst[n++] = (((int)c >> 18) & 0x07);
			dst[n++] = ((c >> 12) & 0x3f);
			dst[n++] = ((c >> 6) & 0x3f);
			dst[n++] = (c & 0x3f);
		} else if (c < 0x4000000) {
			dst[n++] = (((int)c >> 24) & 0x03);
			dst[n++] = (((int)c >> 18) & 0x3f);
			dst[n++] = ((c >> 12) & 0x3f);
			dst[n++] = ((c >> 6) & 0x3f);
			dst[n++] = (c & 0x3f);
		} else if (c < 0x80000000) {
			dst[n++] = (((int)c >> 30) & 0x01);
			dst[n++] = (((int)c >> 24) & 0x3f);
			dst[n++] = (((int)c >> 18) & 0x3f);
			dst[n++] = ((c >> 12) & 0x3f);
			dst[n++] = ((c >> 6) & 0x3f);
			dst[n++] = (c & 0x3f);
		}
	}

	dst[n] = '\0';
	*len = n;

	return dst;
} // convert_passwd
#endif // MSVC

/////////////////////////////// Callbacks ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CALLBACK tta_callback(TTAuint32 rate, TTAuint32 fnum, TTAuint32 frames) {
	TTAuint32 pcnt = (TTAuint32)(fnum * 100. / frames);
	if (!(pcnt % 10))
		tta_print("\rProgress: %02d%%", pcnt);
} // tta_callback

TTAint32 CALLBACK read_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io; 
	TTAint32 result;
	if (tta_read(iocb->handle, buffer, size, result))
		return result;
	return 0;
} // read_callback

TTAint32 CALLBACK write_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io; 
	TTAint32 result;
	if (tta_write(iocb->handle, buffer, size, result))
		return result;
	return 0;
} // write_callback

TTAint64 CALLBACK seek_callback(TTA_io_callback *io, TTAint64 offset) {
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io; 
	return tta_seek(iocb->handle, offset);
} // seek_callback

int test_libtta_compatibility() {
	int ret = tta_binary_version();

	if (ret == CPU_ARCH_UNDEFINED) return 0;

#if defined(CPU_X86)
	{
	int ax, bx, cx, dx;
	tta_cpuid(1, ax, bx, cx, dx);
	if (((dx & 0x4000000) && ret == CPU_ARCH_IX86_SSE2) ||
		((cx & 0x0000001) && ret == CPU_ARCH_IX86_SSE3) ||
		((cx & 0x0080000) && ret == CPU_ARCH_IX86_SSE4_1))
		return 0;
	}
#endif

	return ret;
} // test_libtta_compatibility

//////////////////////////////// Compress ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int compress(HANDLE infile, HANDLE outfile, HANDLE tmpfile, void const *passwd, int pwlen, int format) {
	tta_encoder *TTA;
	void *aligned_encoder;
	TTAuint32 data_size;
	WAVE_hdr wave_hdr;
	TTA_io_callback_wrapper io;
	TTAuint8 *buffer = NULL;
	TTAuint32 buf_size, smp_size, len, res;
	TTA_info info;
	int ret = -1;

	io.iocb.write = &write_callback;
	io.iocb.seek = &seek_callback;
	io.iocb.read = NULL;
	io.handle = outfile;

	if (read_wav_hdr(infile, &wave_hdr, &data_size)) {
		tta_strerror(TTA_READ_ERROR);
		return -1;
	}

	// check for supported formats
	if ((wave_hdr.chunk_id != RIFF_SIGN) ||
		(wave_hdr.format != WAVE_SIGN) ||
		(wave_hdr.num_channels == 0) ||
		(wave_hdr.num_channels > MAX_NCH) ||
		(wave_hdr.bits_per_sample == 0) ||
		(wave_hdr.bits_per_sample > MAX_BPS)) {
		tta_strerror(TTA_FORMAT_ERROR);
		return -1;
	}

	aligned_encoder = tta_malloc(sizeof(tta_encoder));
	TTA = new(aligned_encoder) tta_encoder((TTA_io_callback *)&io);

	smp_size = (wave_hdr.num_channels * ((wave_hdr.bits_per_sample + 7) / 8));
	info.nch = wave_hdr.num_channels;
	info.bps = wave_hdr.bits_per_sample;
	info.sps = wave_hdr.sample_rate;
	info.format = format;

	if (info.format == TTA_FORMAT_ENCRYPTED)
		TTA->set_password(passwd, pwlen);

	buf_size = PCM_BUFFER_LENGTH * smp_size;

	// allocate memory for PCM buffer
	buffer = (TTAuint8 *) tta_malloc(buf_size + 4); // +4 for READ_BUFFER macro
	if (buffer == NULL) {
		tta_strerror(TTA_MEMORY_ERROR);
		goto done;
	}

	if (tmpfile != INVALID_HANDLE_VALUE) {
		data_size = 0;
		while (tta_read(infile, buffer, buf_size, len) && len) {
			if (!tta_write(tmpfile, buffer, len, res) || !res) {
				tta_strerror(TTA_WRITE_ERROR);
				goto done;
			}
			data_size += len;
		}
		tta_print("\rBuffered: %d bytes\n", data_size);
		infile = tmpfile;
		tta_reset(infile);
	} else if (data_size >= 0x7fffffff) {
		tta_print("\r%s: incorrect data size info in wav file\n", myname);
		goto done;
	}

	info.samples = data_size / smp_size;

	try {
		TTA->init_set_info(&info, 0);

		while (data_size > 0) {
			buf_size = (buf_size < data_size) ? buf_size : data_size;

			if (!tta_read(infile, buffer, buf_size, len) || !len)
				throw tta_exception(TTA_READ_ERROR);

			if (len) {
				TTA->process_stream(buffer, len, tta_callback);
			} else break;

			data_size -= len;
		}

		TTA->finalize();
		ret = 0;
	} catch (tta_exception ex) {
		tta_strerror(ex.code());
	}

done:
	TTA->~tta_encoder();
	tta_free(aligned_encoder);
	if (buffer) tta_free(buffer);

	return ret;
} // compress

/////////////////////////////// Decompress //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int decompress(HANDLE infile, HANDLE outfile, void const *passwd, int pwlen) {
	tta_decoder *TTA;
	void *aligned_decoder;
	WAVE_hdr wave_hdr;
	TTA_io_callback_wrapper io;
	TTAuint8 *buffer = NULL;
	TTAuint32 buf_size, smp_size, data_size, res;
	TTAint32 len;
	TTA_info info;
	int ret = -1;

	io.iocb.read = &read_callback;
	io.iocb.seek = &seek_callback;
	io.iocb.write = NULL;
	io.handle = infile;

	aligned_decoder = tta_malloc(sizeof(tta_decoder));
	TTA = new(aligned_decoder)tta_decoder((TTA_io_callback *)&io);

	if (passwd && pwlen)
		TTA->set_password(passwd, pwlen);

	try {
		TTA->init_get_info(&info, 0);
	} catch (tta_exception ex) {
		tta_strerror(ex.code());
		goto done;
	}

	smp_size = info.nch * ((info.bps + 7) / 8);
	buf_size = PCM_BUFFER_LENGTH * smp_size;

	// allocate memory for PCM buffer
	buffer = (TTAuint8 *) tta_malloc(buf_size + 4); // +4 for WRITE_BUFFER macro
	if (buffer == NULL) {
		tta_strerror(TTA_MEMORY_ERROR);
		goto done;
	}

	// Fill in WAV header
	data_size = info.samples * smp_size;
	tta_memclear(&wave_hdr, sizeof (wave_hdr));
	wave_hdr.chunk_id = RIFF_SIGN;
	wave_hdr.chunk_size = data_size + 36;
	wave_hdr.format = WAVE_SIGN;
	wave_hdr.subchunk_id = fmt_SIGN;
	wave_hdr.subchunk_size = 16;
	wave_hdr.audio_format = 1;
	wave_hdr.num_channels = (TTAuint16) info.nch;
	wave_hdr.sample_rate = info.sps;
	wave_hdr.bits_per_sample = info.bps;
	wave_hdr.byte_rate = info.sps * smp_size;
	wave_hdr.block_align = (TTAuint16) smp_size;

	// Write WAVE header
	if (write_wav_hdr(outfile, &wave_hdr, data_size)){
		tta_strerror(TTA_WRITE_ERROR);
		goto done;
	}

	try {
		while (1) {
			len = TTA->process_stream(buffer, buf_size, tta_callback);
			if (len) {
				if (!tta_write(outfile, buffer, len * smp_size, res) || !res)
					throw tta_exception(TTA_WRITE_ERROR);
			} else break;
		}
		ret = 0;
	} catch (tta_exception ex) {
		tta_strerror(ex.code());
	}

done:
	TTA->~tta_decoder();
	tta_free(aligned_decoder);
	if (buffer) tta_free(buffer);

	return ret;
} // decompress

//////////////////////////// The main function //////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int tta_main(int argc, TTAwchar **argv) {
	TTAwchar *fname_in, *fname_out;
	TTAwchar fname_tmp[10] = {'T','T','A','X','X','X','X','X','X','\0'};
	HANDLE infile = INVALID_HANDLE_VALUE;
	HANDLE outfile = INVALID_HANDLE_VALUE;
	HANDLE tmpfile = INVALID_HANDLE_VALUE;
	TTAuint8 *pwstr = NULL;
	TTAuint32 start, end;
	int act = 0;
	int pwlen = 0;
	int blind = 0;
	int format = TTA_FORMAT_SIMPLE;
	int ret = -1;
	char c;

	setlocale(LC_ALL, LOCALE);
	myname = argv[0];

	tta_print("\r\nTTA1 lossless audio encoder/decoder, version %s\n\n", TTA_VERSION);

	if (test_libtta_compatibility()) {
		tta_print("\r%s: unsupported architecture type\n", myname);
		return ret;
	}

	if (argc > 3) {
		fname_in = argv[argc - 2];
		fname_out = argv[argc - 1];
		argc -= 2;
	} else {
		usage();
		goto done;
	}

	while ((c = getopt(argc, argv, "hedbp:")) != -1)
	switch (c) {
		case 'h': // print help
			usage();
			goto done;
		case 'e': // encode file
			if (act == 2) {
				tta_print("\r%s: can't combine two options '-e & -d'\n", myname);
				goto done;
			}
			act = 1;
			break;
		case 'd': // decode file
			if (act == 1) {
				tta_print("\r%s: can't combine two options '-d & -e'\n", myname);
				goto done;
			}
			act = 2;
			break;
		case 'p': // password protection
			format = TTA_FORMAT_ENCRYPTED;
			pwlen = tta_strlen(optarg);
			pwstr = convert_password(optarg, &pwlen);
			if (pwstr == NULL) {
				tta_strerror(TTA_MEMORY_ERROR);
				goto done;
			}
			break;
		case 'b': // blindly mode
			if (act == 2) {
				tta_print("\r%s: option '-b' is not supported by decoder\n", myname);
				goto done;
			}
			blind = 1;
			break;
		case '?':
		default:
			goto done;
	}

	if (!act) {
		tta_print("\r%s: commandline options incomplete\n", myname);
		goto done;
	}

	if (*fname_in == '-' && *(fname_in + 1) == '\0')
		infile = STDIN_FILENO;
	else infile = tta_open_read(fname_in);
	if (infile == INVALID_HANDLE_VALUE) {
		tta_strerror(TTA_OPEN_ERROR);
		goto done;
	}

	if (*fname_out == '-' && *(fname_out + 1) == '\0')
		outfile = STDOUT_FILENO;
	else outfile = tta_open_write(fname_out);
	if (outfile == INVALID_HANDLE_VALUE) {
		tta_strerror(TTA_OPEN_ERROR);
		goto done;
	}

	start = GetTickCount();

	// process
	switch (act) {
	case 1:
		tta_print("\rEncoding: \"%s\" to \"%s\"\n", fname_in, fname_out);
		if (blind) {
			tmpfile = mkstemp(fname_tmp);
			if (tmpfile == INVALID_HANDLE_VALUE) {
				tta_print("\r%s: can't create tempfile: \"%s\"\n", fname_tmp, myname);
				goto done;
			} else tta_print("\rTempfile: \"%s\"\n", fname_tmp);
		}
		ret = compress(infile, outfile, tmpfile, pwstr, pwlen, format);
		if (blind && tmpfile != INVALID_HANDLE_VALUE) {
			tta_close(tmpfile);
			tta_unlink(fname_tmp);
		}
		break;
	case 2:
		tta_print("\rDecoding: \"%s\" to \"%s\"\n", fname_in, fname_out);
		ret = decompress(infile, outfile, pwstr, pwlen);
		break;
	}

	if (infile != STDIN_FILENO) tta_close(infile);
	if (outfile != STDOUT_FILENO) {
		tta_close(outfile);
		if (ret) tta_unlink(fname_out);
	}

	if (!ret) {
		end = GetTickCount();
		tta_print("\rTime: %.3f sec.\n",
			(end - start) / 1000.);
	}

done:
	if (pwstr) tta_free(pwstr);
	return ret;
}

/* eof */

