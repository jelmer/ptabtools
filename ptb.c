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

int ptb_end_of_section(int fd)
{
	guint16 data;
	if(read(fd, &data, 2) < 2) return 1;

	if(data == 0xffff) return 1;

	lseek(fd, -2, SEEK_CUR);

	return 0;
}

int ptb_read_string(int fd, char **dest) {
	guint8 shortlength;
	guint16 length;
	char *data;
	read(fd, &shortlength, 1);
	
	/* If length is 0xff, this is followed by a guint16 length */
	if(shortlength == 0xff) {
		if(read(fd, &length, 2) < 2) return -1;
	} else {
		length = shortlength;
	}
	
	if(length) {
		data = malloc(length+1);
		if(read(fd, data, length) < length) return -1;
		data[length] = '\0';
		if(debugging) printf("Read string: %s\n", data);
		*dest = data;
	} else {
		if(debugging) printf("Empty string\n");
		*dest = NULL;
	}

	return length;
}

static int ptb_read_header(int fd, struct ptb_hdr *hdr)
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
	ptb_read_string(fd, &hdr->title);
	ptb_read_string(fd, &hdr->artist);

	/* FIXME: REcord type ? */
	read(fd, unknown, 2);
//	fprintf(stderr, "%s: %02x %02x : %lx\n", debug_filename, unknown[0], unknown[1], *((guint16 *)unknown));
	ptb_read_string(fd, &hdr->album);
	
	/* d4 0700 00 */
	read(fd, unknown, 4);
	ptb_read_string(fd, &hdr->music_by);
	ptb_read_string(fd, &hdr->words_by);
	ptb_read_string(fd, &hdr->arranged_by);
	ptb_read_string(fd, &hdr->guitar_transcribed_by);
	ptb_read_string(fd, &hdr->bass_transcribed_by);
	ptb_read_string(fd, &hdr->lyrics);
	ptb_read_string(fd, &hdr->copyright);
	ptb_read_string(fd, &hdr->guitar_notes);
	ptb_read_string(fd, &hdr->bass_notes);

	/* FIXME Integer indicating something ? */
	read(fd, unknown, 2);
//	fprintf(stderr, "%s: %02x %02x : %lx\n", debug_filename, unknown[0], unknown[1], *((guint16 *)unknown));

	/* This should be 0xffff, ending the header */
	read(fd, &header_end, 2);
	if(header_end != 0xffff) return -1;

	return 0;
}

struct ptbf *ptb_read_file(const char *file, struct ptb_section *sections)
{
	struct ptbf *bf = calloc(sizeof(struct ptbf), 1);
	char eof = 0;
	int i;
	bf->fd = open(file, O_RDONLY);
	
	bf->filename = strdup(file);

	if(bf->fd < 0) return NULL;

	if(fstat(bf->fd, &bf->st_buf) < 0) return NULL;

	if(ptb_read_header(bf->fd, &bf->hdr) < 0) {
		fprintf(stderr, "Error parsing header\n");	
		return NULL;
	}

	debugging = 1;

	while(lseek(bf->fd, 0, SEEK_CUR) < bf->st_buf.st_size) {
		guint16 unknownval;
		guint16 l;
		guint16 length;
		guint8 data = 0;
		char unknown[6];
		char *sectionname;

		/* Read section */
		if(read(bf->fd, &unknownval, 2) < 2) {
			fprintf(stderr, "Unexpected end of file\n");
			return NULL;
		}

		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return NULL;
		}
		
		if(read(bf->fd, &length, 2) < 2) {
			fprintf(stderr, "Unexpected end of file\n");
			return NULL;
		}
		
		sectionname = malloc(length + 1);
		read(bf->fd, sectionname, length);
		sectionname[length] = '\0';

		//fprintf(stderr, "%s: %02x %02x %02x %02x %02x %02x\n", bf->filename, unknown[0], unknown[1], unknown[2], unknown[3], unknown[4], unknown[5]);

		for(i = 0; sections[i].name; i++) {
			if(!strcmp(sections[i].name, sectionname)) {
				break;
			}
		}

		if(!sections[i].handler) {
			fprintf(stderr, "No handler for '%s'\n", sectionname);
			return NULL;
		}

		if(sections[i].handler(bf, sectionname) != 0) {
			fprintf(stderr, "Error parsing section '%s'\n", sectionname);
		}
	}
	
	close(bf->fd);
	return bf;
}
