/*
 * Musepack audio compression
 * Copyright (c) 2005-2009, The Musepack Development Team
 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf
 * Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#import <mpc/mpc_types.h>
#import <mpc/libmpcenc.h>

struct mpc_stream_encoder_t;
typedef struct mpc_stream_encoder_t mpc_stream_encoder;

// Create and destroy
mpc_stream_encoder * mpc_stream_encoder_create(void);
void mpc_stream_encoder_destroy(mpc_stream_encoder *enc);

// Quality [0.0, 10.0]
mpc_status mpc_stream_encoder_set_quality(mpc_stream_encoder *enc, float quality);

mpc_status mpc_stream_encoder_set_estimated_total_frames(mpc_stream_encoder *enc, mpc_uint64_t frames);

// Must be even
mpc_status mpc_stream_encoder_set_frames_block_power(mpc_stream_encoder *enc, unsigned int frames_block_power);
mpc_status mpc_stream_encoder_set_seek_distance(mpc_stream_encoder *enc, unsigned int seek_distance);

mpc_status mpc_stream_encoder_init(mpc_stream_encoder *enc, float samplerate, int channels, mpc_write_callback_t write_callback, mpc_seek_callback_t seek_callback, mpc_tell_callback_t tell_callback, void *vio_context);

mpc_status mpc_stream_encoder_encode(mpc_stream_encoder *enc, const mpc_int16_t *data, unsigned int frames);

mpc_status mpc_stream_encoder_finish(mpc_stream_encoder *enc);
