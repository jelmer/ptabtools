/*
   (c) 2004 Jelmer Vernooij <jelmer@samba.org>

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

#ifndef __GP_H__
#define __GP_H__

#include <sys/stat.h>
#include <glib.h>
#include <stdlib.h>

#if defined(_WIN32) && !defined(PTB_CORE)
#pragma comment(lib,"ptb.lib")
#endif

struct gp_color {
	guint8 unknown;
	guint8 red;
	guint8 green;
	guint8 blue;
};

struct gpf {
	int fd;
	const char *version_string;
	double version;
	
	const char *title;
	const char *artist;
	const char *album;
	const char *subtitle;
	const char *tab_by;
	const char *instruction;
	const char *author;
	const char *copyright;
	guint32 notice_num_lines;
	const char **notice;

	guint8 shuffle;

	guint32 lyrics_track;

	guint32 num_lyrics;
	struct gp_lyric {
		guint32 bar;
		const char *data;
	} *lyrics;

	guint32 bpm;

	guint32 num_instruments;
	struct gp_instrument {
	} *instrument;

	guint32 num_bars;
	guint32 num_tracks;

	struct gp_bar {
		guint8 properties;
#define GP_BAR_PROPERTY_CUSTOM_RHYTHM_1 0x01
#define GP_BAR_PROPERTY_CUSTOM_RHYTHM_2	0x02
#define GP_BAR_PROPERTY_REPEAT_OPEN		0x04
#define GP_BAR_PROPERTY_REPEAT_CLOSE	0x08
#define GP_BAR_PROPERTY_ALT_ENDING		0x10
#define GP_BAR_PROPERTY_MARKER			0x20
#define GP_BAR_PROPERTY_CHANGE_ARMOR	0x40
#define GP_BAR_PROPERTY_DOUBLE_ENDING	0x80
		
		guint8 rhythm_1;
		guint8 rhythm_2;
		struct {
			guint8 volta;
		} repeat_close;
		struct {
			guint8 type;
		} alternate_ending;
		struct {
			const char *name;
			struct gp_color color;
		} marker;
		struct {
			guint8 armor_jumps;
			guint8 minor;
		} change_armor;
		struct gp_bar_track
		{
			guint32 num_beats;
			struct gp_beat
			{
				guint8 properties;
#define GP_BEAT_PROPERTY_DOTTED	0x01
#define GP_BEAT_PROPERTY_CHORD	0x02
#define GP_BEAT_PROPERTY_TEXT	0x04
#define GP_BEAT_PROPERTY_EFFECT	0x08
#define GP_BEAT_PROPERTY_CHANGE	0x10
#define GP_BEAT_PROPERTY_TUPLET	0x20
#define GP_BEAT_PROPERTY_REST	0x40
				struct { 
					guint8 complete;
					const char *name;
					guint32 top_fret;
				} chord;
				struct {
#define GP_BEAT_EFFECT1_VIBRATO				0x01
#define GP_BEAT_EFFECT1_WIDE_VIBRATO		0x02
#define GP_BEAT_EFFECT1_NATURAL_HARMONIC	0x04
#define GP_BEAT_EFFECT1_OTHER_HARMONIC		0x08
#define GP_BEAT_EFFECT1_FADE_IN				0x10
#define GP_BEAT_EFFECT1_4_STROCKE_EFFECT	0x20
#define GP_BEAT_EFFECT1_TREMOLO_BAR			0x20
#define GP_BEAT_EFFECT1_STROCKE				0x40
#define GP_BEAT_EFFECT2_RASGUEADO			0x01
#define GP_BEAT_EFFECT2_PICK_STROCKE		0x02
#define GP_BEAT_EFFECT2_TREMOLO_BAR			0x04
					guint8 properties1;
					guint8 properties2;
					struct {
						guint32 num_points;
					} tremolo_bar;
				} effect;
				struct {
					guint8 new_instrument;
					guint8 new_volume;
					guint8 new_reverb;
					guint8 new_pan;
					guint8 new_chorus;
					guint8 new_phaser;
					guint32 new_tempo;
					guint8 new_tremolo;
				} change;
				struct {
					guint32 n_tuplet;
				} tuplet;
				guint8 duration;
				const char *text;
				guint8 strings_present;
				struct gp_note {
					guint8 duration;
					guint8 new_nuance;
					guint8 value;
					guint8 properties;
#define GP_NOTE_PROPERTY_DURATION_SPECIAL	0x01
#define GP_NOTE_PROPERTY_DOTTED				0x02
#define GP_NOTE_PROPERTY_GHOST				0x04
#define GP_NOTE_PROPERTY_EFFECT				0x08
#define GP_NOTE_PROPERTY_NUANCE_CHANGE		0x10
#define GP_NOTE_PROPERTY_ALTERATION			0x20
#define GP_NOTE_PROPERTY_ACCENTUATED		0x40
#define GP_NOTE_PROPERTY_FINGERING			0x80
					guint8 alteration;
#define GP_NOTE_ALTERATION_LINKED			0x02
#define GP_NOTE_ALTERATION_DEAD				0x03
					struct {
						guint8 properties1;
#define GP_NOTE_EFFECT1_BEND				0x01
#define GP_NOTE_EFFECT1_HAMMER				0x02
#define GP_NOTE_EFFECT1_SLIDE				0x04
#define GP_NOTE_EFFECT1_LET_RING			0x08
#define GP_NOTE_EFFECT1_APPOGIATURE			0x10
						guint8 properties2;
#define GP_NOTE_EFFECT2_STACCATO			0x01
#define GP_NOTE_EFFECT2_PALM_MUTE			0x02
#define GP_NOTE_EFFECT2_TREMOLO_PICKING		0x04
#define GP_NOTE_EFFECT2_SLIDE				0x08
#define GP_NOTE_EFFECT2_HARMONIC			0x10
#define GP_NOTE_EFFECT2_TRILL				0x20
#define GP_NOTE_EFFECT2_VIBRATO				0x40
						struct {
							guint32 num_points;
							struct gp_note_effect_bend_point 
							{
								guint32 pitch;
							} *points;
						} bend;

						struct {
							guint8 duration;
						} tremolo_picking;

						struct {
							guint8 type;
						} slide;

						struct {
							guint8 type;
						} harmonic;

						struct {
							guint8 note_value;
							guint8 frequency;
						} trill;

						struct {
							guint8 duration;
							guint8 previous_note;
							guint8 transition;
						} appogiature;
					} effect;
					struct {
						guint8 left_hand;
						guint8 right_hand;
					} fingering;
				} notes[7];
			} *beats;
		} *tracks;
	} *bars;

	struct gp_track {
		const char *name;
		guint8 spc;
		guint32 num_frets;
		guint32 num_strings;
		guint32 midi_port;
		guint32 channel1;
		guint32 channel2;
		struct gp_track_string {
			guint32 pitch;
		} *strings;
		struct gp_color color;
		guint32 capo;
	} *tracks;

};

extern struct gpf *gp_read_file(const char *filename);

#endif /* __GP_H__ */
