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

int ptb_read_font(int fd, struct ptb_font *dest) {
	char unknown[256];
	int ret = 0;
	ret+=read(fd, &dest->alignment, 1);
	ret+=ptb_read_string(fd, &dest->family);
	ret+=read(fd, &dest->size, 1);
	ret+=read(fd, unknown, 5);
	ret+=read(fd, &dest->thickness, 1);
	ret+=read(fd, unknown, 2);
	ret+=read(fd, &dest->italic, 1);
	ret+=read(fd, &dest->underlined, 1);
	
	return ret;
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
		if(debugging) fprintf(stderr, "Read string: %s\n", data);
		*dest = data;
	} else {
		if(debugging) fprintf(stderr, "Empty string\n");
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
	read(fd, &hdr->classification, 1);

	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
	read(fd, unknown, 1); /* FIXME */
	ptb_read_string(fd, &hdr->class_info.song.title);
	ptb_read_string(fd, &hdr->class_info.song.artist);

	read(fd, &hdr->class_info.song.release_type, 1);

	switch(hdr->class_info.song.release_type) {
	case RELEASE_TYPE_PR_AUDIO:
	read(fd, &hdr->class_info.song.release_info.pr_audio.type, 1);
	ptb_read_string(fd, &hdr->class_info.song.release_info.pr_audio.album_title);
	read(fd, &hdr->class_info.song.release_info.pr_audio.year, 2);
	read(fd, &hdr->class_info.song.release_info.pr_audio.is_live_recording, 1);
	break;
	case RELEASE_TYPE_PR_VIDEO:
	ptb_read_string(fd, &hdr->class_info.song.release_info.pr_video.video_title);
	read(fd, &hdr->class_info.song.release_info.pr_video.is_live_recording, 1);
	break;
	case RELEASE_TYPE_BOOTLEG:
		ptb_read_string(fd, &hdr->class_info.song.release_info.bootleg.title);
		read(fd, &hdr->class_info.song.release_info.bootleg.day, 2);
		read(fd, &hdr->class_info.song.release_info.bootleg.month, 2);
		read(fd, &hdr->class_info.song.release_info.bootleg.year, 2);
		break;
	case RELEASE_TYPE_UNRELEASED:
	break;

	default:
		fprintf(stderr, "Unknown release type: %d\n", hdr->class_info.song.release_type);
		break;
	}

	read(fd, &hdr->class_info.song.is_original_author_unknown, 1);
	ptb_read_string(fd, &hdr->class_info.song.music_by);
	ptb_read_string(fd, &hdr->class_info.song.words_by);
	ptb_read_string(fd, &hdr->class_info.song.arranged_by);
	ptb_read_string(fd, &hdr->class_info.song.guitar_transcribed_by);
	ptb_read_string(fd, &hdr->class_info.song.bass_transcribed_by);
	ptb_read_string(fd, &hdr->class_info.song.copyright);
	ptb_read_string(fd, &hdr->class_info.song.lyrics);
	ptb_read_string(fd, &hdr->guitar_notes);
	ptb_read_string(fd, &hdr->bass_notes);
	break;
	case CLASSIFICATION_LESSON:
	ptb_read_string(fd, &hdr->class_info.lesson.title);
	ptb_read_string(fd, &hdr->class_info.lesson.artist);
	read(fd, &hdr->class_info.lesson.style, 2);
	read(fd, &hdr->class_info.lesson.level, 1);
	ptb_read_string(fd, &hdr->class_info.lesson.author);
	ptb_read_string(fd, &hdr->guitar_notes);
	ptb_read_string(fd, &hdr->class_info.lesson.copyright);
	break;

	default:
	fprintf(stderr, "Unknown classification: %d\n", hdr->classification);
	break;

	}

	return 0;
}

int ptb_read_item(struct ptbf *bf, struct ptb_section_handler *sections) {
	int i;
	guint16 unknownval;
	guint16 l;
	guint16 length;
	guint16 header;
	guint8 data = 0;
	static section_index = 0;
	guint16 nr_items;
	char *sectionname;

	read(bf->fd, &nr_items, 2);	
	if(nr_items == 0x0) { return 0; }
	section_index++;

	read(bf->fd, &header, 2);
	fprintf(stderr, "Header: %04x\n", header);

	/* Read section */
	if(read(bf->fd, &unknownval, 2) < 2) {
		fprintf(stderr, "Unexpected end of file\n");
		return -1;
	}

	if(unknownval != 0x0001) {
		fprintf(stderr, "Unknownval: %04x\n", unknownval);
		return -1;
	}

	if(read(bf->fd, &length, 2) < 2) {
		fprintf(stderr, "Unexpected end of file\n");
		return -1;
	}

	sectionname = malloc(length + 1);
	read(bf->fd, sectionname, length);
	sectionname[length] = '\0';
	fprintf(stderr, "---- %s ----: %d (%02x) \n", sectionname, nr_items, section_index);
	

	for(i = 0; sections[i].name; i++) {
		if(!strcmp(sections[i].name, sectionname)) {
			break;
		}
	}

	sections[i].index = section_index;

	if(!sections[i].handler) {
		fprintf(stderr, "No handler for '%s'\n", sectionname);
		return -1;
	}

	for(l = 0; l < nr_items; l++) {
		char unknown[2];
		section_index++;
		if(sections[i].handler(bf, sectionname) != 0) {
			fprintf(stderr, "Error parsing section '%s'\n", sectionname);
		}


		if(l < nr_items - 1) {
			read(bf->fd, unknown, 2);
			fprintf(stderr, "Seperators: %02x %02x\n", unknown[0], unknown[1]);
		}
	}

	fprintf(stderr, "------ End of %s ------ \n", sectionname);
	return 0;
}


struct ptbf *ptb_read_file(const char *file, struct ptb_section_handler *sections)
{
	struct ptbf *bf = calloc(sizeof(struct ptbf), 1);
	char eof = 0;
	bf->fd = open(file, O_RDONLY);

	bf->filename = strdup(file);

	if(bf->fd < 0) return NULL;

	if(fstat(bf->fd, &bf->st_buf) < 0) return NULL;

	debugging = 1;

	if(ptb_read_header(bf->fd, &bf->hdr) < 0) {
		fprintf(stderr, "Error parsing header\n");	
		return NULL;
	} else if(debugging) {
		fprintf(stderr, "Header parsed correctly\n");
	}

	while(lseek(bf->fd, 0L, SEEK_CUR) < bf->st_buf.st_size) {
		if(ptb_read_item(bf, sections) != 0) return NULL;
	}

	close(bf->fd);
	return bf;
}
