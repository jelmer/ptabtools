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
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef _WIN32
#  include <io.h>
#endif

#define PTB_CORE
#include "gp.h"

#define malloc_p(t, n) (t *) calloc(sizeof(t), n)

static void gp_read(struct gpf *gpf, void *data, size_t len)
{
	int ret = read(gpf->fd, data, len);
	assert(ret == len);
}

static void gp_read_unknown(struct gpf *gpf, size_t num)
{
	char *tmp = malloc_p(char, num);
	gp_read(gpf, tmp, num);
	free(tmp);
}

static void gp_read_string(struct gpf *gpf, const char **dest)
{
	unsigned char len = 0;
	char *ret;
	gp_read(gpf, &len, 1);
	ret = malloc_p(char, len+1);
	gp_read(gpf, ret, len);
	*dest = ret;
}

static void gp_read_uint8(struct gpf *gpf, uint8_t *n)
{
	gp_read(gpf, n, sizeof(uint8_t));
}

static void gp_read_uint32(struct gpf *gpf, uint32_t *n)
{
	gp_read(gpf, n, sizeof(uint32_t));
}

static void gp_read_long_string(struct gpf *gpf, const char **dest)
{
	uint32_t l;
	char *ret;
	gp_read_uint32(gpf, &l);
	ret = malloc_p(char, l + 1);
	gp_read(gpf, ret, l);
	*dest = ret;
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
	uint8_t _len;
	char *ret = malloc_p(char, len + 1);
	gp_read_uint8(gpf, &_len);
	assert(_len <= len);
	gp_read(gpf, ret, len);
	ret[_len] = '\0';
	*dest = ret;
}

static void gp_read_header(struct gpf *gpf)
{
	if (gpf->version >= 3.0) {
		uint32_t i;
		gp_read_long_string(gpf, &gpf->title);
		gp_read_long_string(gpf, &gpf->subtitle);
		gp_read_long_string(gpf, &gpf->artist);
		gp_read_long_string(gpf, &gpf->album);
		gp_read_long_string(gpf, &gpf->author);
		gp_read_long_string(gpf, &gpf->copyright);
		gp_read_long_string(gpf, &gpf->tab_by);
		gp_read_long_string(gpf, &gpf->instruction);

		gp_read_uint32(gpf, &gpf->notice_num_lines);
		gpf->notice = malloc_p(const char *, gpf->notice_num_lines);
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
		uint32_t i;
		gp_read_uint32(gpf, &gpf->lyrics_track);

		gpf->num_lyrics = 5;

		gpf->lyrics = malloc_p(struct gp_lyric, gpf->num_lyrics);
		
		for (i = 0; i < gpf->num_lyrics; i++) {
			gp_read_uint32(gpf, &gpf->lyrics[i].bar);
			gp_read_long_string(gpf, &gpf->lyrics[i].data);
		}
	}
}

static void gp_read_instruments(struct gpf *gpf)
{
	if (gpf->version >= 3.0) {
		uint32_t i;
		gpf->num_instruments = 64; 
		gpf->instrument = malloc_p(struct gp_instrument, gpf->num_instruments);
		for (i = 0; i < gpf->num_instruments; i++) 
		{
			gp_read_unknown(gpf, 12);
		}
	} else {
		uint32_t i;
		for (i = 0; i < 8; i++) 
		{
			uint32_t x;
			gp_read_uint32(gpf, &x);
			gp_read_unknown(gpf, x * 4);
		}
	}
}

static void gp_read_bars(struct gpf *gpf)
{
	uint32_t i;
	gpf->bars = malloc_p(struct gp_bar, gpf->num_bars);

	for (i = 0; i < gpf->num_bars; i++) 
	{
		gp_read_uint8(gpf, &gpf->bars[i].properties);

		assert((	gpf->bars[i].properties 
					&~ GP_BAR_PROPERTY_CUSTOM_RHYTHM_1 
					&~ GP_BAR_PROPERTY_CUSTOM_RHYTHM_2
					&~ GP_BAR_PROPERTY_REPEAT_OPEN
					&~ GP_BAR_PROPERTY_REPEAT_CLOSE
					&~ GP_BAR_PROPERTY_ALT_ENDING
					&~ GP_BAR_PROPERTY_MARKER
					&~ GP_BAR_PROPERTY_CHANGE_ARMOR
					&~ GP_BAR_PROPERTY_DOUBLE_ENDING) == 0);

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
	uint32_t i;

	gpf->tracks = malloc_p(struct gp_track, gpf->num_tracks);

	if (gpf->version >= 3.0) 
	{
		for (i = 0; i < gpf->num_tracks; i++) 
		{
			uint32_t j;
			gp_read_uint8(gpf, &gpf->tracks[i].spc);
			gp_read_nstring(gpf, &gpf->tracks[i].name, 40);
			gp_read_uint32(gpf, &gpf->tracks[i].num_strings);
			gpf->tracks[i].strings = malloc_p(struct gp_track_string, gpf->tracks[i].num_strings);
			for (j = 0; j < 7; j++) {
				uint32_t string_pitch;
				gp_read_uint32(gpf, &string_pitch);
				if (j < gpf->tracks[i].num_strings) {
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
	int i;
	gp_read_uint8(gpf, &beat->properties);
	assert((beat->properties 
			 &~ GP_BEAT_PROPERTY_DOTTED	
			 &~ GP_BEAT_PROPERTY_CHORD
			 &~ GP_BEAT_PROPERTY_TEXT
			 &~ GP_BEAT_PROPERTY_EFFECT
			 &~ GP_BEAT_PROPERTY_CHANGE
			 &~ GP_BEAT_PROPERTY_TUPLET
			 &~ GP_BEAT_PROPERTY_REST) == 0);

	if (beat->properties & GP_BEAT_PROPERTY_REST) {
		gp_read_unknown(gpf, 1);
	}

	gp_read_uint8(gpf, &beat->duration);

	if (beat->properties & GP_BEAT_PROPERTY_TUPLET) {
		gp_read_uint32(gpf, &beat->tuplet.n_tuplet);
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
		} else {
			beat->effect.properties2 = 0;
		}

		if (beat->effect.properties1 & GP_BEAT_EFFECT1_STROCKE) {
			gp_read_unknown(gpf, 2);
		}

		if (beat->effect.properties2 & GP_BEAT_EFFECT2_PICK_STROCKE) {
			gp_read_unknown(gpf, 1);
		}

		if (beat->effect.properties2 & GP_BEAT_EFFECT2_TREMOLO_BAR) {
			gp_read_unknown(gpf, 5);
			gp_read_uint32(gpf, &beat->effect.tremolo_bar.num_points);
			gp_read_unknown(gpf, beat->effect.tremolo_bar.num_points * 9);
		}

		if (gpf->version >= 4.0) {
			if (beat->effect.properties1 & GP_BEAT_EFFECT1_4_STROCKE_EFFECT) {
				gp_read_unknown(gpf, 1);
			}
		} else {
			if (beat->effect.properties1 & GP_BEAT_EFFECT1_TREMOLO_BAR) {
				gp_read_unknown(gpf, 5);
			}
		}
	}

	if (beat->properties & GP_BEAT_PROPERTY_CHANGE) 
	{
		gp_read_uint8(gpf, &beat->change.new_instrument);
		gp_read_uint8(gpf, &beat->change.new_volume);
		gp_read_uint8(gpf, &beat->change.new_pan);
		gp_read_uint8(gpf, &beat->change.new_chorus);
		gp_read_uint8(gpf, &beat->change.new_reverb);
		gp_read_uint8(gpf, &beat->change.new_phaser);
		gp_read_uint8(gpf, &beat->change.new_tremolo);
		gp_read_uint32(gpf, &beat->change.new_tempo);
		if (beat->change.new_volume != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_pan != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_chorus != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_reverb != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_phaser != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_tremolo != 0xFF) gp_read_unknown(gpf, 1);
		if (beat->change.new_tempo != -1) gp_read_unknown(gpf, 1);

		if (gpf->version >= 4.0) {
			gp_read_unknown(gpf, 1);
		}
	}

	gp_read_uint8(gpf, &beat->strings_present);

	for (i = 0; i < 7; i++)
	{
		struct gp_note *n = &beat->notes[i];

		if (!(beat->strings_present & (1 << i))) continue;

		gp_read_uint8(gpf, &n->properties);

		if (n->properties & GP_NOTE_PROPERTY_ALTERATION) {
			gp_read_uint8(gpf, &n->alteration);
		} else {
			n->alteration = 0;
		}

		if (n->properties & GP_NOTE_PROPERTY_DURATION_SPECIAL) {
			gp_read_uint8(gpf, &n->duration);
			gp_read_unknown(gpf, 1);
		} else {
			n->duration = beat->duration;
		}

		if (n->properties & GP_NOTE_PROPERTY_NUANCE_CHANGE) {
			gp_read_uint8(gpf, &n->new_nuance);
		}

		if (n->properties & GP_NOTE_PROPERTY_ALTERATION) {
			gp_read_uint8(gpf, &n->value);
		}

		if (n->properties & GP_NOTE_PROPERTY_FINGERING) {
			gp_read_uint8(gpf, &n->fingering.left_hand);
			gp_read_uint8(gpf, &n->fingering.right_hand);
		}

		if (n->properties & GP_NOTE_PROPERTY_EFFECT) {
			gp_read_uint8(gpf, &n->effect.properties1);
			
			if (gpf->version >= 4.0) {
				gp_read_uint8(gpf, &n->effect.properties2);
			} else {
				n->effect.properties2 = 0;
			}

			if (n->effect.properties1 & GP_NOTE_EFFECT1_BEND) {
				int k;
				gp_read_unknown(gpf, 5);
				gp_read_uint32(gpf, &n->effect.bend.num_points);
				n->effect.bend.points = malloc_p(struct gp_note_effect_bend_point, n->effect.bend.num_points);
				for (k = 0; k < n->effect.bend.num_points; k++)
				{
					gp_read_unknown(gpf, 4);
					gp_read_uint32(gpf, &n->effect.bend.points[k].pitch);
					gp_read_unknown(gpf, 1);
				}
			}

			if (n->effect.properties1 & GP_NOTE_EFFECT1_APPOGIATURE) 
			{
				gp_read_uint8(gpf, &n->effect.appogiature.previous_note);
				gp_read_unknown(gpf, 1);
				gp_read_uint8(gpf, &n->effect.appogiature.transition);
				gp_read_uint8(gpf, &n->effect.appogiature.duration);
			}

			if (n->effect.properties2 & GP_NOTE_EFFECT2_TREMOLO_PICKING)
			{
				gp_read_uint8(gpf, &n->effect.tremolo_picking.duration);
			}

			if (n->effect.properties2 & GP_NOTE_EFFECT2_SLIDE)
			{
				gp_read_uint8(gpf, &n->effect.slide.type);
			}

			if (n->effect.properties2 & GP_NOTE_EFFECT2_HARMONIC)
			{
				gp_read_uint8(gpf, &n->effect.harmonic.type);
			}

			if (n->effect.properties2 & GP_NOTE_EFFECT2_TRILL)
			{
				gp_read_uint8(gpf, &n->effect.trill.note_value);
				gp_read_uint8(gpf, &n->effect.trill.frequency);
			}
		}
	}
}

static void gp_read_data(struct gpf *gpf)
{
	uint32_t i, j, k;
	for (i = 0; i < gpf->num_bars; i++) 
	{
		gpf->bars[i].tracks = malloc_p(struct gp_bar_track, gpf->num_tracks);
		for (j = 0; j < gpf->num_tracks; j++) 
		{
			gp_read_uint32(gpf, &gpf->bars[i].tracks[j].num_beats);
			gpf->bars[i].tracks[j].beats = malloc_p(struct gp_beat, gpf->bars[i].tracks[j].num_beats);
			for (k = 0; k < gpf->bars[i].tracks[j].num_beats; k++) 
			{
				gp_read_beat(gpf, &gpf->bars[i].tracks[j].beats[k]);
			}
		}
	}
}

static double find_version(const char *name)
{
	int i;
	for(i = strlen(name)-1; isdigit(name[i]) || name[i] == '.'; i--);

	return atof(name+i+1);
}

struct gpf *gp_read_file(const char *filename)
{
	
	struct gpf *gpf = malloc_p(struct gpf, 1);
	gpf->fd = open(filename, O_RDONLY);

	if (gpf->fd < 0) {
		return NULL;
	}

	gp_read_string(gpf, &gpf->version_string);
	gpf->version = find_version(gpf->version_string);

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

	gp_read_unknown(gpf, 2);

	return gpf;
}
