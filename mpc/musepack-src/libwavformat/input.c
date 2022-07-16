#include "libwaveformat.h"

typedef struct {
	t_wav_uint8 m_data[4];
} t_riff_header;

static t_wav_uint32 riff_compare(t_riff_header p_chunk,t_wav_uint8 a,t_wav_uint8 b,t_wav_uint8 c,t_wav_uint8 d)
{
	return (p_chunk.m_data[0] == a && p_chunk.m_data[1] == b && p_chunk.m_data[2] == c && p_chunk.m_data[3] == d) ? 1 : 0;
}


static void g_convert_float32_to_float32(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_conv temp;

		temp.n = (t_wav_uint32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_uint32)p_input[2] << 16) |
			( (t_wav_uint32)p_input[3] << 24)
			);

		p_input += 4;

		p_sample_buffer[n] = temp.f;
	}
}

static void g_convert_float32_to_int16(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_conv temp;
		t_wav_int32 tempi;
		temp.n = (t_wav_uint32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_uint32)p_input[2] << 16) |
			( (t_wav_uint32)p_input[3] << 24)
			);

		p_input += 4;

		tempi = (t_wav_int32)(temp.f * 0x8000);
		if (tempi < -0x8000) tempi = -0x8000;
		else if (tempi > 0x7FFF) tempi = 0x7FFF;

		p_sample_buffer[n] = (t_wav_int16) tempi;
	}
}

static void g_convert_int32_to_float32(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_uint32)p_input[2] << 16) |
			( (t_wav_uint32)p_input[3] << 24)
			);

		p_input += 4;

		p_sample_buffer[n] = (t_wav_float32) temp / (t_wav_float32) 0x80000000 ;
	}
}

static void g_convert_int32_to_int16(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_uint32)p_input[2] << 16) |
			( (t_wav_uint32)p_input[3] << 24)
			);

		p_input += 4;

		p_sample_buffer[n] = (t_wav_int16) ( temp >> 16 );
	}
}

static void g_convert_int24_to_float32(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_int32)(t_wav_int8)p_input[2] << 16)
			);

		p_input += 3;

		p_sample_buffer[n] = (t_wav_float32) temp / (t_wav_float32) 0x800000 ;
	}
}

static void g_convert_int24_to_int16(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int32 temp = (t_wav_int32) (
			( (t_wav_uint32)p_input[0] ) |
			( (t_wav_uint32)p_input[1] << 8) |
			( (t_wav_int32)(t_wav_int8)p_input[2] << 16)
			);

		p_input += 3;

		p_sample_buffer[n] = (t_wav_int16) ( temp >> 8 );
	}
}

static void g_convert_int16_to_float32(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int16 temp = (t_wav_int16)
			(
				(t_wav_uint16)p_input[0] |
				( (t_wav_uint16)p_input[1] << 8 )
			);

		p_input += 2;

		p_sample_buffer[n] = (t_wav_float32) temp / (t_wav_float32) 0x8000 ;
	}
}

static void g_convert_int16_to_int16(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int16 temp = (t_wav_int16)
			(
				(t_wav_uint16)p_input[0] |
				( (t_wav_uint16)p_input[1] << 8 )
			);

		p_input += 2;

		p_sample_buffer[n] = temp;
	}
}

static void g_convert_uint8_to_float32(t_wav_uint8 const * p_input,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int8 temp = (t_wav_int8)( *(p_input++) ^ 0x80 );
		p_sample_buffer[n] = (t_wav_float32) temp / (t_wav_float32) 0x80;
	}
}

static void g_convert_uint8_to_int16(t_wav_uint8 const * p_input,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 n;
	for(n=0;n<p_sample_count;n++)
	{
		t_wav_int8 temp = (t_wav_int8)( *(p_input++) ^ 0x80 );
		p_sample_buffer[n] = (t_wav_int16) temp << 8;
	}
}

static t_wav_input_handler g_input_handler_float32 = {g_convert_float32_to_float32,g_convert_float32_to_int16};
static t_wav_input_handler g_input_handler_int32 = {g_convert_int32_to_float32,g_convert_int32_to_int16};
static t_wav_input_handler g_input_handler_int24 = {g_convert_int24_to_float32,g_convert_int24_to_int16};
static t_wav_input_handler g_input_handler_int16 = {g_convert_int16_to_float32,g_convert_int16_to_int16};
static t_wav_input_handler g_input_handler_uint8 = {g_convert_uint8_to_float32,g_convert_uint8_to_int16};

static t_wav_uint32 waveformat_read(t_wav_input_file * p_file,void * p_buffer,t_wav_uint32 p_bytes)
{
	return p_file->m_callback.m_read(p_file->m_callback.m_user_data,p_buffer,p_bytes);
}

static t_wav_uint32 waveformat_skip(t_wav_input_file * p_file,t_wav_uint32 p_bytes)
{
	t_wav_uint8 dummy[256];
	t_wav_uint32 delta,done,delta_done;

	done = 0;

	while(done < p_bytes)
	{
		delta = p_bytes - done;
		if (delta > sizeof(dummy)) delta = sizeof(dummy);

		delta_done = waveformat_read(p_file,dummy,delta);

		done += delta_done;

		if (delta_done != delta) break;
	}

	return done;
}


static t_wav_uint32 waveformat_read_riff(t_wav_input_file * p_file,t_riff_header * p_header)
{
	return waveformat_read(p_file,p_header,sizeof(t_riff_header)) / sizeof(t_riff_header);
}

static t_wav_uint32 waveformat_read_uint32(t_wav_input_file * p_file,t_wav_uint32 * p_value)
{
	t_wav_uint8 temp[4];
	if (waveformat_read(p_file,&temp,sizeof(temp)) != sizeof(temp)) return 0;
	* p_value =
		((t_wav_uint32)temp[0]) |
		((t_wav_uint32)temp[1] << 8) |
		((t_wav_uint32)temp[2] << 16) |
		((t_wav_uint32)temp[3] << 24);
	return 1;
}

static t_wav_uint32 waveformat_read_uint16(t_wav_input_file * p_file,t_wav_uint16 * p_value)
{
	t_wav_uint8 temp[2];
	if (waveformat_read(p_file,&temp,sizeof(temp)) != sizeof(temp)) return 0;
	* p_value =
		((t_wav_uint16)temp[0]) |
		((t_wav_uint16)temp[1] << 8);
	return 1;
}

t_wav_uint32 waveformat_input_open(t_wav_input_file * p_file,t_wav_input_file_callback p_callback)
{
	t_riff_header header;
	t_wav_uint32 main_size,main_offset,chunk_size;
	t_wav_uint8 found_fmt;

	found_fmt = 0;

	p_file->m_callback = p_callback;

	if (waveformat_read_riff(p_file,&header) != 1) return 0;

	if (!riff_compare(header,'R','I','F','F')) return 0;

	if (waveformat_read_uint32(p_file,&main_size) != 1) return 0;

	if (main_size < 4) return 0;


	if (waveformat_read_riff(p_file,&header) != 1) return 0;

	if (!riff_compare(header,'W','A','V','E')) return 0;

	main_offset = 4;

	for(;;)
	{
		if (main_size - main_offset < 8) return 0;

		if (waveformat_read_riff(p_file,&header) != 1) return 0;
		if (waveformat_read_uint32(p_file,&chunk_size) != 1) return 0;

		main_offset += 8;

		if (main_size - main_offset < chunk_size) return 0;



		if (riff_compare(header,'f','m','t',' '))
		{
			t_wav_uint32 fmt_remaining;
			if (found_fmt) return 0;//duplicate fmt chunk

			fmt_remaining = chunk_size;

			if (fmt_remaining < 2+2+4+4+2+2) return 0;

			if (waveformat_read_uint16(p_file,&p_file->m_format_tag			) != 1) return 0;
			if (waveformat_read_uint16(p_file,&p_file->m_channels			) != 1) return 0;
			if (waveformat_read_uint32(p_file,&p_file->m_samples_per_sec	) != 1) return 0;
			if (waveformat_read_uint32(p_file,&p_file->m_avg_bytes_per_sec	) != 1) return 0;
			if (waveformat_read_uint16(p_file,&p_file->m_block_align		) != 1) return 0;
			if (waveformat_read_uint16(p_file,&p_file->m_bits_per_sample	) != 1) return 0;

			p_file->m_bytes_per_sample = p_file->m_bits_per_sample / 8;

			if (p_file->m_bytes_per_sample == 0) return 0;

			p_file->m_buffer_size = sizeof(p_file->m_workbuffer) / p_file->m_bytes_per_sample;

			fmt_remaining -= 2+2+4+4+2+2;

			switch(p_file->m_format_tag)
			{
			case waveformat_tag_int:
				switch(p_file->m_bits_per_sample)
				{
				case 8:
					p_file->m_input_handler = g_input_handler_uint8;
					break;
				case 16:
					p_file->m_input_handler = g_input_handler_int16;
					break;
				case 24:
					p_file->m_input_handler = g_input_handler_int24;
					break;
				case 32:
					p_file->m_input_handler = g_input_handler_int32;
					break;
				default:
					//unsupported format
					return 0;
				}
				break;
			case waveformat_tag_float:
				switch(p_file->m_bits_per_sample)
				{
				case 32:
					p_file->m_input_handler = g_input_handler_float32;
					break;
#if 0
				case 64:

					break;
#endif
				default:
					//unsupported format
					return 0;
				}
				break;
			default:
				//unsupported format
				return 0;
			}

			if (chunk_size & 1) fmt_remaining++;

			if (fmt_remaining > 0)
			{
				if (waveformat_skip(p_file,fmt_remaining) != fmt_remaining) return 0;
			}

			main_offset += chunk_size;
			if (chunk_size & 1) main_offset++;

			found_fmt = 1;
		}
		else if (riff_compare(header,'d','a','t','a'))
		{
			if (!found_fmt) return 0;//found data before fmt, don't know how to handle data
			//found parsable data chunk, ok to proceed
			p_file->m_data_size = chunk_size / p_file->m_bytes_per_sample;
			p_file->m_data_position = 0;

			break;
		}
		else
		{//unknown chunk, let's skip over
			t_wav_uint32 toskip = chunk_size;
			if (toskip & 1) toskip++;

			if (waveformat_skip(p_file,toskip) != toskip) return 0;

			main_offset += toskip;
		}
	}

	return 1;
}

t_wav_uint32 waveformat_input_process_float32(t_wav_input_file * p_file,t_wav_float32 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 samples_read;

	samples_read = 0;

	if (p_file->m_data_position + p_sample_count > p_file->m_data_size)
		p_sample_count = p_file->m_data_size - p_file->m_data_position;

	while(samples_read < p_sample_count)
	{
		t_wav_uint32 delta, deltaread;

		delta = p_sample_count - samples_read;
		if (delta > p_file->m_buffer_size) delta = p_file->m_buffer_size;

		deltaread = waveformat_read(p_file,p_file->m_workbuffer,delta * p_file->m_bytes_per_sample) / p_file->m_bytes_per_sample;

		if (deltaread > 0)
		{
			p_file->m_input_handler.m_convert_float32(p_file->m_workbuffer,p_sample_buffer + samples_read,deltaread);

			samples_read += deltaread;
		}

		if (deltaread != delta) break;
	}

	p_file->m_data_position += samples_read;

	return samples_read;
}

t_wav_uint32 waveformat_input_process_int16(t_wav_input_file * p_file,t_wav_int16 * p_sample_buffer,t_wav_uint32 p_sample_count)
{
	t_wav_uint32 samples_read;

	samples_read = 0;

	if (p_file->m_data_position + p_sample_count > p_file->m_data_size)
		p_sample_count = p_file->m_data_size - p_file->m_data_position;

	while(samples_read < p_sample_count)
	{
		t_wav_uint32 delta, deltaread;

		delta = p_sample_count - samples_read;
		if (delta > p_file->m_buffer_size) delta = p_file->m_buffer_size;

		deltaread = waveformat_read(p_file,p_file->m_workbuffer,delta * p_file->m_bytes_per_sample) / p_file->m_bytes_per_sample;

		if (deltaread > 0)
		{
			p_file->m_input_handler.m_convert_int16(p_file->m_workbuffer,p_sample_buffer + samples_read,deltaread);

			samples_read += deltaread;
		}

		if (deltaread != delta) break;
	}

	p_file->m_data_position += samples_read;

	return samples_read;
}

void waveformat_input_close(t_wav_input_file * p_file)
{
}

t_wav_uint32 waveformat_input_query_sample_rate(t_wav_input_file * p_file)
{
	return p_file->m_samples_per_sec;
}
t_wav_uint32 waveformat_input_query_channels(t_wav_input_file * p_file)
{
	return p_file->m_channels;
}
t_wav_uint32 waveformat_input_query_length(t_wav_input_file * p_file)
{
	return p_file->m_data_size;
}
