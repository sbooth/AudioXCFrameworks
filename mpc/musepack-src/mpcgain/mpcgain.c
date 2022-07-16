/*
  Copyright (c) 2006-2009, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <math.h>
#include <mpc/mpcdec.h>
#include <mpc/minimax.h>
#include <replaygain/gain_analysis.h>

#include "../libmpcdec/internal.h"
#include "../libmpcdec/huffman.h"
#include "../libmpcdec/mpc_bits_reader.h"

#define MPCGAIN_MAJOR 0
#define MPCGAIN_MINOR 9
#define MPCGAIN_BUILD 3

#define _cat(a,b,c) #a"."#b"."#c
#define cat(a,b,c) _cat(a,b,c)
#define MPCGAIN_VERSION cat(MPCGAIN_MAJOR,MPCGAIN_MINOR,MPCGAIN_BUILD)

#define MAX_HEAD_SIZE 20 // maximum size of the packet header before chapter gain (2 + 9 + 9)

const char    About []        = "mpcgain - Musepack (MPC) ReplayGain calculator v" MPCGAIN_VERSION " (C) 2006-2009 MDT\nBuilt " __DATE__ " " __TIME__ "\n";


static void usage(const char *exename)
{
	printf("Usage: %s <infile.mpc> [<infile2.mpc> <infile3.mpc> ... ]\n", exename);
}

static mpc_inline MPC_SAMPLE_FORMAT _max(MPC_SAMPLE_FORMAT a, MPC_SAMPLE_FORMAT b)
{
	if (a > b)
		return a;
	return b;
}

static mpc_inline MPC_SAMPLE_FORMAT max_abs(MPC_SAMPLE_FORMAT a, MPC_SAMPLE_FORMAT b)
{
	if (b < 0)
		b = -b;
	if (b > a)
		return b;
	return a;
}

static MPC_SAMPLE_FORMAT analyze_get_max(MPC_SAMPLE_FORMAT * sample_buffer, int sample_nb)
{
	Float_t left_samples[MPC_FRAME_LENGTH * sizeof(Float_t)];
	Float_t right_samples[MPC_FRAME_LENGTH * sizeof(Float_t)];
	MPC_SAMPLE_FORMAT max = 0;
	int i;

	for (i = 0; i < sample_nb; i++){
		left_samples[i] = sample_buffer[2 * i] * (1 << 15);
		right_samples[i] = sample_buffer[2 * i + 1] * (1 << 15);
		max = max_abs(max, sample_buffer[2 * i]);
		max = max_abs(max, sample_buffer[2 * i + 1]);
	}
	gain_analyze_samples(left_samples, right_samples, sample_nb, 2);

	return max;
}



static void write_chaps_gain(mpc_demux * demux, const char * file_name,
                             mpc_uint16_t * chap_gain, mpc_uint16_t * chap_peak)
{
	unsigned char buffer[MAX_HEAD_SIZE];
	mpc_bits_reader r;
	mpc_block b;
	mpc_uint64_t size, dummy;
	FILE * file;
	int chap = 0;
	long next_chap_pos = demux->chap_pos >> 3;


	file = fopen( file_name, "r+b");
	if (file == 0) {
		fprintf(stderr, "Can't open file \"%s\" for writing\n", file_name);
		return;
	}

	while (1) {
		fseek(file, next_chap_pos, SEEK_SET);
		fread(buffer, 1, MAX_HEAD_SIZE, file);
		r.buff = buffer;
		r.count = 8;
		size = mpc_bits_get_block(&r, &b);

		if (memcmp(b.key, "CT", 2) != 0)
			break;

		b.size += size;
		size += mpc_bits_get_size(&r, &dummy);

		fseek(file, next_chap_pos + size, SEEK_SET);
		buffer[0] = chap_gain[chap] >> 8;
		buffer[1] = chap_gain[chap] & 0xFF;
		buffer[2] = chap_peak[chap] >> 8;
		buffer[3] = chap_peak[chap] & 0xFF;
		fwrite(buffer, 1, 4, file); // writing chapter gain / peak

		chap++;
		next_chap_pos += b.size;
	}

	fclose(file);
}

int main(int argc, char **argv)
{
	MPC_SAMPLE_FORMAT album_max = 0;
	mpc_uint16_t album_gain;
	mpc_uint16_t album_peak;
	mpc_uint16_t * title_gain;
	mpc_uint16_t * title_peak;
	mpc_uint32_t * header_pos;
	int j;

	printf(About);

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	title_gain = malloc((sizeof(mpc_uint16_t) * 2 + sizeof(mpc_uint32_t)) * (argc - 1));
	title_peak = title_gain + (argc - 1);
	header_pos = (mpc_uint32_t *) (title_peak + (argc - 1));

	for (j = 1; j < argc; j++) {
		MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];
		MPC_SAMPLE_FORMAT title_max = 0, chap_max;
		mpc_uint16_t * chap_gain, * chap_peak;
		mpc_reader reader;
		mpc_demux* demux;
		mpc_streaminfo si;
		mpc_status err;
		int chap_nb, chap = 0;
		mpc_uint64_t cur_sample = 1, next_chap_sample = mpc_int64_max;

		err = mpc_reader_init_stdio(&reader, argv[j]);
		if (err < 0) return !MPC_STATUS_OK;

		demux = mpc_demux_init(&reader);
		if (!demux) return !MPC_STATUS_OK;
		mpc_demux_get_info(demux,  &si);

		chap_nb = mpc_demux_chap_nb(demux);
		mpc_demux_seek_sample(demux, 0);
		if (chap_nb > 0) {
			mpc_chap_info * chap_info = mpc_demux_chap(demux, chap);
			next_chap_sample = chap_info->sample;
			chap_gain = malloc(sizeof(mpc_uint16_t) * 2 * chap_nb);
			chap_peak = chap_gain + chap_nb;
		}

		if (j == 1) gain_init_analysis ( si.sample_freq );

		while (1) {
			mpc_frame_info frame;
			int i = 0;

			frame.buffer = sample_buffer;
			mpc_demux_decode(demux, &frame);
			if (frame.bits == -1) break;

			while (next_chap_sample < cur_sample + frame.samples) {
				int sample_nb = (int)(next_chap_sample - cur_sample);

				chap_max = _max(chap_max, analyze_get_max(sample_buffer + 2 * i, sample_nb));

				if (chap == 0) // first samples are not in a chapter
					gain_get_chapter();
				else {
					chap_gain[chap - 1] = (mpc_uint16_t) (gain_get_chapter() * 256);
					chap_peak[chap - 1] = (mpc_uint16_t) (log10(chap_max * (1 << 15)) * 20 * 256);
				}
				chap++;
				title_max = _max(title_max, chap_max);
				chap_max = 0;
				i += sample_nb;
				cur_sample = next_chap_sample;
				if (chap < chap_nb) {
					mpc_chap_info * chap_info = mpc_demux_chap(demux, chap);
					next_chap_sample = chap_info->sample;
				} else
					next_chap_sample = mpc_int64_max;
			}

			chap_max = _max(chap_max, analyze_get_max(sample_buffer + 2 * i, frame.samples - i));
			cur_sample += frame.samples - i;
		}

		if (chap_nb > 0) {
			chap_gain[chap - 1] = (mpc_uint16_t) (gain_get_chapter() * 256);
			chap_peak[chap - 1] = (mpc_uint16_t) (log10(chap_max * (1 << 15)) * 20 * 256);
			write_chaps_gain(demux, argv[j], chap_gain, chap_peak);
		}

		title_max = _max(title_max, chap_max);
		album_max = _max(album_max, title_max);

		title_gain[j-1] = (mpc_uint16_t) (gain_get_title() * 256);
		title_peak[j-1] = (mpc_uint16_t) (log10(title_max * (1 << 15)) * 20 * 256);
		header_pos[j-1] = si.header_position + 4;

		mpc_demux_exit(demux);
		mpc_reader_exit_stdio(&reader);
		if (chap_nb > 0)
			free(chap_gain);
	}

	album_gain = (mpc_uint16_t) (gain_get_album() * 256);
	album_peak = (mpc_uint16_t) (log10(album_max * (1 << 15)) * 20 * 256);

	for (j = 0; j < argc - 1; j++) {
		unsigned char buffer[64];
		mpc_bits_reader r;
		mpc_block b;
		mpc_uint64_t size;
		FILE * file;

		file = fopen( argv[j + 1], "r+b");
		if (file == 0) {
			fprintf(stderr, "Can't open file \"%s\" for writing\n", argv[j + 1]);
			continue;
		}
		fseek(file, header_pos[j] - 4, SEEK_SET);
		fread(buffer, 1, 16, file);
		if (memcmp(buffer, "MPCK", 4) != 0) {
			fprintf(stderr, "Unsupported file format, not a sv8 file : %s\n", argv[j + 1]);
			fclose(file);
			continue;
		}
		r.buff = buffer + 4;
		r.count = 8;

		for(;;) {
			size = mpc_bits_get_block(&r, &b);
			if (mpc_check_key(b.key) != MPC_STATUS_OK) break;

			if (memcmp(b.key, "RG", 2) == 0) break;
			header_pos[j] += b.size + size;
			fseek(file, header_pos[j], SEEK_SET);
			fread(buffer, 1, 16, file);
			r.buff = buffer;
			r.count = 8;
		}

		if (memcmp(b.key, "RG", 2) != 0 || b.size < 9) { //check for the loop above having aborted without finding the packet we want to update
			fprintf(stderr, "Unsupported file format or corrupted file : %s\n", argv[j + 1]);
			fclose(file);
			continue;
		}
		header_pos[j] += size;

		buffer[size] = 1; // replaygain version
		buffer[size + 1] = title_gain[j] >> 8;
		buffer[size + 2] = title_gain[j] & 0xFF;
		buffer[size + 3] = title_peak[j] >> 8;
		buffer[size + 4] = title_peak[j] & 0xFF;
		buffer[size + 5] = album_gain >> 8;
		buffer[size + 6] = album_gain & 0xFF;
		buffer[size + 7] = album_peak >> 8;
		buffer[size + 8] = album_peak & 0xFF;

		fseek(file, header_pos[j], SEEK_SET);
		fwrite(buffer + size, 1, b.size, file);
		fclose(file);
	}

	free(title_gain);

    return 0;
}
