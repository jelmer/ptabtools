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

#ifndef __PTB_H__
#define __PTB_H__

#include <sys/stat.h>
#include <glib.h>
#include <stdlib.h>

typedef guint8 ptb_chord;
typedef guint8 ptb_tone;

struct ptb_hdr {
	enum { CLASSIFICATION_SONG = 0, CLASSIFICATION_LESSON} classification;

	union {
		struct {
			enum { RELEASE_TYPE_PR_AUDIO = 0, RELEASE_TYPE_PR_VIDEO, RELEASE_TYPE_BOOTLEG, RELEASE_TYPE_UNRELEASED } release_type;

			union {
				struct {
					enum { AUDIO_TYPE_SINGLE = 0, AUDIO_TYPE_EP, AUDIO_TYPE_ALBUM, AUDIO_TYPE_DOUBLE_ALBUM, AUDIO_TYPE_TRIPLE_ALBUM, AUDIO_TYPE_BOXSET } type;
					char *album_title;
					guint16 year;
					guint8 is_live_recording;
				} pr_audio;
				struct {
					char *video_title;
					guint16 year;
					guint8 is_live_recording;
				} pr_video;
				struct {
					char *title;
					guint16 day;
					guint16 month;
					guint16 year;
				} bootleg;
				struct {
				} unreleased;
			} release_info;
			guint8 is_original_author_unknown;
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
			guint16 style;
			enum { LEVEL_BEGINNER = 0, LEVEL_INTERMEDIATE, LEVEL_ADVANCED} level;
			char *author;
			char *copyright;
		} lesson;
	} class_info;

	char *guitar_notes;
	char *bass_notes;
	char *drum_notes;
	guint16 version;
};

struct ptb_guitar {
	guint8 index;
	char *title;
	char *type;
	guint8 nr_strings;
	guint8 *strings;
	guint8 reverb;
	guint8 chorus;
	guint8 tremolo;
	guint8 pan;
	guint8 capo;
	guint8 initial_volume;
	guint8 midi_instrument;
	guint8 half_up;
	guint8 simulate;
};

struct ptb_dynamic {
	guint8 offset;
};

struct ptb_guitarin {
	guint8 offset;	
	guint8 section;
	guint8 staff;

	/* OR'd numbers of guitars 
	 * (0x01 = guitar1, 0x02 = guitar2, 0x04 = guitar3, etc) */
	guint8 rhythm_slash;
	guint8 staff_in;
};

struct ptb_font {
	guint8 size;
	guint8 thickness;
	guint8 underlined;
	guint8 italic;
	char *family;
};

struct ptb_floatingtext {
	char *text;
	guint8 beginpos;
	enum { ALIGN_LEFT = 1, ALIGN_CENTER, ALIGN_RIGHT } alignment;
	struct ptb_font font;
};

struct ptb_tempomarker {
	char *description;
	guint16 type; 
	guint8 bpm;
};

struct ptb_chorddiagram {
	ptb_chord name[2];
	guint8 frets;
	guint8 nr_strings;
	guint8 type;
	ptb_tone *tones;
};

struct ptb_chordtext {
	ptb_chord name[2];
	guint8 offset;
	guint8 additions;
	guint8 alterations;
};

struct ptb_position {
	guint8 offset;
	guint8 length;
#define POSITION_PROPERTY_STACCATO 0x02
	guint16 properties;
	guint8 let_ring;
	guint8 fermenta;
	GList *linedatas;
};

#define STAFF_TYPE_BASS_KEY	0x10

struct ptb_staff {
	/* Number of strings OR-ed with some settings */
	guint8 properties;
	guint8 extra_data;
	guint8 highest_note;
	guint8 lowest_note;
	GList *positions1;
	GList *positions2;
	GList *musicbars;
};

struct ptb_linedata {
	guint8 tone;	
#define LINEDATA_PROPERTIES_GHOST_NOTE 	0x01
#define LINEDATA_PROPERTIES_MUTED		0x02
#define LINEDATA_PROPERTIES_PULLOFF_FROM_NOWHERE	0x30
#define LINEDATA_PROPERTIES_HAMMERON_FROM_NOWHERE	0x28
	guint8 properties;
	guint8 transcribe;
	guint8 conn_to_next;
};


#define METER_TYPE_COMMON 	0x4000
#define METER_TYPE_CUT    	0x8000
#define METER_TYPE_SHOW   	0x1000
#define METER_TYPE_BEAM_2	0x0080	
#define METER_TYPE_BEAM_4	0x0100
#define METER_TYPE_BEAM_3	0x0180
#define METER_TYPE_BEAM_6	0x0200
#define METER_TYPE_BEAM_5	0x0280

#define END_MARK_TYPE_NORMAL 0x00
#define END_MARK_TYPE_REPEAT 0x80

struct ptb_section {
	char letter;
	GList *staffs;
	GList *chordtexts;
	GList *rhythmslashes;
	GList *directions;
	/* Number of times to repeat OR-ed with end mark type */
	guint8 end_mark;
	guint16 meter_type;
	guint8 beat_value;
	guint8 metronome_pulses_per_measure;
	guint16 properties;
	guint8 key_extra;
	guint8 position_width;
	char *description;
};

struct ptb_sectionsymbol {
	guint16 repeat_ending;
};

struct ptb_musicbar {
	char letter;
	char *description;
};

struct ptb_rhythmslash {
};

struct ptb_direction {
};

struct ptbf {
	char data[3];
	int fd;
	char *filename;
	struct ptb_hdr hdr;
	struct {
		GList *guitars;
		GList *sections;
		GList *guitarins;
		GList *chorddiagrams;
		GList *tempomarkers;
		GList *dynamics;
		GList *floatingtexts;
		GList *sectionsymbols;
	} instrument[2];
	off_t curpos;
	struct ptb_font default_font;
	struct ptb_font chord_name_font;
	struct ptb_font tablature_font;
};

struct ptbf *ptb_read_file(const char *ptb);

#endif /* __PTB_H__ */
