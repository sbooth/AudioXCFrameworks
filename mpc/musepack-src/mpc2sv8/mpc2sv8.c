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
#include <mpc/mpcdec.h>
#include <mpc/minimax.h>
#include <getopt.h>

#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

#include "../libmpcdec/decoder.h"
#include "../libmpcdec/internal.h"
#include "../libmpcenc/libmpcenc.h"

#define TMP_BUF_SIZE 128

#define MPC2SV8_MAJOR 1
#define MPC2SV8_MINOR 0
#define MPC2SV8_BUILD 0

#define _cat(a,b,c) #a"."#b"."#c
#define cat(a,b,c) _cat(a,b,c)
#define MPC2SV8_VERSION cat(MPC2SV8_MAJOR,MPC2SV8_MINOR,MPC2SV8_BUILD)

const char    About []        = "mpc2sv8 - Musepack (MPC) sv7 to sv8 converter v" MPC2SV8_VERSION " (C) 2007-2009 MDT\nBuilt " __DATE__ " " __TIME__ "\n";

static void datacpy(mpc_decoder * d, mpc_encoder_t * e)
{
	static const int  offset[] = { 0, 1, 2, 3, 4, 7, 15, 31, 63, 127, 255, 511,
		1023, 2047, 4095, 8191, 16383, 32767 };
	int i, j;

	memcpy(e->SCF_Index_L, d->SCF_Index_L, sizeof(e->SCF_Index_L));
	memcpy(e->SCF_Index_R, d->SCF_Index_R, sizeof(e->SCF_Index_R));
	memcpy(e->Res_L, d->Res_L, sizeof(e->Res_L));
	memcpy(e->Res_R, d->Res_R, sizeof(e->Res_R));
	memcpy(e->MS_Flag, d->MS_Flag, sizeof(e->MS_Flag));

	for( i = 0; i <= d->max_band; i++){
		mpc_int16_t * q_d = d->Q[i].L, * q_e = e->Q[i].L, Res = d->Res_L[i];

		if (Res > 0)
			for( j = 0; j < 36; j++)
				q_e[j] = q_d[j] + offset[Res];

		q_d = d->Q[i].R, q_e = e->Q[i].R, Res = d->Res_R[i];

		if (Res > 0)
			for( j = 0; j < 36; j++)
				q_e[j] = q_d[j] + offset[Res];
	}
}

static void
usage(const char *exename)
{
	printf("Usage:\n"
	       "%s <infile.mpc> <outfile.mpc>\n"
	       "or\n"
	       "%s <infile_1.mpc> [ <infile_2.mpc> ... <infile_n.mpc> ] <outdir>\n", exename, exename);
}

int convert(char * sv7file, char * sv8file)
{
	mpc_reader reader;
	mpc_demux* demux;
	mpc_streaminfo si;
	mpc_status err;
	mpc_encoder_t e;
	mpc_uint_t si_size;
	mpc_size_t stream_size;
	size_t r_size;
	FILE * in_file;
	char buf[TMP_BUF_SIZE];

	err = mpc_reader_init_stdio(&reader, sv7file);
	if(err < 0) return err;

	demux = mpc_demux_init(&reader);
	if(!demux) {
		err = !MPC_STATUS_OK;
		goto READER_ERR;
	}
	mpc_demux_get_info(demux,  &si);

	if (si.stream_version >= 8) {
		fprintf(stderr, "Error : the file \"%s\" is already a sv8 file\n", sv7file);
		err = !MPC_STATUS_OK;
		goto DEMUX_ERR;
	}

	mpc_encoder_init(&e, si.samples, 6, 1);
	e.outputFile = fopen( sv8file, "w+b" );
	e.MS_Channelmode = si.ms;

	// copy begining of file
	in_file = fopen(sv7file, "rb");
	if(in_file == 0) {
		err = !MPC_STATUS_OK;
		goto OUT_FILE_ERR;
	}
	r_size = si.header_position;
	while(r_size) {
		size_t tmp_size = fread(buf, 1, mini(TMP_BUF_SIZE, r_size), in_file);
		if (fwrite(buf, 1, tmp_size, e.outputFile) != tmp_size) {
			fprintf(stderr, "Error writing to target file : \"%s\"\n", sv8file);
			err = MPC_STATUS_FAIL;
			goto IN_FILE_ERR;
		}
		r_size -= tmp_size;
	}

	// stream conversion
	e.seek_ref = ftell(e.outputFile);
	writeMagic(&e);
	writeStreamInfo( &e, si.max_band, si.ms > 0, si.samples, 0, si.sample_freq,
	                 si.channels);
	si_size = writeBlock(&e, "SH", MPC_TRUE, 0);
	writeGainInfo(&e, si.gain_title, si.peak_title, si.gain_album, si.peak_album);
	si_size = writeBlock(&e, "RG", MPC_FALSE, 0);
	writeEncoderInfo(&e, si.profile, si.pns, si.encoder_version / 100,
	                 si.encoder_version % 100, 0);
	writeBlock(&e, "EI", MPC_FALSE, 0);
	e.seek_ptr = ftell(e.outputFile);
	writeBits (&e, 0, 16);
	writeBits (&e, 0, 24); // jump 40 bits for seek table pointer
	writeBlock(&e, "SO", MPC_FALSE, 0); // reserve space for seek offset
	while(MPC_TRUE)
	{
		mpc_frame_info frame;

		demux->d->samples_to_skip = MPC_FRAME_LENGTH + MPC_DECODER_SYNTH_DELAY;
		err = mpc_demux_decode(demux, &frame);

		if(frame.bits == -1) break;

		datacpy(demux->d, &e);
		writeBitstream_SV8 ( &e, si.max_band); // write SV8-Bitstream
	}

	if (err != MPC_STATUS_OK)
		fprintf(stderr, "An error occured while decoding, this file may be corrupted\n");

    // write the last incomplete block
	if (e.framesInBlock != 0) {
		if ((e.block_cnt & ((1 << e.seek_pwr) - 1)) == 0) {
			e.seek_table[e.seek_pos] = ftell(e.outputFile);
			e.seek_pos++;
		}
		e.block_cnt++;
		writeBlock(&e, "AP", MPC_FALSE, 0);
	}
	writeSeekTable(&e);
	writeBlock(&e, "ST", MPC_FALSE, 0); // write seek table block
	writeBlock(&e, "SE", MPC_FALSE, 0); // write end of stream block
	if (demux->d->samples != si.samples) {
		fseek(e.outputFile, e.seek_ref + 4, SEEK_SET);
		writeStreamInfo( &e, si.max_band, si.ms > 0, demux->d->samples, 0,
		                 si.sample_freq, si.channels);
		writeBlock(&e, "SH", MPC_TRUE, si_size);
		fseek(e.outputFile, 0, SEEK_END);
	}

	// copy end of file

	stream_size = (((mpc_demux_pos(demux) + 7 - 20) >> 3) - si.header_position + 3) & ~3;
	fseek(in_file, si.header_position + stream_size, SEEK_SET);
	while((r_size = fread(buf, 1, TMP_BUF_SIZE, in_file))) {
		if (fwrite(buf, 1, r_size, e.outputFile) != r_size) {
			fprintf(stderr, "Error writing to target file");
			break;
		}
	}

IN_FILE_ERR:
	fclose ( in_file );
OUT_FILE_ERR:
	fclose ( e.outputFile );
	mpc_encoder_exit(&e);
DEMUX_ERR:
	mpc_demux_exit(demux);
READER_ERR:
	mpc_reader_exit_stdio(&reader);

	return err;
}

mpc_bool_t is_dir(char * dir_path)
{
	DIR * out_dir = opendir(dir_path);
	if (out_dir != 0) {
		closedir(out_dir);
		return MPC_TRUE;
	}
	return MPC_FALSE;
}

int
main(int argc, char **argv)
{
	int c, i;
	mpc_bool_t overwrite = MPC_FALSE, use_dir = MPC_FALSE;
	int ret = MPC_STATUS_OK;
	printf(About);

	while ((c = getopt(argc , argv, "oh")) != -1) {
		switch (c) {
		case 'o':
			overwrite = MPC_TRUE;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		}
	}

	use_dir = is_dir(argv[argc - 1]);

	if((argc - optind) < 2 || (use_dir == MPC_FALSE && (argc - optind) > 2)) {
		usage(argv[0]);
		return 0;
	}

	for (i = optind; i < argc - 1; i++) {
		char * in_file = argv[i];
		char * out_file = argv[argc - 1];
		if (use_dir == MPC_TRUE) {
			char * file_name = basename(in_file);
			out_file = malloc(strlen(file_name) + strlen(argv[argc - 1]) + 2);
			sprintf(out_file, "%s/%s", argv[argc - 1], file_name);
		}
		if (overwrite == MPC_FALSE) {
			FILE * test_file = fopen( out_file, "rb" );
			if ( test_file != 0 ) {
				fprintf(stderr, "Error : output file \"%s\" already exists\n", out_file);
				fclose(test_file);
				continue;
			}
		}
		// FIXME : test if in and out files are the same
		ret = convert(in_file, out_file);
		if (use_dir == MPC_TRUE)
			free(out_file);
	}

	return ret;
}
