/*
   Functions for writing and reading PowerTab (.ptb) files
   (c) 2004-2005 Jelmer Vernooij <jelmer@samba.org>

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

#ifndef __PTB_H__
#define __PTB_H__

#include <sys/stat.h>
#include <stdlib.h>

#if defined(_MSC_VER) && !defined(PTB_CORE)
#pragma comment(lib,"ptb.lib")
#endif

#ifdef _MSC_VER
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ptb_chord;
typedef uint8_t ptb_tone;
typedef uint8_t ptb_note;

struct ptb_hdr {
	enum { CLASSIFICATION_SONG = 0, CLASSIFICATION_LESSON} classification;

	union {
		struct {
			enum { RELEASE_TYPE_PR_AUDIO = 0, RELEASE_TYPE_PR_VIDEO, RELEASE_TYPE_BOOTLEG, RELEASE_TYPE_UNRELEASED } release_type;

			union {
				struct {
					enum { AUDIO_TYPE_SINGLE = 0, AUDIO_TYPE_EP, AUDIO_TYPE_ALBUM, AUDIO_TYPE_DOUBLE_ALBUM, AUDIO_TYPE_TRIPLE_ALBUM, AUDIO_TYPE_BOXSET } type;
					char *album_title;
					uint16_t year;
					uint8_t is_live_recording;
				} pr_audio;
				struct {
					char *video_title;
					uint16_t year;
					uint8_t is_live_recording;
				} pr_video;
				struct {
					char *title;
					uint16_t day;
					uint16_t month;
					uint16_t year;
				} bootleg;
				struct {
					char empty;
				} unreleased;
			} release_info;
			uint8_t is_original_author_unknown;
			char *title;
			char *artist;
			char *words_by;
			char *music_by;
			char *arranged_by;
			char *guitar_transcribed_by;
			char *bass_transcribed_by;
			char *lyrics;
			char *copyright;
		} song;

		struct 	{
			char *title;
			char *artist;
			uint16_t style;
			enum { LEVEL_BEGINNER = 0, LEVEL_INTERMEDIATE, LEVEL_ADVANCED} level;
			char *author;
			char *copyright;
		} lesson;
	} class_info;

	char *guitar_notes;
	char *bass_notes;
	char *drum_notes;
	uint16_t version;
};

struct ptb_guitar {
	struct ptb_guitar *prev, *next;

	uint8_t index;
	char *title;
	char *type;
	uint8_t nr_strings;
	uint8_t *strings;
	uint8_t reverb;
	uint8_t chorus;
	uint8_t tremolo;
	uint8_t pan;
	uint8_t capo;
	uint8_t initial_volume;
	uint8_t midi_instrument;
	uint8_t half_up;
	uint8_t simulate;
};

struct ptb_dynamic {
	struct ptb_dynamic *prev, *next;

	uint8_t offset;
	uint8_t staff;
	uint8_t volume;
};

struct ptb_guitarin {
	struct ptb_guitarin *prev, *next;

	uint8_t offset;	
	uint8_t section;
	uint8_t staff;

	/* OR'd numbers of guitars 
	 * (0x01 = guitar1, 0x02 = guitar2, 0x04 = guitar3, etc) */
	uint8_t rhythm_slash;
	uint8_t staff_in;
};

struct ptb_font {
	uint8_t size;
	uint8_t thickness;
	uint8_t underlined;
	uint8_t italic;
	char *family;
};

struct ptb_floatingtext {
	struct ptb_floatingtext *prev, *next;

	char *text;
	uint8_t beginpos;
#define ALIGN_LEFT		1
#define ALIGN_CENTER	2
#define ALIGN_RIGHT		3
#define ALIGN_TIMESTAMP	8
	uint8_t alignment;
	struct ptb_font font;
};

struct ptb_tempomarker {
	struct ptb_tempomarker *prev, *next;

	char *description;
	uint16_t type; 
	uint8_t bpm;
	uint8_t section;
	uint8_t offset;
};

struct ptb_chorddiagram {
	struct ptb_chorddiagram *prev, *next;

	ptb_chord name[2];
	uint8_t frets;
	uint8_t nr_strings;
	uint8_t type;
	ptb_tone *tones;
};

struct ptb_chordtext {
	struct ptb_chordtext *prev, *next;

	ptb_chord name[2];
#define CHORDTEXT_PROPERTY_NOCHORD 			0x10
#define CHORDTEXT_PROPERTY_PARENTHESES		0x20
#define CHORDTEXT_PROPERTY_FORMULA_M		0x01
#define CHORDTEXT_PROPERTY_FORMULA_MAJ7		0x08
	uint8_t properties;
	uint8_t offset;
#define CHORDTEXT_ADD_9						0x40
	uint8_t additions;
	uint8_t alterations;
#define CHORDTEXT_VII						0x08
	uint8_t VII;
};

struct ptb_position {
	struct ptb_position *prev, *next;

	uint8_t offset;
#define POSITION_PALM_MUTE						0x20
#define POSITION_STACCATO 						0x02
#define POSITION_ACCENT							0x04
	uint8_t palm_mute;
	uint8_t length;
#define POSITION_DOTS_1							0x01
#define POSITION_DOTS_2							0x02
#define POSITION_DOTS_REST						0x04
#define POSITION_DOTS_ARPEGGIO_UP				0x10
	uint8_t dots;
#define POSITION_PROPERTY_IN_SINGLE_BEAM		0x0080
#define POSITION_PROPERTY_IN_DOUBLE_BEAM		0x0100
#define POSITION_PROPERTY_FIRST_IN_BEAM 		0x0400
#define POSITION_PROPERTY_LAST_IN_BEAM			0x2000
	uint16_t properties;
	uint8_t let_ring;
	uint8_t fermenta;
#define POSITION_FERMENTA_LET_RING				0x08
#define POSITION_FERMENTA_FERMENTA				0x10
#define POSITION_FERMENTA_TRIPLET_1				0x20
#define POSITION_FERMENTA_TRIPLET_2				0x40
#define POSITION_FERMENTA_TRIPLET_3				0x80
	uint8_t conn_to_next;
	struct ptb_linedata *linedatas;
};

#define STAFF_TYPE_BASS_KEY	0x10

struct ptb_staff {
	struct ptb_staff *prev, *next;

	/* Number of strings OR-ed with some settings */
	uint8_t properties;
	uint8_t highest_note;
	uint8_t lowest_note;
	struct ptb_position *positions[2];
};

struct ptb_bend
{
	uint8_t bend_pitch:4;
	uint8_t release_pitch:4;
	uint8_t bend1;
	uint8_t bend2;
	uint8_t bend3;
};

struct ptb_linedata {
	struct ptb_linedata *prev, *next;

	union {
		struct {
			unsigned int fret:5;
			unsigned int string:3;
		} detailed;
		uint8_t tone; 
	}; 
#define LINEDATA_PROPERTY_TIE					0x01
#define LINEDATA_PROPERTY_MUTED					0x02
#define LINEDATA_PROPERTY_HAMMERON_FROM			0x08
#define LINEDATA_PROPERTY_PULLOFF_FROM			0x10
#define LINEDATA_PROPERTY_NATURAL_HARMONIC		0x40
#define LINEDATA_PROPERTY_GHOST_NOTE 			0x80
	uint8_t properties;
	uint8_t transcribe;
#define LINEDATA_TRANSCRIBE_8VA					0x01
#define LINEDATA_TRANSCRIBE_15MA				0x02
#define LINEDATA_TRANSCRIBE_8VB					0x03
#define LINEDATA_TRANSCRIBE_15MB				0x04
	uint8_t conn_to_next;
	struct ptb_bend *bends;
};

#define METER_TYPE_BEAM_2	0x0080	
#define METER_TYPE_BEAM_4	0x0100
#define METER_TYPE_BEAM_3	0x0180
#define METER_TYPE_BEAM_6	0x0200
#define METER_TYPE_BEAM_5	0x0280
#define METER_TYPE_COMMON 	0x4000
#define METER_TYPE_CUT    	0x8000
#define METER_TYPE_SHOW   	0x1000

#define END_MARK_TYPE_NORMAL 	 0x00
#define END_MARK_TYPE_DOUBLELINE 0x20
#define END_MARK_TYPE_REPEAT 	 0x80

struct ptb_section {
	struct ptb_section *prev, *next;

	char letter;
	struct ptb_staff *staffs;
	struct ptb_chordtext *chordtexts;
	struct ptb_rhythmslash *rhythmslashes;
	struct ptb_direction *directions;
	struct ptb_musicbar *musicbars;
	
	/* Number of times to repeat OR-ed with end mark type */
	uint8_t end_mark;
	uint16_t meter_type;
	union {
		uint8_t beat_info;
		struct {
			unsigned int beat:3;
			unsigned int beat_value:5;
		} detailed;
	};
	uint8_t metronome_pulses_per_measure;
	uint16_t properties;
	uint8_t key_extra;
	uint8_t position_width;
	char *description;
};

struct ptb_sectionsymbol {
	struct ptb_sectionsymbol *prev, *next;

	uint16_t repeat_ending;
};

struct ptb_musicbar {
	struct ptb_musicbar *prev, *next;

	uint8_t offset;
#define MUSICBAR_PROPERTY_SINGLE_BAR    0x01
#define MUSICBAR_PROPERTY_DOUBLE_BAR    0x20
#define MUSICBAR_PROPERTY_FREE_BAR      0x40
#define MUSICBAR_PROPERTY_REPEAT_BEGIN  0x60
#define MUSICBAR_PROPERTY_REPEAT_END    0x80
#define MUSICBAR_PROPERTY_END_BAR       0xA0
	/* Number of times to repeat OR-ed only with REPEAT_END property */
	uint8_t properties;
	char letter;
	char *description;
};

struct ptb_rhythmslash {
	struct ptb_rhythmslash *prev, *next;

#define RHYTHMSLASH_PROPERTY_FIRST_IN_BEAM	0x04
	uint8_t properties;
	uint8_t offset;
	uint8_t dotted;
	uint8_t length;
};

struct ptb_direction {
	struct ptb_direction *prev, *next;
};

struct ptbf {
	char data[3];
	int fd;
	int mode;
	char *filename;
	struct ptb_hdr hdr;
	struct ptb_instrument {
		struct ptb_guitar *guitars;
		struct ptb_section *sections;
		struct ptb_guitarin *guitarins;
		struct ptb_chorddiagram *chorddiagrams;
		struct ptb_tempomarker *tempomarkers;
		struct ptb_dynamic *dynamics;
		struct ptb_floatingtext *floatingtexts;
		struct ptb_sectionsymbol *sectionsymbols;
	} instrument[2];
	off_t curpos;
	struct ptb_font default_font;
	struct ptb_font chord_name_font;
	struct ptb_font tablature_font;
};

extern struct ptbf *ptb_read_file(const char *ptb);
extern int ptb_write_file(const char *ptb, struct ptbf *);
extern void ptb_free(struct ptbf *);

extern void ptb_set_debug(int level);
extern void ptb_set_asserts_fatal(int yes);

extern const char *ptb_get_note(struct ptb_guitar *guitar, ptb_note);
extern const char *ptb_get_tone(ptb_tone);
extern const char *ptb_get_tone_full(ptb_tone);

extern void ptb_get_position_difference(struct ptb_section *, int start, int end, int *bars, int *length);

/* Reading tuning data files (tunings.dat) */

struct ptb_tuning_dict {
	uint16_t nr_tunings;

	struct ptb_tuning {
		char *name;
		uint8_t capo;
		uint8_t nr_strings;
		uint8_t *strings;
	} *tunings;
};

extern struct ptb_tuning_dict *ptb_read_tuning_dict(const char *);
extern int ptb_write_tuning_dict(const char *, struct ptb_tuning_dict *);
extern void ptb_free_tuning_dict(struct ptb_tuning_dict *);
extern const char *ptb_tuning_get_note(char);

#ifdef __cplusplus
}
#endif

#endif /* __PTB_H__ */
