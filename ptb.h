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

struct ptb_hdr {
	char *title;
	char *artist;
	char *album;
	char *words_by;
	char *music_by;
	char *arranged_by;
	char *guitar_transcribed_by;
	char *bass_transcribed_by;
	char *lyrics;
	char *guitar_notes;
	char *bass_notes;
	char *copyright;
	guint16 version;
};

struct ptb_chord {
	
	struct ptb_chord *next;
};

struct ptb_track {
	guint8 index;
	char *title;
	char *type;
	char *tempo;
	struct ptb_track *next;
};

struct ptbf {
	int fd;
	char *filename;
	struct stat st_buf;
	struct ptb_hdr hdr;
	struct ptb_track *tracks;
};

struct ptb_section {
    char *name;
    int (*handler) (struct ptbf *, const char *section);
};

extern struct ptb_section default_sections[];

struct ptbf *ptb_read_file(const char *ptb, struct ptb_section *sections);
int ptb_read_string(int fd, char **);
int ptb_end_of_section(int fd);

#endif /* __PTB_H__ */
