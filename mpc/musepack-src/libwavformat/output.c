#include "libwaveformat.h"

__inline static void write_int24(t_wav_uint8 * p_output,t_wav_int32 p_value)
{
	p_output[0] = (t_wav_uint8)(p_value);
	p_output[1] = (t_wav_uint8)(p_value>>8);
	p_output[2] = (t_wav_uint8)(p_value>>16);
}

__inline static void write_int32(t_wav_uint8 * p_output,t_wav_int32 p_value)
{
	p_output[0] = (t_wav_uint8)(p_value);
	p_output[1] = (t_wav_uint8)(p_value>>8);
	p_output[2] = (t_wav_uint8)(p_value>>16);
	p_output[3] = (t_wav_uint8)(p_value>>24);
}

__inline static void write_float(t_wav_uint8 * p_output,t_wav_float32 p_value)
{
	t_wav_conv bah;
	bah.f = p_value;
	p_output[0] = (t_wav_uint8)bah.n;
	p_output[1] = (t_wav_uint8)(bah.n >> 8);
	p_output[2] = (t_wav_uint8)(bah.n >> 16);
	p_output[3] = (t_wav_uint8)(bah.n >> 24);
}

static void g_convert_float32_to_uint8(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) ( p_sample_buffer[n] * 0x80 );
		if (temp < -0x80) temp = -0x80;
		else if (temp > 0x7F) temp = 0x7F;
		*(p_output++) = (t_wav_uint8)temp ^ 0x80;
	}
}

static void g_convert_int16_to_uint8(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		*(p_output++) = (t_wav_uint8)(p_sample_buffer[n] >> 8) ^ 0x80;
	}
}

static void g_convert_float32_to_int16(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) ( p_sample_buffer[n] * 0x8000 );
		if (temp < -0x8000) temp = -0x8000;
		else if (temp > 0x7FFF) temp = 0x7FFF;
		*(p_output++) = (t_wav_uint8)((t_wav_uint32)temp);
		*(p_output++) = (t_wav_uint8)((t_wav_uint32)temp >> 8);
	}
}

static void g_convert_int16_to_int16(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int16 temp = p_sample_buffer[n];
		*(p_output++) = (t_wav_uint8)((t_wav_uint16)temp);
		*(p_output++) = (t_wav_uint8)((t_wav_uint16)temp >> 8);
	}
}

static void g_convert_float32_to_int24(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_int24(p_output,(t_wav_int32) ( p_sample_buffer[n] * 0x800000 ));
		p_output += 3;
	}
}

static void g_convert_int16_to_int24(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_int24(p_output,(t_wav_int32) p_sample_buffer[n] << 8 );
		p_output += 3;
	}
}

static void g_convert_float32_to_int32(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_int32(p_output,(t_wav_int32) ( p_sample_buffer[n] * 0x80000000 ));
		p_output += 4;
	}
}

static void g_convert_int16_to_int32(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_int32(p_output,(t_wav_int32) p_sample_buffer[n] << 16 );
		p_output += 4;
	}
}

static void g_convert_float32_to_float32(t_wav_float32 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_float(p_output,p_sample_buffer[n]);
		p_output += 4;
	}
}

static void g_convert_int16_to_float32(t_wav_int16 const * p_sample_buffer,t_wav_uint8 * p_output,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		write_float(p_output,(t_wav_float32)p_sample_buffer[n] / (t_wav_float32) 0x8000);
		p_output += 4;
	}
}

static const t_wav_output_handler g_wav_output_handler_uint8 = {g_convert_float32_to_uint8,g_convert_int16_to_uint8};
static const t_wav_output_handler g_wav_output_handler_int16 = {g_convert_float32_to_int16,g_convert_int16_to_int16};
static const t_wav_output_handler g_wav_output_handler_int24 = {g_convert_float32_to_int24,g_convert_int16_to_int24};
static const t_wav_output_handler g_wav_output_handler_int32 = {g_convert_float32_to_int32,g_convert_int16_to_int32};
static const t_wav_output_handler g_wav_output_handler_float32 = {g_convert_float32_to_float32,g_convert_int16_to_float32};

typedef struct {
	t_wav_uint8 m_data[4];
} t_riff_header;

static const t_riff_header g_header_riff = {{'R','I','F','F'}};
static const t_riff_header g_header_wave = {{'W','A','V','E'}};
static const t_riff_header g_header_data = {{'d','a','t','a'}};
static const t_riff_header g_header_fmt  = {{'f','m','t',' '}};

static t_wav_uint32 waveformat_write(t_wav_output_file * p_file,void const * p_buffer,t_wav_uint32 p_buffer_size)
{
	return p_file->m_callback.m_write(p_file->m_callback.m_user_data,p_buffer,p_buffer_size);
}

static t_wav_uint32 waveformat_seek(t_wav_output_file * p_file,t_wav_uint32 p_position)
{
	return p_file->m_callback.m_seek(p_file->m_callback.m_user_data,p_position);
}

static t_wav_uint32 waveformat_write_riff(t_wav_output_file * p_file,t_riff_header p_riff)
{
	return waveformat_write(p_file,&p_riff,sizeof(p_riff)) / sizeof(p_riff);
}

static t_wav_uint32 waveformat_write_uint32(t_wav_output_file * p_file,t_wav_uint32 p_val)
{
	t_wav_uint8 temp[4] = {
		(t_wav_uint8)(p_val & 0xFF),
		(t_wav_uint8)((p_val >> 8) & 0xFF),
		(t_wav_uint8)((p_val >> 16) & 0xFF),
		(t_wav_uint8)((p_val >> 24) & 0xFF)
	};

	return waveformat_write(p_file,temp,4) / 4;
}

static t_wav_uint32 waveformat_write_uint16(t_wav_output_file * p_file,t_wav_uint16 p_val)
{
	t_wav_uint8 temp[2] = {
		(t_wav_uint8)(p_val & 0xFF),
		(t_wav_uint8)((p_val >> 8) & 0xFF)
	};

	return waveformat_write(p_file,temp,2) / 2;
}

static t_wav_uint32 calculate_riff_size(t_wav_output_file * p_file,t_wav_uint32 p_samples_written)
{
	t_wav_uint32 bytes_written = p_samples_written * p_file->m_bytes_per_sample, bytes_written_padded = bytes_written;
	if (bytes_written & 1)//padding
		bytes_written_padded++;

	return bytes_written_padded + 4 + 8 + 2+2+4+4+2+2 + 8;
}

static t_wav_uint32 calculate_data_size(t_wav_output_file * p_file,t_wav_uint32 p_samples_written)
{
	return p_samples_written * p_file->m_bytes_per_sample;
}

t_wav_uint32 waveformat_output_open(t_wav_output_file * p_file,t_wav_output_file_callback p_callback,t_wav_uint32 p_channels,t_wav_uint32 p_bits_per_sample,t_wav_uint32 p_float,t_wav_uint32 p_sample_rate,t_wav_uint32 p_samples_written_expected)
{
	p_file->m_callback = p_callback;
	p_file->m_channels = p_channels;
	p_file->m_bits_per_sample = p_bits_per_sample;
	p_file->m_float = p_float;
	p_file->m_sample_rate = p_sample_rate;
	p_file->m_bytes_per_sample = p_bits_per_sample >> 3;
	if (p_file->m_bytes_per_sample == 0) return 0;
	p_file->m_buffer_size = sizeof(p_file->m_workbuffer) / p_file->m_bytes_per_sample;
	p_file->m_samples_written_expected = p_samples_written_expected;

	if (p_float)
	{
		switch(p_bits_per_sample)
		{
		case 32:
			p_file->m_output_handler = g_wav_output_handler_float32;
			break;
		default:
			return 0;
		}
	}
	else
	{
		switch(p_bits_per_sample)
		{
		case 8:
			p_file->m_output_handler = g_wav_output_handler_uint8;
			break;
		case 16:
			p_file->m_output_handler = g_wav_output_handler_int16;
			break;
		case 24:
			p_file->m_output_handler = g_wav_output_handler_int24;
			break;
		case 32:
			p_file->m_output_handler = g_wav_output_handler_int32;
			break;
		default:
			return 0;
		}
	}

	if (waveformat_write_riff(p_file,g_header_riff) != 1) return 0;
	if (waveformat_write_uint32(p_file,calculate_riff_size(p_file,p_file->m_samples_written_expected)) != 1) return 0; //to be possibly rewritten later, offset : 4
	if (waveformat_write_riff(p_file,g_header_wave) != 1) return 0;
//offset: 12
	if (waveformat_write_riff(p_file,g_header_fmt) != 1) return 0;
	if (waveformat_write_uint32(p_file,2+2+4+4+2+2) != 1) return 0;
//offset: 12 + 8
	if (waveformat_write_uint16(p_file,p_float ? waveformat_tag_float : waveformat_tag_int) != 1) return 0;
	if (waveformat_write_uint16(p_file,(t_wav_uint16) p_channels) != 1) return 0;
	if (waveformat_write_uint32(p_file,p_sample_rate) != 1) return 0;
	if (waveformat_write_uint32(p_file,(t_wav_uint32)(p_sample_rate * p_file->m_bytes_per_sample * p_channels)) != 1) return 0;
	if (waveformat_write_uint16(p_file,(t_wav_uint16)(p_file->m_bytes_per_sample * p_channels)) != 1) return 0;
	if (waveformat_write_uint16(p_file,(t_wav_uint16)p_bits_per_sample) != 1) return 0;
//offset: 12 + 8 + 2+2+4+4+2+2
	if (waveformat_write_riff(p_file,g_header_data) != 1) return 0;
	if (waveformat_write_uint32(p_file,calculate_data_size(p_file,p_file->m_samples_written_expected)) != 1) return 0; //to be possibly rewritten later, offset : 12 + 8 + 2+2+4+4+2+2 + 4
//total header size: 12 + 8 + 2+2+4+4+2+2 + 8
	p_file->m_samples_written = 0;

	return 1;
}


t_wav_uint32 waveformat_output_process_float32(t_wav_output_file * p_file,t_wav_float32 const * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 samples_done;

	samples_done = 0;

	while(samples_done < p_sample_count)
	{
		t_wav_uint32 delta = p_sample_count - samples_done, delta_written;
		if (delta > p_file->m_buffer_size) delta = p_file->m_buffer_size;

		p_file->m_output_handler.m_convert_float32(p_sample_buffer + samples_done,p_file->m_workbuffer,delta);

		delta_written = waveformat_write(p_file,p_file->m_workbuffer,delta * p_file->m_bytes_per_sample) / p_file->m_bytes_per_sample;

		if (delta_written > 0)
		{
			samples_done += delta_written;
		}

		if (delta_written != delta) break;
	}

	p_file->m_samples_written += samples_done;

	return samples_done;
}

t_wav_uint32 waveformat_output_process_int16(t_wav_output_file * p_file,t_wav_int16 const * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 samples_done;

	samples_done = 0;

	while(samples_done < p_sample_count)
	{
		t_wav_uint32 delta = p_sample_count - samples_done, delta_written;
		if (delta > p_file->m_buffer_size) delta = p_file->m_buffer_size;

		p_file->m_output_handler.m_convert_int16(p_sample_buffer + samples_done,p_file->m_workbuffer,delta);

		delta_written = waveformat_write(p_file,p_file->m_workbuffer,delta * p_file->m_bytes_per_sample) / p_file->m_bytes_per_sample;

		if (delta_written > 0)
		{
			samples_done += delta_written;
		}

		if (delta_written != delta) break;
	}

	p_file->m_samples_written += samples_done;

	return samples_done;
}

t_wav_uint32 waveformat_output_close(t_wav_output_file * p_file)
{
	if ((p_file->m_samples_written * p_file->m_bytes_per_sample) & 1)//padding
	{
		t_wav_uint8 meh = 0;
		if (waveformat_write(p_file,&meh,1) != 1) return 0;
	}

	if (p_file->m_samples_written != p_file->m_samples_written_expected)
	{
		if (!waveformat_seek(p_file,4)) return 0;
		if (waveformat_write_uint32(p_file,calculate_riff_size(p_file,p_file->m_samples_written)) != 1) return 0;
		if (!waveformat_seek(p_file,12 + 8 + 2+2+4+4+2+2 + 4)) return 0;
		if (waveformat_write_uint32(p_file,calculate_data_size(p_file,p_file->m_samples_written)) != 1) return 0;
	}
	return 1;
}
