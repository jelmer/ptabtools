/*
   Parsing utilities for GuitarPro (version 2, 3 and 4) files
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
#include <stdlib.h>

#if defined(_WIN32) && !defined(PTB_CORE)
#pragma comment(lib,"ptb.lib")
#endif

#ifdef _WIN32
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

struct gp_color {
	uint8_t unknown;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
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
	uint32_t notice_num_lines;
	const char **notice;

	uint8_t shuffle;

	uint32_t lyrics_track;

	uint32_t num_lyrics;
	struct gp_lyric {
		uint32_t bar;
		const char *data;
	} *lyrics;

	uint32_t bpm;

	uint32_t num_instruments;
	struct gp_instrument {
		char dummy;
	} *instrument;

	uint32_t num_bars;
	uint32_t num_tracks;

	struct gp_bar {
		uint8_t properties;
#define GP_BAR_PROPERTY_CUSTOM_RHYTHM_1 0x01
#define GP_BAR_PROPERTY_CUSTOM_RHYTHM_2	0x02
#define GP_BAR_PROPERTY_REPEAT_OPEN		0x04
#define GP_BAR_PROPERTY_REPEAT_CLOSE	0x08
#define GP_BAR_PROPERTY_ALT_ENDING		0x10
#define GP_BAR_PROPERTY_MARKER			0x20
#define GP_BAR_PROPERTY_CHANGE_ARMOR	0x40
#define GP_BAR_PROPERTY_DOUBLE_ENDING	0x80
		
		uint8_t rhythm_1;
		uint8_t rhythm_2;
		struct {
			uint8_t volta;
		} repeat_close;
		struct {
			uint8_t type;
		} alternate_ending;
		struct {
			const char *name;
			struct gp_color color;
		} marker;
		struct {
			uint8_t armor_jumps;
			uint8_t minor;
		} change_armor;
		struct gp_bar_track
		{
			uint32_t num_beats;
			struct gp_beat
			{
				uint8_t properties;
#define GP_BEAT_PROPERTY_DOTTED	0x01
#define GP_BEAT_PROPERTY_CHORD	0x02
#define GP_BEAT_PROPERTY_TEXT	0x04
#define GP_BEAT_PROPERTY_EFFECT	0x08
#define GP_BEAT_PROPERTY_CHANGE	0x10
#define GP_BEAT_PROPERTY_TUPLET	0x20
#define GP_BEAT_PROPERTY_REST	0x40
				struct { 
					uint8_t complete;
					const char *name;
					uint32_t top_fret;
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
					uint8_t properties1;
					uint8_t properties2;
					struct {
						uint32_t num_points;
					} tremolo_bar;
				} effect;
				struct {
					uint8_t new_instrument;
					uint8_t new_volume;
					uint8_t new_reverb;
					uint8_t new_pan;
					uint8_t new_chorus;
					uint8_t new_phaser;
					uint32_t new_tempo;
					uint8_t new_tremolo;
				} change;
				struct {
					uint32_t n_tuplet;
				} tuplet;
				uint8_t duration;
				const char *text;
				uint8_t strings_present;
				struct gp_note {
					uint8_t duration;
					uint8_t new_nuance;
					uint8_t value;
					uint8_t properties;
#define GP_NOTE_PROPERTY_DURATION_SPECIAL	0x01
#define GP_NOTE_PROPERTY_DOTTED				0x02
#define GP_NOTE_PROPERTY_GHOST				0x04
#define GP_NOTE_PROPERTY_EFFECT				0x08
#define GP_NOTE_PROPERTY_NUANCE_CHANGE		0x10
#define GP_NOTE_PROPERTY_ALTERATION			0x20
#define GP_NOTE_PROPERTY_ACCENTUATED		0x40
#define GP_NOTE_PROPERTY_FINGERING			0x80
					uint8_t alteration;
#define GP_NOTE_ALTERATION_LINKED			0x02
#define GP_NOTE_ALTERATION_DEAD				0x03
					struct {
						uint8_t properties1;
#define GP_NOTE_EFFECT1_BEND				0x01
#define GP_NOTE_EFFECT1_HAMMER				0x02
#define GP_NOTE_EFFECT1_SLIDE				0x04
#define GP_NOTE_EFFECT1_LET_RING			0x08
#define GP_NOTE_EFFECT1_APPOGIATURE			0x10
						uint8_t properties2;
#define GP_NOTE_EFFECT2_STACCATO			0x01
#define GP_NOTE_EFFECT2_PALM_MUTE			0x02
#define GP_NOTE_EFFECT2_TREMOLO_PICKING		0x04
#define GP_NOTE_EFFECT2_SLIDE				0x08
#define GP_NOTE_EFFECT2_HARMONIC			0x10
#define GP_NOTE_EFFECT2_TRILL				0x20
#define GP_NOTE_EFFECT2_VIBRATO				0x40
						struct {
							uint32_t num_points;
							struct gp_note_effect_bend_point 
							{
								uint32_t pitch;
							} *points;
						} bend;

						struct {
							uint8_t duration;
						} tremolo_picking;

						struct {
							uint8_t type;
						} slide;

						struct {
							uint8_t type;
						} harmonic;

						struct {
							uint8_t note_value;
							uint8_t frequency;
						} trill;

						struct {
							uint8_t duration;
							uint8_t previous_note;
							uint8_t transition;
						} appogiature;
					} effect;
					struct {
						uint8_t left_hand;
						uint8_t right_hand;
					} fingering;
				} notes[7];
			} *beats;
		} *tracks;
	} *bars;

	struct gp_track {
		const char *name;
		uint8_t spc;
		uint32_t num_frets;
		uint32_t num_strings;
		uint32_t midi_port;
		uint32_t channel1;
		uint32_t channel2;
		struct gp_track_string {
			uint32_t pitch;
		} *strings;
		struct gp_color color;
		uint32_t capo;
	} *tracks;

};

extern struct gpf *gp_read_file(const char *filename);

#endif /* __GP_H__ */
