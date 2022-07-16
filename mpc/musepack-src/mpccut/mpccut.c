/*
  Copyright (c) 2007-2009, The Musepack Development Team
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
#include <stdlib.h>
#include <getopt.h>

#include <mpc/mpcdec.h>
#include "../libmpcenc/libmpcenc.h"
#include "../libmpcdec/internal.h"
#include "../libmpcdec/huffman.h"
#include "../libmpcdec/mpc_bits_reader.h"

#ifdef _MSC_VER
#define atoll _atoi64
#endif

#define MPCCUT_MAJOR 0
#define MPCCUT_MINOR 9
#define MPCCUT_BUILD 0

#define _cat(a,b,c) #a"."#b"."#c
#define cat(a,b,c) _cat(a,b,c)
#define MPCCUT_VERSION cat(MPCCUT_MAJOR,MPCCUT_MINOR,MPCCUT_BUILD)

const char    About []        = "mpccut - Musepack (MPC) stream cutter v" MPCCUT_VERSION " (C) 2007-2009 MDT\nBuilt " __DATE__ " " __TIME__ "\n";

static void copy_data(FILE * in_file, int in_file_pos, FILE * out_file, int data_size)
{
	char * buff[512];

	fseek(in_file, in_file_pos, SEEK_SET);

	while (data_size != 0) {
		int read_size = data_size < 512 ? data_size : 512;
		read_size = fread(buff, 1, read_size, in_file);
		data_size -= read_size;
		fwrite(buff, 1, read_size, out_file);
	}
}

static void usage(const char *exename)
{
	printf("Usage: %s [-s start_sample] [-e end_sample] <infile.mpc> <outfile.mpc>\n", exename);
}

int main(int argc, char **argv)
{
	mpc_reader reader;
	mpc_demux* demux;
	mpc_streaminfo si;
	mpc_encoder_t e;
	unsigned char buffer[16];
	mpc_bits_reader r;
	mpc_block b;
	mpc_uint64_t size;
	mpc_status err;
	mpc_int64_t start_sample = 0, end_sample = 0;
	mpc_uint32_t beg_silence, start_block, block_num, i;
	int c;
	FILE * in_file;

	printf(About);

	while ((c = getopt(argc , argv, "s:e:")) != -1) {
		switch (c) {
			case 's':
				start_sample = atoll(optarg);
				break;
			case 'e':
				end_sample = atoll(optarg);
				break;
		}
	}

	if(argc - optind != 2)
	{
		usage(argv[0]);
		return 0;
	}

	err = mpc_reader_init_stdio(&reader, argv[optind]);
	if(err < 0) return !MPC_STATUS_OK;

	demux = mpc_demux_init(&reader);
	if(!demux) return !MPC_STATUS_OK;
	mpc_demux_get_info(demux,  &si);

	if (si.stream_version < 8) {
		fprintf(stderr, "this file cannot be edited, please convert it first to sv8 using mpc2sv8\n");
		exit(!MPC_STATUS_OK);
	}

	if (end_sample == 0)
		end_sample = si.samples;
	else
		end_sample += si.beg_silence;
	start_sample += si.beg_silence;

	if (start_sample < 0 || end_sample > si.samples || end_sample <= start_sample) {
		fprintf(stderr, "specified samples bounds out of stream bounds\n");
		exit(!MPC_STATUS_OK);
	}

	beg_silence = start_sample % (MPC_FRAME_LENGTH << si.block_pwr);
	start_block = start_sample / (MPC_FRAME_LENGTH << si.block_pwr);
	block_num = (end_sample + (MPC_FRAME_LENGTH << si.block_pwr) - 1) /
			 (MPC_FRAME_LENGTH << si.block_pwr) - start_block;
	end_sample -= start_block * (MPC_FRAME_LENGTH << si.block_pwr);

	mpc_encoder_init(&e, end_sample, si.block_pwr, 1);
	e.outputFile = fopen( argv[optind + 1], "rb" );
	if ( e.outputFile != 0 ) {
		fprintf(stderr, "Error : output file \"%s\" already exists\n", argv[optind + 1]);
		exit(MPC_STATUS_FAIL);
	}
	e.outputFile = fopen( argv[optind + 1], "w+b" );

	e.seek_ref = 0;
	writeMagic(&e);
	writeStreamInfo( &e, si.max_band, si.ms > 0, end_sample, beg_silence,
					  si.sample_freq, si.channels);
	writeBlock(&e, "SH", MPC_TRUE, 0);
	writeGainInfo(&e, 0, 0, 0, 0);
	writeBlock(&e, "RG", MPC_FALSE, 0);


	in_file = fopen(argv[optind], "rb");
	i = si.header_position + 4;
	fseek(in_file, i, SEEK_SET);
	fread(buffer, 1, 16, in_file);
	r.buff = buffer;
	r.count = 8;
	size = mpc_bits_get_block(&r, &b);

	while( memcmp(b.key, "AP", 2) != 0 ) {
		if ((err = mpc_check_key(b.key)) != MPC_STATUS_OK) {
			fprintf(stderr, "Error : invalid input stream\n");
			goto error;
		}
		if (memcmp(b.key, "EI", 2) == 0)
			copy_data(in_file, i, e.outputFile, b.size + size);
		i += b.size + size;
		fseek(in_file, i, SEEK_SET);
		fread(buffer, 1, 16, in_file);
		r.buff = buffer;
		r.count = 8;
		size = mpc_bits_get_block(&r, &b);
	}

	e.seek_ptr = ftell(e.outputFile);
	writeBits (&e, 0, 16);
	writeBits (&e, 0, 24); // jump 40 bits for seek table pointer
	writeBlock(&e, "SO", MPC_FALSE, 0); // reserve space for seek offset


	while( start_block != 0 ){
		if ((err = mpc_check_key(b.key)) != MPC_STATUS_OK) {
			fprintf(stderr, "Error : invalid input stream\n");
			goto error;
		}
		if (memcmp(b.key, "AP", 2) == 0)
			start_block--;
		i += b.size + size;
		fseek(in_file, i, SEEK_SET);
		fread(buffer, 1, 16, in_file);
		r.buff = buffer;
		r.count = 8;
		size = mpc_bits_get_block(&r, &b);
	}

	while( block_num != 0 ){
		if ((err = mpc_check_key(b.key)) != MPC_STATUS_OK) {
			fprintf(stderr, "Error : invalid input stream\n");
			goto error;
		}
		if (memcmp(b.key, "AP", 2) == 0) {
			if ((e.block_cnt & ((1 << e.seek_pwr) - 1)) == 0) {
				e.seek_table[e.seek_pos] = ftell(e.outputFile);
				e.seek_pos++;
			}
			e.block_cnt++;
			copy_data(in_file, i, e.outputFile, b.size + size);
			block_num--;
		}
		i += b.size + size;
		fseek(in_file, i, SEEK_SET);
		fread(buffer, 1, 16, in_file);
		r.buff = buffer;
		r.count = 8;
		size = mpc_bits_get_block(&r, &b);
	}

	writeSeekTable(&e);
	writeBlock(&e, "ST", MPC_FALSE, 0); // write seek table block
	writeBlock(&e, "SE", MPC_FALSE, 0); // write end of stream block

error:
	fclose ( e.outputFile );
	fclose ( in_file );
	mpc_demux_exit(demux);
	mpc_reader_exit_stdio(&reader);
	mpc_encoder_exit(&e);
	if (err != MPC_STATUS_OK)
		remove(argv[optind + 1]);

    return err;
}
