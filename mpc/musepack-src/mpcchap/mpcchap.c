/*
  Copyright (c) 2008-2009, The Musepack Development Team
  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <mpc/mpcdec.h>
#include "../libmpcdec/internal.h"
#include "../libmpcenc/libmpcenc.h"
#include "iniparser.h"

#include <sys/stat.h>

#include <cuetools/cuefile.h>

// tags.c
void    Init_Tags        ( void );
int     FinalizeTags     ( FILE* fp, unsigned int Version, unsigned int flags );
int     addtag           ( const char* key, size_t keylen, const char* value,
                           size_t valuelen, int converttoutf8, int flags );
#define TAG_NO_HEADER 1
#define TAG_NO_FOOTER 2
#define TAG_NO_PREAMBLE 4
#define TAG_VERSION 2000

#ifdef _MSC_VER
# include <io.h>
# define atoll		_atoi64
# define ftruncate	chsize
# define strcasecmp stricmp
#endif

#define MPCCHAP_MAJOR 0
#define MPCCHAP_MINOR 9
#define MPCCHAP_BUILD 0

#define _cat(a,b,c) #a"."#b"."#c
#define cat(a,b,c) _cat(a,b,c)
#define MPCCHAP_VERSION cat(MPCCHAP_MAJOR,MPCCHAP_MINOR,MPCCHAP_BUILD)

const char About[] = "%s - Musepack (MPC) sv8 chapter editor v" MPCCHAP_VERSION " (C) 2008-2009 MDT\nBuilt " __DATE__ " " __TIME__ "\n";


static const int Ptis[] = {	PTI_TITLE, PTI_PERFORMER, PTI_SONGWRITER, PTI_COMPOSER,
		PTI_ARRANGER, PTI_MESSAGE, PTI_GENRE};
static char const * const APE_keys[] = {"Title", "Artist", "Songwriter", "Composer",
		"Arranger", "Comment", "Genre"};

static void usage(const char *exename)
{
	fprintf(stderr,
	        "Usage: %s <infile.mpc> <chapterfile.ini / cuefile>\n"
	        "   if chapterfile.ini exists, chapter tags in infile.mpc will be\n"
	        "   replaced by those from chapterfile.ini, else chapters will be\n"
	        "   dumped to chapterfile.ini\n"
	        "   chapterfile.ini is something like :\n"
	        "   	[chapter_start_sample]\n"
	        "   	SomeKey=Some Value\n"
	        "   	SomeOtherKey=Some Other Value\n"
	        "   	[other_chapter_start]\n"
	        "   	YouKnowWhatKey=I think you start to understand ...\n"
	        , exename);
}

mpc_status add_chaps_ini(char * mpc_file, char * chap_file, mpc_demux * demux, mpc_streaminfo * si)
{
	struct stat stbuf;
	FILE * in_file;
	int chap_pos, end_pos, chap_size, i, nchap;
	char * tmp_buff;
	dictionary * dict;

	chap_pos = (demux->chap_pos >> 3) + si->header_position;
	end_pos = mpc_demux_pos(demux) >> 3;
	chap_size = end_pos - chap_pos;

	stat(mpc_file, &stbuf);
	tmp_buff = malloc(stbuf.st_size - chap_pos - chap_size);
	in_file = fopen( mpc_file, "r+b" );
	fseek(in_file, chap_pos + chap_size, SEEK_SET);
	fread(tmp_buff, 1, stbuf.st_size - chap_pos - chap_size, in_file);
	fseek(in_file, chap_pos, SEEK_SET);

	dict = iniparser_load(chap_file);

	nchap = iniparser_getnsec(dict);
	for (i = 0; i < nchap; i++) {
		int j, nitem, ntags = 0, tag_len = 0, offset_size;
		mpc_uint16_t gain = 0, peak = 0;
		char * chap_sec = iniparser_getsecname(dict, i), block_header[12] = "CT", sample_offset[10];
		mpc_int64_t chap_pos = atoll(chap_sec);

		if (chap_pos > si->samples - si->beg_silence)
			fprintf(stderr, "warning : chapter %i starts @ %lli samples after the end of the stream (%lli)\n",
			        i + 1, chap_pos, si->samples - si->beg_silence);

		Init_Tags();

		nitem = iniparser_getnkey(dict, i);
		for (j = 0; j < nitem; j++) {
			char * item_key, * item_value;
			int key_len, item_len;
			item_key = iniparser_getkeyname(dict, i, j, & item_value);
			if (strcmp(item_key, "gain") == 0)
				gain = atoi(item_value);
			else if (strcmp(item_key, "peak") == 0)
				peak = atoi(item_value);
			else {
				key_len = strlen(item_key);
				item_len = strlen(item_value);
				addtag(item_key, key_len, item_value, item_len, 0, 0);
				tag_len += key_len + item_len;
				ntags++;
			}
		}
		if (ntags > 0) tag_len += 24 + ntags * 9;

		offset_size = encodeSize(chap_pos, sample_offset, MPC_FALSE);
		tag_len = encodeSize(tag_len + 4 + offset_size + 2, block_header + 2, MPC_TRUE);
		fwrite(block_header, 1, tag_len + 2, in_file);
		fwrite(sample_offset, 1, offset_size, in_file);
		sample_offset[0] = gain >> 8;
		sample_offset[1] = gain & 0xFF;
		sample_offset[2] = peak >> 8;
		sample_offset[3] = peak & 0xFF;
		fwrite(sample_offset, 1, 4, in_file);
		FinalizeTags(in_file, TAG_VERSION, TAG_NO_FOOTER | TAG_NO_PREAMBLE);
	}

	fwrite(tmp_buff, 1, stbuf.st_size - chap_pos - chap_size, in_file);
	ftruncate(fileno(in_file), ftell(in_file));

	fclose(in_file);
	free(tmp_buff);
	iniparser_freedict(dict);

	return MPC_STATUS_OK;
}

mpc_status add_chaps_cue(char * mpc_file, char * chap_file, mpc_demux * demux, mpc_streaminfo * si)
{
	Cd *cd = 0;
	int nchap, format = UNKNOWN;
	struct stat stbuf;
	FILE * in_file;
	int chap_pos, end_pos, chap_size, i;
	char * tmp_buff;

	if (0 == (cd = cf_parse(chap_file, &format))) {
		fprintf(stderr, "%s: input file error\n", chap_file);
		return !MPC_STATUS_OK;
	}

	chap_pos = (demux->chap_pos >> 3) + si->header_position;
	end_pos = mpc_demux_pos(demux) >> 3;
	chap_size = end_pos - chap_pos;

	stat(mpc_file, &stbuf);
	tmp_buff = malloc(stbuf.st_size - chap_pos - chap_size);
	in_file = fopen( mpc_file, "r+b" );
	fseek(in_file, chap_pos + chap_size, SEEK_SET);
	fread(tmp_buff, 1, stbuf.st_size - chap_pos - chap_size, in_file);
	fseek(in_file, chap_pos, SEEK_SET);

	nchap = cd_get_ntrack(cd);
	for (i = 1; i <= nchap; i++) {
		char track_buf[16], block_header[12] = "CT", sample_offset[10];
		int j, nitem = 0, tag_len = 0, key_len, item_len, offset_size;
		Track * track;
		Cdtext *cdtext;
		mpc_int64_t chap_pos;

		track = cd_get_track (cd, i);
		cdtext = track_get_cdtext(track);

		// position du chapitre
		chap_pos = (mpc_int64_t) si->sample_freq * track_get_start (track) / 75;

		if (chap_pos > si->samples - si->beg_silence)
			fprintf(stderr, "warning : chapter %i starts @ %lli samples after the end of the stream (%lli)\n",
			        i, chap_pos, si->samples - si->beg_silence);

		Init_Tags();

		sprintf(track_buf, "%i/%i", i, nchap);
		key_len = 5;
		item_len = strlen(track_buf);
		addtag("Track", key_len, track_buf, item_len, 0, 0);
		tag_len += key_len + item_len;
		nitem++;

		for (j = 0; j < (sizeof(Ptis) / sizeof(*Ptis)); j++) {
			char const * item_key = APE_keys[j], * item_value;
			item_value = cdtext_get (Ptis[j], cdtext);
			if (item_value != 0) {
				key_len = strlen(item_key);
				item_len = strlen(item_value);
				addtag(item_key, key_len, item_value, item_len, 0, 0);
				tag_len += key_len + item_len;
				nitem++;
			}
		}

		tag_len += 24 + nitem * 9;
		offset_size = encodeSize(chap_pos, sample_offset, MPC_FALSE);
		tag_len = encodeSize(tag_len + 4 + offset_size + 2, block_header + 2, MPC_TRUE);
		fwrite(block_header, 1, tag_len + 2, in_file);
		fwrite(sample_offset, 1, offset_size, in_file);
		fwrite("\0\0\0\0", 1, 4, in_file); // put unknow chapter gain / peak
		FinalizeTags(in_file, TAG_VERSION, TAG_NO_FOOTER | TAG_NO_PREAMBLE);
	}

	fwrite(tmp_buff, 1, stbuf.st_size - chap_pos - chap_size, in_file);
	ftruncate(fileno(in_file), ftell(in_file));

	fclose(in_file);
	free(tmp_buff);

	return MPC_STATUS_OK;
}

mpc_status dump_chaps(mpc_demux * demux, char * chap_file, int chap_nb)
{
	int i;
	FILE * out_file;
	mpc_chap_info const * chap;

	if (chap_nb <= 0)
		return MPC_STATUS_OK;

	out_file = fopen(chap_file, "wb");
	if (out_file == 0)
		return !MPC_STATUS_OK;

	for (i = 0; i < chap_nb; i++) {
		chap = mpc_demux_chap(demux, i);
		fprintf(out_file, "[%lli]\ngain=%i\npeak=%i\n", chap->sample, chap->gain, chap->peak);
		if (chap->tag_size > 0) {
			int item_count, j;
			char const * tag = chap->tag;
			item_count = tag[8] | (tag[9] << 8) | (tag[10] << 16) | (tag[11] << 24);
			tag += 24;
			for( j = 0; j < item_count; j++){
				int key_len = strlen(tag + 8);
				int value_len = tag[0] | (tag[1] << 8) | (tag[2] << 16) | (tag[3] << 24);
				fprintf(out_file, "%s=\"%.*s\"\n", tag + 8, value_len, tag + 9 + key_len);
				tag += 9 + key_len + value_len;
			}
		}
		fprintf(out_file, "\n");
	}

	fclose(out_file);

	return MPC_STATUS_OK;
}

int main(int argc, char **argv)
{
	mpc_reader reader;
	mpc_demux* demux;
	mpc_streaminfo si;
	char * mpc_file, * chap_file;
	mpc_status err;
	FILE * test_file;
	int chap_nb;

	fprintf(stderr, About, argv[0]);

	if (argc != 3)
		usage(argv[0]);

	mpc_file = argv[1];
	chap_file = argv[2];

	err = mpc_reader_init_stdio(&reader, mpc_file);
	if(err < 0) return !MPC_STATUS_OK;

	demux = mpc_demux_init(&reader);
	if(!demux) return !MPC_STATUS_OK;
	mpc_demux_get_info(demux,  &si);

	if (si.stream_version < 8) {
		fprintf(stderr, "this file cannot be edited, please convert it first to sv8 using mpc2sv8\n");
		exit(!MPC_STATUS_OK);
	}

	chap_nb = mpc_demux_chap_nb(demux);

	test_file = fopen(chap_file, "rb" );
	if (test_file == 0) {
		err = dump_chaps(demux, chap_file, chap_nb);
	} else {
		int len;
		fclose(test_file);
		len = strlen(chap_file);
		if (strcasecmp(chap_file + len - 4, ".cue") == 0 || strcasecmp(chap_file + len - 4, ".toc") == 0)
			err = add_chaps_cue(mpc_file, chap_file, demux, &si);
		else if (strcasecmp(chap_file + len - 4, ".ini") == 0)
			err = add_chaps_ini(mpc_file, chap_file, demux, &si);
		else
			err = !MPC_STATUS_OK;
	}

	mpc_demux_exit(demux);
	mpc_reader_exit_stdio(&reader);
	return err;
}
