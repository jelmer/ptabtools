/*
   Parser library for the Guitar Pro 3 file format
   (c) 2004: Jelmer Vernooij <jelmer@samba.org>

   Based on gp3_loader.py from songwrite
   	Copyright (C) 2003 Bertrand LAMY
	Copyright (C) 2003 Jean-Baptiste LAMY

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "gp.h"
#include <glib.h>

static void gp_read(struct gpf *gpf, void *data, size_t len)
{
	g_assert(read(gpf->fd, data, len) == len);
}

static void gp_read_unknown(struct gpf *gpf, size_t num)
{
	char *tmp = g_new0(char, num);
	gp_read(gpf, tmp, num);
	g_free(tmp);
}

static void gp_read_string(struct gpf *gpf, const char **dest)
{
	unsigned char len = 0;
	char *ret;
	gp_read(gpf, &len, 1);
	ret = g_new0(char, len+1);
	gp_read(gpf, ret, len);
	*dest = ret;
	printf("Read short string : %s\n", ret);
}

static void gp_read_uint8(struct gpf *gpf, guint8 *n)
{
	gp_read(gpf, n, sizeof(guint8));
}

static void gp_read_uint32(struct gpf *gpf, guint32 *n)
{
	gp_read(gpf, n, sizeof(guint32));
}

static void gp_read_long_string(struct gpf *gpf, const char **dest)
{
	guint32 l;
	char *ret;
	gp_read_uint32(gpf, &l);
	ret = g_new0(char, l + 1);
	gp_read(gpf, ret, l);
	*dest = ret;
	printf("Read long string : %s\n", ret);
}

static void gp_read_color(struct gpf *gpf, struct gp_color *color)
{
	gp_read_uint8(gpf, &color->unknown);
	gp_read_uint8(gpf, &color->red);
	gp_read_uint8(gpf, &color->green);
	gp_read_uint8(gpf, &color->blue);
}

static void gp_read_nstring(struct gpf *gpf, const char **dest, size_t len)
{
	char *ret = g_new0(char, len + 1);
	gp_read(gpf, ret, len + 1);
	*dest = ret;
	printf("Read n string : %s\n", ret);
}

static void gp_read_header(struct gpf *gpf)
{
	if (gpf->version >= 3.0) {
		guint32 i;
		gp_read_long_string(gpf, &gpf->title);
		gp_read_long_string(gpf, &gpf->subtitle);
		gp_read_long_string(gpf, &gpf->artist);
		gp_read_long_string(gpf, &gpf->album);
		gp_read_long_string(gpf, &gpf->author);
		gp_read_long_string(gpf, &gpf->copyright);
		gp_read_long_string(gpf, &gpf->tab_by);
		gp_read_long_string(gpf, &gpf->instruction);

		gp_read_uint32(gpf, &gpf->notice_num_lines);
		gpf->notice = g_new0(const char *, gpf->notice_num_lines);
		for (i = 0; i < gpf->notice_num_lines; i++) 
		{
			gp_read_long_string(gpf, &gpf->notice[i]);
		}
		gp_read_uint8(gpf, &gpf->shuffle);
	} else {
		if (gpf->version >= 2.0) {
			gp_read_unknown(gpf, 1);
		}
		gp_read_nstring(gpf, &gpf->title, 100);
		gp_read_unknown(gpf, 1);
		gp_read_nstring(gpf, &gpf->author, 50);
		gp_read_unknown(gpf, 1);
		gp_read_nstring(gpf, &gpf->instruction, 100);
	}
}

static void gp_read_lyrics(struct gpf *gpf)
{
	if (gpf->version >= 4.0) 
	{
		guint32 i;
		gp_read_uint32(gpf, &gpf->lyrics_track);

		gpf->num_lyrics = 5;

		gpf->lyrics = g_new0(struct gp_lyric, gpf->num_lyrics);
		
		for (i = 0; i < gpf->num_lyrics; i++) {
			gp_read_uint32(gpf, &gpf->lyrics[i].bar);
			gp_read_long_string(gpf, &gpf->lyrics[i].data);
		}
	}
}

static void gp_read_instruments(struct gpf *gpf)
{
	if (gpf->version >= 3.0) {
		guint32 i;
		gpf->num_instruments = 64; 
		gpf->instrument = g_new(struct gp_instrument, gpf->num_instruments);
		for (i = 0; i < gpf->num_instruments; i++) 
		{
			gp_read_unknown(gpf, 12);
		}
	} else {
		guint32 i;
		for (i = 0; i < 8; i++) 
		{
			guint32 x;
			gp_read_uint32(gpf, &x);
			gp_read_unknown(gpf, x * 4);
		}
	}
}

static void gp_read_bars(struct gpf *gpf)
{
	guint32 i;
	gpf->bars = g_new0(struct gp_bar, gpf->num_bars);

	for (i = 0; i < gpf->num_bars; i++) 
	{
		gp_read_uint32(gpf, &gpf->bars[i].properties);

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_CUSTOM_RHYTHM_1) {
			gp_read_uint8(gpf, &gpf->bars[i].rhythm_1);
		} else {
			gpf->bars[i].rhythm_1 = 4;
		}

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_CUSTOM_RHYTHM_2) {
			gp_read_uint8(gpf, &gpf->bars[i].rhythm_2);
		} else {
			gpf->bars[i].rhythm_2 = 4;
		}

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_REPEAT_CLOSE) {
			gp_read_uint8(gpf, &gpf->bars[i].repeat_close.volta);
		}

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_ALT_ENDING) { 
			gp_read_uint8(gpf, &gpf->bars[i].alternate_ending.type);
		}

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_MARKER) {
			gp_read_long_string(gpf, &gpf->bars[i].marker.name);
			gp_read_color(gpf, &gpf->bars[i].marker.color);
		}

		if (gpf->bars[i].properties & GP_BAR_PROPERTY_CHANGE_ARMOR) {
			gp_read_uint8(gpf, &gpf->bars[i].change_armor.armor_jumps);
			gp_read_uint8(gpf, &gpf->bars[i].change_armor.minor);
		}
	}
}

static void gp_read_tracks(struct gpf *gpf)
{
	guint32 i;

	gpf->tracks = g_new0(struct gp_track, gpf->num_tracks);

	if (gpf->version >= 3.0) 
	{
		for (i = 0; i < gpf->num_tracks; i++) 
		{
			guint32 j;
			gp_read_uint8(gpf, &gpf->tracks[i].spc);
			gp_read_nstring(gpf, &gpf->tracks[i].name, 40);
			gp_read_uint8(gpf, &gpf->tracks[i].num_strings);
			gpf->tracks[i].strings = g_new0(struct gp_track_string, gpf->tracks[i].num_strings);
			for (j = 0; j < 7; j++) {
				guint32 string_pitch;
				gp_read_uint32(gpf, &string_pitch);
				if (i < gpf->tracks[i].num_strings) {
					gpf->tracks[i].strings[j].pitch = string_pitch;
				}
			}
			gp_read_uint32(gpf, &gpf->tracks[i].midi_port);
			gp_read_uint32(gpf, &gpf->tracks[i].channel1);
			gp_read_uint32(gpf, &gpf->tracks[i].channel2);
			gp_read_uint32(gpf, &gpf->tracks[i].num_frets);
			gp_read_uint32(gpf, &gpf->tracks[i].capo);
			gp_read_color(gpf, &gpf->tracks[i].color);
		} 
	} else {
		int i;
		for (i = 0; i < 8; i++) 
		{
			gp_read_unknown(gpf, 4);
			gp_read_uint32(gpf, &gpf->tracks[i].num_frets);
			gp_read_unknown(gpf, 1);
			gp_read_nstring(gpf, &gpf->tracks[i].name, 40);
			gp_read_unknown(gpf, 1 + 5 * 4);
		}
		gpf->num_tracks = 0;
	}
}

static void gp_read_beat(struct gpf *gpf, struct gp_beat *beat)
{
	gp_read_uint8(gpf, &beat->properties);
	gp_read_uint8(gpf, &beat->duration);

	if (beat->properties & GP_BEAT_PROPERTY_TUPLET) {
		gp_read_uint32(gpf, &beat->n_tuplet);
	}

	if (beat->properties & GP_BEAT_PROPERTY_CHORD) {
		gp_read_uint8(gpf, &beat->chord.complete);
		if (!beat->chord.complete) {
			gp_read_long_string(gpf, &beat->chord.name);
		} else {
			if (gpf->version >= 4.0) {
				gp_read_unknown(gpf, 16);
				gp_read_string(gpf, &beat->chord.name);
				gp_read_unknown(gpf, 25);
			} else {
				gp_read_unknown(gpf, 25);
				gp_read_string(gpf, &beat->chord.name);
				gp_read_unknown(gpf, 34);	
			}
		}
		gp_read_uint32(gpf, &beat->chord.top_fret);
		if (beat->chord.top_fret == 0) 
		{
			if (beat->chord.complete) {
				gp_read_unknown(gpf, 6 * 4);
			} else {
				gp_read_unknown(gpf, 7 * 4);
			}
		} 
		if (beat->chord.complete) 
		{
			if (gpf->version >= 4.0) {
                gp_read_unknown(gpf, 24 + 7 + 1);
			} else {
				gp_read_unknown(gpf, 32);
			}
		}
	}

	if (beat->properties & GP_BEAT_PROPERTY_EFFECT) 
	{
		gp_read_uint8(gpf, &beat->effect.properties1);

		if (gpf->version >= 4.0) {
			gp_read_uint8(gpf, &beat->effect.properties2);
		}
	}
}

static void gp_read_data(struct gpf *gpf)
{
	guint32 i, j;
	for (i = 0; i < gpf->num_bars; i++) 
	{
		gpf->beats = g_new0(struct gp_beat, 1);
		for (j = 0; j < gpf->num_tracks; j++) 
		{
			/* FIXME */
		}
	}
}

struct gpf *gp_read_file(const char *filename)
{
	struct gpf *gpf = g_new0(struct gpf, 1);
	gpf->fd = open(filename, O_RDONLY);

	if (gpf->fd < 0) {
		return NULL;
	}

	gp_read_string(gpf, &gpf->version_string);
	gpf->version = 4.06; /* FIXME */

	gp_read_unknown(gpf, 6);

	gp_read_header(gpf);

	gp_read_lyrics(gpf);

	gp_read_uint32(gpf, &gpf->bpm);

	if (gpf->version >= 4.0) {
		gp_read_unknown(gpf, 5);
	} else if(gpf->version >= 3.0) {
		gp_read_unknown(gpf, 4);
	} else {
		gp_read_unknown(gpf, 8);
	}

	gp_read_instruments(gpf);

	gp_read_uint32(gpf, &gpf->num_bars);

	if (gpf->version >= 3.0) 
	{
		gp_read_uint32(gpf, &gpf->num_tracks);
	} else {
		gpf->num_tracks = 8;
	}

	gp_read_bars(gpf);

	gp_read_tracks(gpf);

	gp_read_data(gpf);

	return gpf;
}
	
void main()
{
	gp_read_file("bla.gp4");
}
