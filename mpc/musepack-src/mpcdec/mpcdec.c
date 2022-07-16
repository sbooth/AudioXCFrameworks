/*
  Copyright (c) 2005-2009, The Musepack Development Team
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
#include <assert.h>
#include <time.h>
#include <mpc/mpcdec.h>
#include "../libmpcdec/decoder.h"
#include "../libmpcdec/internal.h"
#include <libwaveformat.h>
#include <getopt.h>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#ifdef WIN32
# include <fcntl.h>
# include <io.h>
# define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
# define SET_BINARY_MODE(file)
#endif

#define MPCDEC_MAJOR 1
#define MPCDEC_MINOR 0
#define MPCDEC_BUILD 0

#define _cat(a,b,c) #a"."#b"."#c
#define cat(a,b,c) _cat(a,b,c)
#define MPCDEC_VERSION cat(MPCDEC_MAJOR,MPCDEC_MINOR,MPCDEC_BUILD)

const char    About []        = "mpcdec - Musepack (MPC) decoder v" MPCDEC_VERSION " (C) 2006-2009 MDT\nBuilt " __DATE__ " " __TIME__ "\n";


t_wav_uint32 mpc_wav_output_write(void* p_user_data, void const* p_buffer, t_wav_uint32 p_bytes)
{
    FILE* p_handle = (FILE*) p_user_data;
    return (t_wav_uint32) fwrite(p_buffer, 1, p_bytes, p_handle);
}

t_wav_uint32 mpc_wav_output_seek(void* p_user_data, t_wav_uint32 p_position)
{
    FILE* p_handle = (FILE*) p_user_data;
    return (t_wav_uint32) !fseek(p_handle, p_position, SEEK_SET);
}

static void print_info(mpc_streaminfo * info, char * filename)
{
	int time = (int) mpc_streaminfo_get_length(info);
	int minutes = time / 60;
	int seconds = time % 60;

	fprintf(stderr, "file: %s\n", filename);
	fprintf(stderr, "stream version %d\n", info->stream_version);
	fprintf(stderr, "encoder: %s\n", info->encoder);
	fprintf(stderr, "profile: %s (q=%0.2f)\n", info->profile_name, info->profile - 5);
	fprintf(stderr, "PNS: %s\n", info->pns == 0xFF ? "unknow" : info->pns ? "on" : "off");
	fprintf(stderr, "mid/side stereo: %s\n", info->ms ? "on" : "off");
	fprintf(stderr, "gapless: %s\n", info->is_true_gapless ? "on" : "off");
	fprintf(stderr, "average bitrate: %6.1f kbps\n", info->average_bitrate * 1.e-3);
	fprintf(stderr, "samplerate: %d Hz\n", info->sample_freq);
	fprintf(stderr, "channels: %d\n", info->channels);
	fprintf(stderr, "length: %d:%.2d (%u samples)\n", minutes, seconds, (mpc_uint32_t)mpc_streaminfo_get_length_samples(info));
	fprintf(stderr, "file size: %d Bytes\n", info->total_file_length);
	fprintf(stderr, "track peak: %2.2f dB\n", info->peak_title / 256.f);
	fprintf(stderr, "track gain: %2.2f dB / %2.2f dB\n", info->gain_title / 256.f, info->gain_title == 0 ? 0 : 64.82f - info->gain_title / 256.f);
	fprintf(stderr, "album peak: %2.2f dB\n", info->peak_album / 256.f);
	fprintf(stderr, "album gain: %2.2f dB / %2.2f dB\n", info->gain_album / 256.f, info->gain_album == 0 ? 0 : 64.82f - info->gain_album / 256.f);
	fprintf(stderr, "\n");

}

static void
usage(const char *exename)
{
    fprintf(stderr, "Usage: %s [-i] [-h] <infile.mpc> [<outfile.wav>]\n"
			"-i : print file information on stdout\n"
			"-c : check the file for stream errors\n"
			"     (doesn't fully decode, outfile will be ignored)\n"
			"-h : print this help\n"
            "you can use stdin and stdout as resp. <infile.mpc> and\n"
            "<outfile.wav> replacing the file name by \"-\"\n", exename);
}

int
main(int argc, char **argv)
{
    mpc_reader reader;
	mpc_demux* demux;
	mpc_streaminfo si;
	mpc_status err;
	mpc_bool_t info = MPC_FALSE, is_wav_output = MPC_FALSE, check = MPC_FALSE;
    MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];
    clock_t begin, end, sum; int total_samples; t_wav_output_file wav_output;
	int c;

    fprintf(stderr, About);

	while ((c = getopt(argc , argv, "ihc")) != -1) {
		switch (c) {
			case 'i':
				info = MPC_TRUE;
				break;
			case 'c':
				check = MPC_TRUE;
				break;
			case 'h':
				usage(argv[0]);
				return 0;
		}
	}

	if(2 < argc - optind || argc - optind < 1)
    {
        usage(argv[0]);
        return 0;
    }

	if (strcmp(argv[optind], "-") == 0) {
		SET_BINARY_MODE(stdin);
		err = mpc_reader_init_stdio_stream(& reader, stdin);
	} else
		err = mpc_reader_init_stdio(&reader, argv[optind]);
    if(err < 0) return !MPC_STATUS_OK;

    demux = mpc_demux_init(&reader);
    if(!demux) return !MPC_STATUS_OK;
    mpc_demux_get_info(demux,  &si);

	if (info == MPC_TRUE) {
		print_info(&si, argv[optind]);
		mpc_demux_exit(demux);
		mpc_reader_exit_stdio(&reader);
		return 0;
	}

	if (!check)
		is_wav_output = argc - optind > 1;
    if(is_wav_output)
    {
        t_wav_output_file_callback wavo_fc;
        memset(&wav_output, 0, sizeof wav_output);
        wavo_fc.m_seek      = mpc_wav_output_seek;
        wavo_fc.m_write     = mpc_wav_output_write;
		if (strcmp(argv[optind + 1], "-") == 0) {
			SET_BINARY_MODE(stdout);
		    wavo_fc.m_user_data = stdout;
		} else
			wavo_fc.m_user_data = fopen(argv[optind + 1], "wb");
        if(!wavo_fc.m_user_data) return !MPC_STATUS_OK;
        err = waveformat_output_open(&wav_output, wavo_fc, si.channels, 16, 0, si.sample_freq, (t_wav_uint32) si.samples * si.channels);
        if(!err) return !MPC_STATUS_OK;
    }

    sum = total_samples = 0;
    while(MPC_TRUE)
    {
        mpc_frame_info frame;

        frame.buffer = sample_buffer;
	    if (check)
		    demux->d->samples_to_skip = MPC_FRAME_LENGTH + MPC_DECODER_SYNTH_DELAY;
        begin        = clock();
        err = mpc_demux_decode(demux, &frame);
        end          = clock();
        if(frame.bits == -1) break;

        total_samples += frame.samples;
        sum           += end - begin;

		if(is_wav_output) {
#ifdef MPC_FIXED_POINT
			mpc_int16_t tmp_buff[MPC_DECODER_BUFFER_LENGTH];
			int i;
			for( i = 0; i < MPC_DECODER_BUFFER_LENGTH; i++) {
				int tmp = sample_buffer[i] >> MPC_FIXED_POINT_FRACTPART;
				if (tmp > ((1 << 15) - 1)) tmp = ((1 << 15) - 1);
				if (tmp < -(1 << 15)) tmp = -(1 << 15);
				tmp_buff[i] = tmp;
			}
			if(waveformat_output_process_int16(&wav_output, tmp_buff, frame.samples * si.channels) < 0)
#else
			if(waveformat_output_process_float32(&wav_output, sample_buffer, frame.samples * si.channels) < 0)
#endif
                break;
		}
    }

	if (err != MPC_STATUS_OK)
		fprintf(stderr, "An error occured while decoding\n");
	else if (check)
		fprintf(stderr, "No error found\n");

	if (!check) {
		fprintf(stderr, "%u samples ", total_samples);
		if (sum <= 0) sum = 1;
		total_samples = (mpc_uint32_t) ((mpc_uint64_t) total_samples * CLOCKS_PER_SEC * 100 / ((mpc_uint64_t)si.sample_freq * sum));
		fprintf(stderr, "decoded in %u ms (%u.%02ux)\n",
			(unsigned int) (sum * 1000 / CLOCKS_PER_SEC),
			total_samples / 100,
			total_samples % 100
			);
	}

    mpc_demux_exit(demux);
    mpc_reader_exit_stdio(&reader);
    if(is_wav_output)
    {
        waveformat_output_close(&wav_output);
        fclose(wav_output.m_callback.m_user_data);
    }

#ifdef _MSC_VER
    assert(_CrtCheckMemory());
    _CrtDumpMemoryLeaks();
#endif
    return err;
}
