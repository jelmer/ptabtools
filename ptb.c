/*
	(c) 2004: Jelmer Vernooij <jelmer@samba.org>

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
#include <sys/mman.h>
#include <sys/stat.h>
#include "ptb.h"
#include <glib.h>

int debugging = 0;

int handle_CGuitar (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CFloatingText (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CGuitarIn (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CTempoMarker (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CDynamic (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CSectionSymbol (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CSection (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CChordText (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CStaff (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CPosition (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CLineData (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CMusicBar (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CChordDiagram (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CRhythmSlash (struct ptbf *bf, guint8 *data, size_t len) { return 0; }
int handle_CDirection (struct ptbf *bf, guint8 *data, size_t len) { return 0; }

struct {
	char *name;
	int (*handler) (struct ptbf *, guint8 *data, size_t len);
} sections[] = {
	{"CGuitar", handle_CGuitar },
	{"CFloatingText", handle_CFloatingText },
	{"CGuitarIn", handle_CGuitarIn },
	{"CTempoMarker", handle_CTempoMarker},
	{"CDynamic", handle_CDynamic },
	{"CSectionSymbol", handle_CSectionSymbol },
	{"CSection", handle_CSection },
	{"CChordText", handle_CChordText },
	{"CStaff", handle_CStaff },
	{"CPosition", handle_CPosition },
	{"CLineData", handle_CLineData },
	{"CMusicBar", handle_CMusicBar },
	{"CChordDiagram", handle_CChordDiagram },
	{"CRhythmSlash", handle_CRhythmSlash },
	{"CDirection", handle_CDirection },
	{ 0, NULL }
};

void readstring(int fd, char **dest) {
	guint8 shortlength;
	guint16 length;
	char *data;
	read(fd, &shortlength, 1);
	
	/* If length is 0xff, this is followed by a guint16 length */
	if(shortlength == 0xff) {
		read(fd, &length, 2);
	} else {
		length = shortlength;
	}
	
	if(length) {
		data = malloc(length+1);
		read(fd, data, length);
		data[length] = '\0';
		if(debugging) printf("Read string: %s\n", data);
		*dest = data;
	} else {
		if(debugging) printf("Empty string\n");
		*dest = NULL;
	}
}


int ptb_read_header(int fd, struct ptb_hdr *hdr)
{
	char id[4];
	char unknown[256];
	guint16 header_end;

	read(fd, id, 4);
	id[4] = '\0';
	if(strcmp(id, "ptab")) return -1;

	read(fd, &hdr->version, 2);
	
	/* FIXME: 0000 / 0003 */
	read(fd, unknown, 2);
//	fprintf(stderr, "%s: %02x %02x : %lx\n", debug_filename, unknown[0], unknown[1], *((guint16 *)unknown));
	readstring(fd, &hdr->title);
	readstring(fd, &hdr->artist);

	/* FIXME: REcord type ? */
	read(fd, unknown, 2);
//	fprintf(stderr, "%s: %02x %02x : %lx\n", debug_filename, unknown[0], unknown[1], *((guint16 *)unknown));
	readstring(fd, &hdr->album);
	
	/* d4 0700 00 */
	read(fd, unknown, 4);
	readstring(fd, &hdr->music_by);
	readstring(fd, &hdr->words_by);
	readstring(fd, &hdr->arranged_by);
	readstring(fd, &hdr->guitar_transcribed_by);
	readstring(fd, &hdr->bass_transcribed_by);
	readstring(fd, &hdr->lyrics);
	readstring(fd, &hdr->copyright);
	readstring(fd, &hdr->guitar_notes);
	readstring(fd, &hdr->bass_notes);

	/* FIXME Integer indicating something ? */
	read(fd, unknown, 2);
//	fprintf(stderr, "%s: %02x %02x : %lx\n", debug_filename, unknown[0], unknown[1], *((guint16 *)unknown));

	/* This should be 0xffff, ending the header */
	read(fd, &header_end, 2);
	if(header_end != 0xffff) return -1;

	return 0;
}

struct ptb_chord *ptb_read_chords(int fd)
{
	char unknown[256];
	struct ptb_chord *prevchord = NULL, *firstchord = NULL;
	
	/* FIXME: 01 00 0d 00 */
	read(fd, unknown, 4);

}

struct ptb_track *ptb_read_tracks(int fd)
{
	char unknown[256];
	struct ptb_track *prevtrack = NULL, *firsttrack = NULL;

	/* FIXME: 01 00 07 00 */
	read(fd, unknown, 4);
	//fprintf(stderr, "%2x%2x%2x%2x\n", unknown[0], unknown[1], unknown[2], unknown[3]);

	/* FIXME: CGuitar */
	read(fd, unknown, 7);
	unknown[7] = '\0';
	//fprintf(stderr, "%s\n", unknown);

	while(1) {
		struct ptb_track *track = calloc(sizeof(struct ptb_track), 1);
	
		if(firsttrack) prevtrack->next = track;
		else firsttrack = track;
		
		/* Track number */
		read(fd, &track->index, 1);
		if(track->index == 0xff) return firsttrack;
		readstring(fd, &track->title);

		//FIXME
		read(fd, unknown, 8);
		fprintf(stderr, "%s %02x %02x %02x\n", debug_filename, unknown[0], unknown[1], unknown[2]);
		fprintf(stderr, "%s %02x %02x %02x\n", debug_filename, unknown[3], unknown[4], unknown[5]);
		fprintf(stderr, "%s %02x %02x\n", debug_filename, unknown[6], unknown[7]);
		readstring(fd, &track->type);

		//FIXME
		read(fd, unknown, 10);

		prevtrack = track;
	}

	return firsttrack;
}

struct ptbf *readptb(const char *file)
{
	struct ptbf *bf = calloc(sizeof(struct ptbf), 1);
	char eof = 0;
	int i;
	bf->fd = open(file, O_RDONLY);
	
	bf->filename = strdup(file);

	if(bf->fd < 0) return NULL;

	if(fstat(bf->fd, &bf->st_buf) < 0) return NULL;

	if(ptb_read_header(bf->fd, &bf->hdr) < 0) return NULL;

	debugging = 1;

	while(!eof) {
		guint16 unknownval;
		guint16 l;
		guint16 length;
		guint8 data = 0;
		char unknown[6];
		char *sectionname;
		/* Read section */
		read(bf->fd, &unknownval, 2);
		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return NULL;
		}
		read(bf->fd, &length, 2);
		
		sectionname = malloc(length + 1);
		read(bf->fd, sectionname, length);
		sectionname[length] = '\0';

		read(bf->fd, unknown, 6);

		fprintf(stderr, "%s: %02x %02x %02x %02x %02x %02x\n", debug_filename, unknown[0], unknown[1], unknown[2], unknown[3], unknown[4], unknown[5]);

		fprintf(stderr, "%d\n", *((short *)unknown));

		l = 0;
		while(data != 0xff) {
			l++;
			if(read(bf->fd, &data, 1) < 1) { eof = 1; break; }
			if(data == 0xff)  {
				l++;
				if(read(bf->fd, &data, 1) < 1) { eof = 1; break; }
			}
		}

		for(i = 0; sections[i].name; i++) {
			if(!strcmp(sections[i].name, sectionname)) {
				if(sections[i].handler(bf, &data, l-2) != 0) {
					fprintf(stderr, "Error parsing section '%s'\n", sectionname);
				}
				break;
			}
		}

		if(!sections[i].name) {
			fprintf(stderr, "Unknown section '%s'\n", sectionname);
		}
	}
	
	close(bf->fd);
	return bf;
}
