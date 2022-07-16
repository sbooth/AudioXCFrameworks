/*
*  C Implementation: wavcmp
*
* Description:
*
*
* Author: Nicolas Botti <rududu@laposte.net>, (C) 2007
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include <stdio.h>
#include <string.h>

#include <libwaveformat.h>

t_wav_uint32 wav_write(void* p_user_data, void const* p_buffer, t_wav_uint32 p_bytes)
{
	return (t_wav_uint32) fwrite(p_buffer, 1, p_bytes, (FILE*) p_user_data);
}

t_wav_uint32 wav_seek(void* p_user_data, t_wav_uint32 p_position)
{
	return (t_wav_uint32) !fseek((FILE*) p_user_data, p_position, SEEK_SET);
}

t_wav_uint32 wav_read(void * p_user_data,void * p_buffer,t_wav_uint32 p_bytes)
{
	return (t_wav_uint32) fread(p_buffer, 1, p_bytes, (FILE*) p_user_data);
}

static void
usage(const char *exename)
{
	printf("Usage: %s <file1.wav> <file2.wav> [<diff.wav>]\n", exename);
}

int
main(int argc, char **argv)
{
	t_wav_output_file wav_output;
	t_wav_input_file wav_in_1;
	t_wav_input_file wav_in_2;
	int is_wav_output;
	int err;
	int total_samples, total_diff;

	if(4 < argc && argc < 3)
	{
		usage(argv[0]);
		return 0;
	}

	t_wav_input_file_callback wavi_fc;
	memset(&wav_in_1, 0, sizeof wav_in_1);
	wavi_fc.m_read      = wav_read;
	wavi_fc.m_user_data = fopen(argv[1], "rb");
	if(!wavi_fc.m_user_data) return 1;
	if (! waveformat_input_open(&wav_in_1, wavi_fc)) return 1;

	memset(&wav_in_2, 0, sizeof wav_in_2);
	wavi_fc.m_read      = wav_read;
	wavi_fc.m_user_data = fopen(argv[2], "rb");
	if(!wavi_fc.m_user_data) return 1;
	if (! waveformat_input_open(&wav_in_2, wavi_fc)) return 1;

	if (wav_in_1.m_channels != wav_in_2.m_channels) {
		printf("Channel number doesn't match\n");
		return 1;
	}
	if (wav_in_1.m_samples_per_sec != wav_in_2.m_samples_per_sec) {
		printf("Sample rate doesn't match\n");
		return 1;
	}
	if (wav_in_1.m_bits_per_sample != wav_in_2.m_bits_per_sample) {
		printf("Sample size doesn't match\n");
		return 1;
	}
	if (wav_in_1.m_data_size != wav_in_2.m_data_size) {
		printf("File length doesn't match\n");
		return 1;
	}

	is_wav_output = argc > 3;
	if(is_wav_output)
	{
		t_wav_output_file_callback wavo_fc;
		memset(&wav_output, 0, sizeof wav_output);
		wavo_fc.m_seek      = wav_seek;
		wavo_fc.m_write     = wav_write;
		wavo_fc.m_user_data = fopen(argv[3], "wb");
		if(!wavo_fc.m_user_data) return 1;
		err = waveformat_output_open(&wav_output, wavo_fc, wav_in_1.m_channels, wav_in_1.m_bits_per_sample, 0, wav_in_1.m_samples_per_sec, wav_in_2.m_data_size);
		if(!err) return 1;
	}

 	total_samples = 0;
	total_diff = 0;

	while(1)
	{
		short sample_buff[2][512];
		unsigned int samples[2];

		samples[0] = waveformat_input_process_int16(& wav_in_1, sample_buff[0], 512);
		samples[1] = waveformat_input_process_int16(& wav_in_2, sample_buff[1], 512);

		if (samples[0] != samples[1] || samples[0] == 0)
			break;

		int i = 0;
		for( ; i < samples[0]; i++) {
			sample_buff[0][i] -= sample_buff[1][i];
			if (sample_buff[0][i]) {
				printf("diff @ sample %i channel %i : %i\n", (total_samples + i) / wav_in_1.m_channels, (total_samples + i) % wav_in_1.m_channels, sample_buff[0][i]);
				total_diff++;
			}
		}
		total_samples += samples[0];

		if(is_wav_output)
			if(waveformat_output_process_int16(&wav_output, sample_buff[0], samples[0]) < 0)
				break;
	}

	if (total_diff != 0)
		printf("%i diff found\n", total_diff);
	else
		printf("no diff found\n");

	waveformat_input_close(&wav_in_1);
	waveformat_input_close(&wav_in_2);
	fclose(wav_in_1.m_callback.m_user_data);
	fclose(wav_in_2.m_callback.m_user_data);

	if(is_wav_output)
	{
		waveformat_output_close(&wav_output);
		fclose(wav_output.m_callback.m_user_data);
	}

    return 0;
}
