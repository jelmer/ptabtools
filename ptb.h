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
	guint8 nr_guitars;
	guint16 version;
};

struct ptb_chord {

	struct ptb_chord *next;
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
	guint8 string_12;
	guint8 rev_gtr;
	struct ptb_guitar *next;
};

struct ptb_font {
	guint8 size;
	char *family;
};

struct ptb_floatingtext {
	char *text;
	struct ptb_font font;
	struct ptb_floatingtext *next;
};

struct ptb_tempomarker {
	char *description;
	guint8 bpm;
	struct ptb_tempomarker *next;
};

struct ptb_chorddiagram {
	ptb_chord name;
	guint8 frets;
	guint8 nr_strings;
	guint8 type;
	ptb_tone *tones;
	struct ptb_chorddiagram *next;
};

struct ptb_linedata {
	
	struct ptb_linedata *next;
};

struct ptbf {
	int fd;
	char *filename;
	struct stat st_buf;
	struct ptb_hdr hdr;
	struct ptb_guitar *guitars;
	struct ptb_floatingtext *floating_texts;
	struct ptb_tempomarker *tempomarkers;
	struct ptb_chorddiagram *chorddiagrams;
	struct ptb_linedata *linedatas;
};

struct ptb_section {
	char *name;
	int (*handler) (struct ptbf *, const char *section);
};

extern struct ptb_section default_sections[];

struct ptbf *ptb_read_file(const char *ptb, struct ptb_section *sections);
int ptb_read_string(int fd, char **);
int ptb_end_of_section(int fd);

extern int debugging;

#endif /* __PTB_H__ */
