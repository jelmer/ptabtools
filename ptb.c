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
int section_index = 0;

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

ssize_t ptb_read_constant(struct ptbf *f, unsigned char expected) 
{
	unsigned char real;
	ssize_t ret = ptb_read(f, &real, 1);
	
	g_assert(real == expected);
	return ret;
}

ssize_t ptb_read_unknown(struct ptbf *f, size_t length) {
	char unknown[255];
	ssize_t ret;
	off_t oldpos = f->curpos;
	int i;

	ret = ptb_read(f, unknown, length);
	if(debugging) {
		for(i = 0; i < length; i++) 
			ptb_debug("Unknown[%04lx]: %02x", oldpos + i, unknown[i]);
	}
	return ret;
}

ssize_t ptb_read_font(struct ptbf *f, struct ptb_font *dest) {
	int ret = 0;
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
		ptb_debug("Read string: %s", data);
		*dest = data;
	} else {
		ptb_debug("Empty string");
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

int debug_level = 0;

void ptb_debug(const char *fmt, ...) 
{
	va_list ap;
	int i;
	char *newfmt, *tmp;
	if(debugging == 0) return;

	/* Add spaces */
	tmp = g_new0(char, debug_level+1);
	for(i = 0; i < debug_level; i++) tmp[i] = ' ';
	newfmt = g_strdup_printf("%s%s\n", tmp, fmt);
	free(tmp);

	va_start(ap, fmt);
	vfprintf(stderr, newfmt, ap);
	va_end(ap);

	free(newfmt);
}

int ptb_read_stuff(struct ptbf *bf) {
	ssize_t ret = 0, tmp = 1;
	guint16 end;
	
	do { 
		tmp = ptb_read_items(bf);
		ptb_debug("TMP: %d\n", tmp);
		g_assert(tmp >= 0);
		ret+=tmp;
	} while(tmp > 2);

	return ret;
}

int ptb_read_items(struct ptbf *bf) {
	int i;
	guint16 unknownval;
	guint16 l;
	guint16 length;
	guint16 header;
	guint16 nr_items;
	int ret = 0;
	char *sectionname;

	ret+=ptb_read(bf, &nr_items, 2);	
	if(ret == 0 || nr_items == 0x0) return ret; 
	ret+=ptb_read(bf, &header, 2);
	section_index++;

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
		int tmp;
		guint16 next_thing;

		ptb_debug("%02x %02x ============= Handling %s (%d of %d) =============", bf->curpos, ptb_section_handlers[i].index, ptb_section_handlers[i].name, l+1, nr_items);
		section_index++;
		debug_level++;
		tmp = ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name);
		debug_level--;

		ptb_debug("%02x ============= END Handling %s =============", ptb_section_handlers[i].index, ptb_section_handlers[i].name);

		if(tmp < 0) {
			fprintf(stderr, "Error parsing section '%s'\n", ptb_section_handlers[i].name);
		}
		ret+=tmp;
		
		if(l < nr_items - 1) {
			ret+=ptb_read(bf, &next_thing, 2);
			if(next_thing != 0x8000 + ptb_section_handlers[i].index) {
				ptb_debug("Warning: got %04x, expected %04x\n", next_thing, 0x8000 + ptb_section_handlers[i].index);
				if(next_thing & 0x8000) ptb_section_handlers[i].index = next_thing - 0x8000;
			}
		}
	}

	return ret;
}


struct ptbf *ptb_read_file(const char *file)
{
	struct ptbf *bf = g_new0(struct ptbf, 1);
	int ret = 1;
	bf->fd = open(file, O_RDONLY);

	strncpy(bf->data, "abc", 3);

	bf->filename = g_strdup(file);

	if(bf < 0) return NULL;

	bf->curpos = 1;

	debugging = 1;

	if(ptb_read_header(bf, &bf->hdr) < 0) {
		fprintf(stderr, "Error parsing header\n");	
		return NULL;
	} else if(debugging) {
		fprintf(stderr, "Header parsed correctly\n");
	}

	while(ret) {
		ret=ptb_read_items(bf);
	}

	ptb_read_font(bf, bf->tablature_font);
	ptb_read_font(bf, bf->chord_name_font);
	ptb_read_font(bf, bf->default_font);

	close(bf->fd);
	return bf;
}
