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
#include <sys/stat.h>
#include <string.h>
#include "ptb_internals.h"
#include <glib.h>

int debugging = 0;

ssize_t ptb_read(struct ptbf *f, void *data, size_t length){
#undef read
	ssize_t ret = read(f->fd, data, length);
#define read DONT_USE_READ

	if(ret == -1) { 
		perror("read"); 
		g_assert(0);
	}

	f->curpos+=ret;
	
	return ret;
}

ssize_t ptb_read_unknown(struct ptbf *f, size_t length) {
	char unknown[255];
	return ptb_read(f, unknown, length);
}

ssize_t ptb_read_font(struct ptbf *f, struct ptb_font *dest) {
	int ret = 0;
	ret+=ptb_read(f, &dest->alignment, 1);
	ret+=ptb_read_string(f, &dest->family);
	ret+=ptb_read(f, &dest->size, 1);
	ret+=ptb_read_unknown(f, 5);
	ret+=ptb_read(f, &dest->thickness, 1);
	ret+=ptb_read_unknown(f, 2);
	ret+=ptb_read(f, &dest->italic, 1);
	ret+=ptb_read(f, &dest->underlined, 1);
	return ret;
}

ssize_t ptb_read_string(struct ptbf *f, char **dest) {
	guint8 shortlength;
	guint16 length;
	char *data;
	ptb_read(f, &shortlength, 1);

	/* If length is 0xff, this is followed by a guint16 length */
	if(shortlength == 0xff) {
		if(ptb_read(f, &length, 2) < 2) return -1;
	} else {
		length = shortlength;
	}

	if(length) {
		data = g_new0(char, length+1);
		if(ptb_read(f, data, length) < length) return -1;
		if(debugging) fprintf(stderr, "Read string: %s\n", data);
		*dest = data;
	} else {
		if(debugging) fprintf(stderr, "Empty string\n");
		*dest = NULL;
	}

	return length;
}

static ssize_t ptb_read_header(struct ptbf *f, struct ptb_hdr *hdr)
{
	char id[5];
	ptb_read(f, id, 4);
	id[4] = '\0';
	if(strcmp(id, "ptab")) return -1;

	ptb_read(f, &hdr->version, 2);
	ptb_read(f, &hdr->classification, 1);

	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
		ptb_read_unknown(f, 1); /* FIXME */
		ptb_read_string(f, &hdr->class_info.song.title);
		ptb_read_string(f, &hdr->class_info.song.artist);

		ptb_read(f, &hdr->class_info.song.release_type, 1);

		switch(hdr->class_info.song.release_type) {
		case RELEASE_TYPE_PR_AUDIO:
			ptb_read(f, &hdr->class_info.song.release_info.pr_audio.type, 1);
			ptb_read_string(f, &hdr->class_info.song.release_info.pr_audio.album_title);
			ptb_read(f, &hdr->class_info.song.release_info.pr_audio.year, 2);
			ptb_read(f, &hdr->class_info.song.release_info.pr_audio.is_live_recording, 1);
			break;
		case RELEASE_TYPE_PR_VIDEO:
			ptb_read_string(f, &hdr->class_info.song.release_info.pr_video.video_title);
			ptb_read(f, &hdr->class_info.song.release_info.pr_video.is_live_recording, 1);
			break;
		case RELEASE_TYPE_BOOTLEG:
			ptb_read_string(f, &hdr->class_info.song.release_info.bootleg.title);
			ptb_read(f, &hdr->class_info.song.release_info.bootleg.day, 2);
			ptb_read(f, &hdr->class_info.song.release_info.bootleg.month, 2);
			ptb_read(f, &hdr->class_info.song.release_info.bootleg.year, 2);
			break;
		case RELEASE_TYPE_UNRELEASED:
			break;

		default:
			fprintf(stderr, "Unknown release type: %d\n", hdr->class_info.song.release_type);
			break;
		}

		ptb_read(f, &hdr->class_info.song.is_original_author_unknown, 1);
		ptb_read_string(f, &hdr->class_info.song.music_by);
		ptb_read_string(f, &hdr->class_info.song.words_by);
		ptb_read_string(f, &hdr->class_info.song.arranged_by);
		ptb_read_string(f, &hdr->class_info.song.guitar_transcribed_by);
		ptb_read_string(f, &hdr->class_info.song.bass_transcribed_by);
		ptb_read_string(f, &hdr->class_info.song.copyright);
		ptb_read_string(f, &hdr->class_info.song.lyrics);
		ptb_read_string(f, &hdr->guitar_notes);
		ptb_read_string(f, &hdr->bass_notes);
		break;
	case CLASSIFICATION_LESSON:
		ptb_read_string(f, &hdr->class_info.lesson.title);
		ptb_read_string(f, &hdr->class_info.lesson.artist);
		ptb_read(f, &hdr->class_info.lesson.style, 2);
		ptb_read(f, &hdr->class_info.lesson.level, 1);
		ptb_read_string(f, &hdr->class_info.lesson.author);
		ptb_read_string(f, &hdr->guitar_notes);
		ptb_read_string(f, &hdr->class_info.lesson.copyright);
		break;

	default:
		fprintf(stderr, "Unknown classification: %d\n", hdr->classification);
		break;

	}

	return 0;
}

int ptb_read_items(struct ptbf *bf) {
	int i;
	guint16 unknownval;
	guint16 l;
	static int level = 0;
	guint16 length;
	guint16 header;
	static int section_index = 0;
	guint16 nr_items;
	int ret = 0;
	char *sectionname;

	ret+=ptb_read(bf, &nr_items, 2);	
	if(nr_items == 0x0) { fprintf(stderr, "Embedded\n"); return ret; }
	level++;
	section_index++;

	ret+=ptb_read(bf, &header, 2);

	if(header == 0xffff) { /* New section */

		/* Read Section */
		ret+=ptb_read(bf, &unknownval, 2);

		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return -1;
		}

		ret+=ptb_read(bf, &length, 2);

		sectionname = g_new0(char, length + 1);
		ret+=ptb_read(bf, sectionname, length);

		for(i = 0; ptb_section_handlers[i].name; i++) {
			if(!strcmp(ptb_section_handlers[i].name, sectionname)) {
				break;
			}
		}

		ptb_section_handlers[i].index = section_index;

		if(!ptb_section_handlers[i].handler) {
			fprintf(stderr, "No handler for '%s'\n", sectionname);
			return -1;
		}
	} else if(header & 0x8000) {
		header-=0x8000;

		for(i = 0; ptb_section_handlers[i].name; i++) {
			if(ptb_section_handlers[i].index == header) break;
		}
	} else { 
		fprintf(stderr, "Expected new item type, got %04x %02x\n", nr_items, header);
		return -1;
	}

	if(!ptb_section_handlers[i].handler) {
		fprintf(stderr, "Unable to find handler for section %s\n", sectionname);
		return -1;
	}

	for(l = 0; l < nr_items; l++) {
		int j, tmp;
		for(j = 0; j < level; j++) fputc(' ', stderr);

		fprintf(stderr, "%02x ============= Handling %s (%d of %d) =============\n", ptb_section_handlers[i].index, ptb_section_handlers[i].name, l+1, nr_items);
		section_index++;
		tmp = ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name);


		if(tmp < 0) {
			fprintf(stderr, "Error parsing section '%s'\n", ptb_section_handlers[i].name);
		}
		ret+=tmp;


		if(l < nr_items - 1) {
			unsigned char seperators[2];
			ptb_read(bf, seperators, 2);
			g_assert(seperators[1] == 0x80);
		}
	}
	level--;

	return ret;
}


struct ptbf *ptb_read_file(const char *file)
{
	struct ptbf *bf = g_new0(struct ptbf, 1);
	bf->fd = open(file, O_RDONLY);

	strncpy(bf->data, "abc", 3);

	bf->filename = g_strdup(file);

	if(bf < 0) return NULL;

	if(fstat(bf->fd, &bf->st_buf) < 0) return NULL;

	debugging = 1;

	if(ptb_read_header(bf, &bf->hdr) < 0) {
		fprintf(stderr, "Error parsing header\n");	
		return NULL;
	} else if(debugging) {
		fprintf(stderr, "Header parsed correctly\n");
	}

	while(bf->curpos < bf->st_buf.st_size) {
		if(ptb_read_items(bf) < 0) { 
			return NULL;
		}
	}

	ptb_read_font(bf, bf->tablature_font);
	ptb_read_font(bf, bf->chord_name_font);
	ptb_read_font(bf, bf->default_font);

	close(bf->fd);
	return bf;
}
