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
};

struct ptb_font {
	guint8 size;
	enum { ALIGN_LEFT = 1, ALIGN_CENTER, ALIGN_RIGHT } alignment;
	guint8 thickness;
	guint8 underlined;
	guint8 italic;
	char *family;
};

struct ptb_floatingtext {
	char *text;
	guint8 beginpos;
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
	guint16 length;
#define POSITION_PROPERTY_STACCATO 0x02
	guint16 properties;
	guint8 let_ring;
	guint8 fermenta;
};

#define STAFF_TYPE_BASS_KEY	0x10

struct ptb_staff {
	/* Number of strings OR-ed with some settings */
	guint8 properties;
	guint8 child_size;
	guint8 extra_data;
};

struct ptb_linedata {
	guint8 tone;	
#define LINEDATA_PROPERTIES_GHOST_NOTE 	0x01
#define LINEDATA_PROPERTIES_MUTED		0x02
	guint8 properties;
	guint8 transcribe;
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
	/* Number of times to repeat OR-ed with end mark type */
	guint8 end_mark;
	guint16 meter_type;
	guint8 beat_value;
	guint8 metronome_pulses_per_measure;
	guint8 child_size;
	guint8 key_extra;
	char *description;
};

struct ptb_sectionsymbol {
	guint16 repeat_ending;
};

struct ptb_musicbar {
};

struct ptb_rhythmslash {
};

struct ptb_direction {
};

struct ptbf {
	char data[3];
	int fd;
	char *filename;
	struct stat st_buf;
	struct ptb_hdr hdr;
	off_t curpos;
	GList *guitars;
	GList *floatingtexts;
	GList *tempomarkers;
	GList *chorddiagrams;
	GList *linedatas;
	GList *chordtexts;
	GList *guitarins;
	GList *staffs;
	GList *positions;
	GList *sections;
	GList *dynamics;
	GList *sectionsymbols;
	GList *musicbars;
	GList *rhythmslashs;
	GList *directions;
	struct ptb_font *default_font;
	struct ptb_font *chord_name_font;
	struct ptb_font *tablature_font;
};

struct ptbf *ptb_read_file(const char *ptb);

#endif /* __PTB_H__ */
