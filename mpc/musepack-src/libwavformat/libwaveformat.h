#ifndef __LIBWAVEFORMAT_H__
#define __LIBWAVEFORMAT_H__
#ifdef WIN32
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//general declarations

#ifdef _MSC_VER
typedef __int8 t_wav_int8;
typedef unsigned __int8 t_wav_uint8;
typedef __int16 t_wav_int16;
typedef unsigned __int16 t_wav_uint16;
typedef __int32 t_wav_int32;
typedef unsigned __int32 t_wav_uint32;
typedef __int64 t_wav_int64;
typedef unsigned __int64 t_wav_uint64;
typedef float t_wav_float32;
typedef double t_wav_float64;
#else
#include <stdint.h>
typedef int8_t t_wav_int8;
typedef uint8_t t_wav_uint8;
typedef int16_t t_wav_int16;
typedef uint16_t t_wav_uint16;
typedef int32_t t_wav_int32;
typedef uint32_t t_wav_uint32;
typedef int64_t t_wav_int64;
typedef uint64_t t_wav_uint64;
typedef float t_wav_float32;
typedef double t_wav_float64;
#endif

typedef union
{
	t_wav_float32 f;
	t_wav_uint32 n;
} t_wav_conv;

#define waveformat_tag_int 1
#define waveformat_tag_float 3

//WAV file reader

typedef struct
{
	t_wav_uint32 (*m_read)(void * p_user_data,void * p_buffer,t_wav_uint32 p_bytes);
	void * m_user_data;
} t_wav_input_file_callback;

typedef struct
{
	void (*m_convert_float32)(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count);
	void (*m_convert_int16)(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count);
} t_wav_input_handler;

typedef struct
{
	t_wav_input_file_callback m_callback;

	t_wav_input_handler m_input_handler;

	t_wav_uint16 m_format_tag;
	t_wav_uint16 m_channels;
	t_wav_uint32 m_samples_per_sec;
	t_wav_uint32 m_avg_bytes_per_sec;
	t_wav_uint16 m_block_align;
	t_wav_uint16 m_bits_per_sample;

	t_wav_uint32 m_bytes_per_sample, m_buffer_size;

	t_wav_uint32 m_data_size;
	t_wav_uint32 m_data_position;



	t_wav_uint8 m_workbuffer[512];
} t_wav_input_file;

t_wav_uint32 waveformat_input_open(t_wav_input_file * p_file,t_wav_input_file_callback p_callback);

t_wav_uint32 waveformat_input_process_float32(t_wav_input_file * p_file,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count);
t_wav_uint32 waveformat_input_process_int16(t_wav_input_file * p_file,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count);

void waveformat_input_close(t_wav_input_file * p_file);

t_wav_uint32 waveformat_input_query_sample_rate(t_wav_input_file * p_file);
t_wav_uint32 waveformat_input_query_channels(t_wav_input_file * p_file);
t_wav_uint32 waveformat_input_query_length(t_wav_input_file * p_file);

//WAV file writer

typedef struct
{
	t_wav_uint32 (*m_write)(void * p_user_data,void const * p_buffer,t_wav_uint32 p_bytes);
	t_wav_uint32 (*m_seek)(void * p_user_data,t_wav_uint32 p_position);
	void * m_user_data;
} t_wav_output_file_callback;

typedef struct
{
	void (*m_convert_float32)(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count);
	void (*m_convert_int16)(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count);
} t_wav_output_handler;

typedef struct
{
	t_wav_output_file_callback m_callback;

	t_wav_output_handler m_output_handler;

	t_wav_uint32 m_channels;
	t_wav_uint32 m_bits_per_sample;
	t_wav_uint32 m_float;
	t_wav_uint32 m_sample_rate;
	t_wav_uint32 m_samples_written,m_samples_written_expected;

	t_wav_uint32 m_bytes_per_sample, m_buffer_size;

	t_wav_uint8 m_workbuffer[512];
} t_wav_output_file;

t_wav_uint32 waveformat_output_open(t_wav_output_file * p_file,t_wav_output_file_callback p_callback,t_wav_uint32 p_channels,t_wav_uint32 p_bits_per_sample,t_wav_uint32 p_float,t_wav_uint32 p_sample_rate,t_wav_uint32 p_expected_samples);

t_wav_uint32 waveformat_output_process_float32(t_wav_output_file * p_file,t_wav_float32 const * p_sample_buffer,t_wav_uint32 p_sample_count);
t_wav_uint32 waveformat_output_process_int16(t_wav_output_file * p_file,t_wav_int16 const * p_sample_buffer,t_wav_uint32 p_sample_count);

t_wav_uint32 waveformat_output_close(t_wav_output_file * p_file);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //__LIBWAVEFORMAT_H__

