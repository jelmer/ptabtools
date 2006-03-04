/*
   Parser library for the PowerTab (PTB) file format
   (c) 2004-2006: Jelmer Vernooij <jelmer@samba.org>

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
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include "dlinklist.h"

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ ""
#endif

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
typedef int ssize_t;
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#define PTB_CORE
#include "ptb.h"

int assert_is_fatal = 0;

#define ptb_assert(ptb, expr) \
	if (!(expr)) { ptb_debug("---------------------------------------------"); \
		ptb_error("file: %s, line: %d (%s): assertion failed: %s. Current position: 0x%lx", __FILE__, __LINE__, __PRETTY_FUNCTION__, #expr, ptb->curpos); \
		if(assert_is_fatal) abort(); \
	}

#define malloc_p(t,n) (t *) calloc(sizeof(t), n)

#define GET_ITEM(bf, dest, type)  ((bf)->mode == O_WRONLY?(type *)(*(dest)):malloc_p(type, 1))

#define ptb_assert_0(ptb, expr) \
	if(expr) ptb_error("%s == 0x%x!", #expr, expr); \
/*	ptb_assert(ptb, (expr) == 0); */

struct ptb_list {
	struct ptb_list *prev, *next;
};

struct ptb_section_handler {
	char *name;
	int (*handler) (struct ptbf *, const char *section, struct ptb_list **ret);
};

extern struct ptb_section_handler ptb_section_handlers[];

static void ptb_debug(const char *fmt, ...);
static void ptb_error(const char *fmt, ...);

int debugging = 0;

static ssize_t ptb_read(struct ptbf *f, void *data, ssize_t length)
{
	ssize_t ret = read(f->fd, data, length);
#define read DONT_USE_READ

	if(ret == -1) { 
		perror("read"); 
		ptb_assert(f, 0);
	} else if (ret != length) {
		ptb_error("Expected read of %d bytes, got %d\n", length, ret);
	}

	f->curpos+=ret;
	
	return ret;
}

static ssize_t ptb_write(struct ptbf *f, void *data, size_t length)
{
	ssize_t ret = write(f->fd, data, length);
#define write DONT_USE_WRITE

	if(ret == -1) { 
		perror("write"); 
		ptb_assert(f, 0);
	}

	f->curpos+=ret;
	
	return ret;
}

static ssize_t ptb_data(struct ptbf *f, void *data, size_t length)
{
	switch (f->mode) {
	case O_RDONLY: return ptb_read(f, data, length);
	case O_WRONLY: return ptb_write(f, data, length);
	}
	ptb_assert(f, 0);
	return 0;
}

#define ptb_data_uint8(f,d) ptb_data(f,d,1)
#define ptb_data_uint16(f,d) ptb_data(f,d,2)
#define ptb_data_uint32(f,d) ptb_data(f,d,4)

#define ptb_data_constant(f,e) ptb_data_constant_helper(f,e,__LINE__)
static ssize_t ptb_data_constant_helper(struct ptbf *f, unsigned char expected, int line) 
{
	unsigned char real;
	ssize_t ret;
	
	if (f->mode == O_WRONLY) {
		ret = ptb_data_uint8(f, &expected);
	} else {
			ret = ptb_data_uint8(f, &real);
	
		if(real != expected) {
			ptb_error("%04lx: Expected %02x, got %02x at line "__FILE__":%d", f->curpos-1, expected, real, line);
			ptb_assert(f, 0);
		}
	}

	return ret;
}

static ssize_t ptb_data_constant_string(struct ptbf *f, const char *expected, int length)
{
	int i, ret = 0;
	for (i = 0; i < length; i++) {
		ret = ptb_data_constant(f, expected[i]);
	}

	return ret;
}

static ssize_t ptb_data_unknown(struct ptbf *f, size_t length, const char *comment) {
	char unknown[255];
	ssize_t ret;
	off_t oldpos = f->curpos;
	size_t i;

	memset(unknown, 0, length);
	ret = ptb_data(f, unknown, length);
	if(debugging) {
		for(i = 0; i < length; i++) 
			ptb_debug("Unknown[%04lx]: %02x", oldpos + i, unknown[i]);
	}
	return ret;
}

static ssize_t ptb_read_string(struct ptbf *f, char **dest)
{
	uint8_t shortlength;
	uint16_t length;
	char *data;
	ptb_data_uint8(f, &shortlength);

	/* If length is 0xff, this is followed by a uint16_t length */
	if(shortlength == 0xff) {
		if(ptb_data_uint16(f, &length) < 2) return -1;
	} else {
		length = shortlength;
	}

	if(length) {
		data = malloc_p(char, length+1);
		if(ptb_data(f, data, length) < length) return -1;
		ptb_debug("Read string: %s", data);
		*dest = data;
	} else {
		ptb_debug("Empty string");
		*dest = NULL;
	}

	return length;
}

static ssize_t ptb_write_string(struct ptbf *f, char **dest)
{
	uint8_t shortlength;
	uint16_t length = 0;
	
	if (*dest == NULL) {
		shortlength = 0;
	} else if (strlen(*dest) >= 0xff) {
		shortlength = 0xff;
		length = strlen(*dest);
	} else {
		shortlength = strlen(*dest);
	}
	
	ptb_data_uint8(f, &shortlength);

	/* If length is 0xff, this is followed by a uint16_t length */
	if(shortlength == 0xff) {
		if(ptb_data_uint16(f, &length) < 2) return -1;
	} else {
		length = shortlength;
	}

	if(ptb_data(f, *dest, length) < length) return -1;

	return length;
}

static ssize_t ptb_data_string(struct ptbf *bf, char **dest) {
	switch (bf->mode) {
	case O_RDONLY: return ptb_read_string(bf, dest);
	case O_WRONLY: return ptb_write_string(bf, dest);
	default: ptb_assert(bf, 0);
	}
	return 0;
}


static ssize_t ptb_data_rect(struct ptbf *f, struct ptb_rect *dest)
{
	int ret = 0;
	ret+=ptb_data_unknown(f, 16, "RECTANGLE");
	return ret;
}

static ssize_t ptb_data_color(struct ptbf *f, struct ptb_color *dest)
{
	int ret = 0;

	ret+=ptb_data_uint8(f, &dest->r);
	ret+=ptb_data_uint8(f, &dest->g);
	ret+=ptb_data_uint8(f, &dest->b);
	ret+=ptb_data_unknown(f, 1, "FIXME");

	return ret;
}

static ssize_t ptb_data_font(struct ptbf *f, struct ptb_font *dest) 
{
	int ret = 0;
	ret+=ptb_data_string(f, &dest->family);
	ret+=ptb_data_uint32(f, &dest->pointsize);
	ret+=ptb_data_uint32(f, &dest->weight);
	ret+=ptb_data_uint8(f, &dest->italic);
	ret+=ptb_data_uint8(f, &dest->underlined);
	ret+=ptb_data_uint8(f, &dest->strikeout);
	ret+=ptb_data_color(f, &dest->color);
	return ret;
}

static ssize_t ptb_data_header(struct ptbf *f, struct ptb_hdr *hdr)
{
	ptb_data_constant_string(f, "ptab", 4);

	ptb_data_uint16(f, &hdr->version);
	ptb_data_uint8(f, &hdr->classification);

	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
		ptb_data_uint8(f, &hdr->class_info.song.content_type);
		ptb_data_string(f, &hdr->class_info.song.title);
		ptb_data_string(f, &hdr->class_info.song.artist);

		ptb_data_uint8(f, &hdr->class_info.song.release_type);

		switch(hdr->class_info.song.release_type) {
		case RELEASE_TYPE_PR_AUDIO:
			ptb_data_uint8(f, &hdr->class_info.song.release_info.pr_audio.type);
			ptb_data_string(f, &hdr->class_info.song.release_info.pr_audio.album_title);
			ptb_data_uint16(f, &hdr->class_info.song.release_info.pr_audio.year);
			ptb_data_uint8(f, &hdr->class_info.song.release_info.pr_audio.is_live_recording);
			break;
		case RELEASE_TYPE_PR_VIDEO:
			ptb_data_string(f, &hdr->class_info.song.release_info.pr_video.video_title);
			ptb_data_uint8(f, &hdr->class_info.song.release_info.pr_video.is_live_recording);
			break;
		case RELEASE_TYPE_BOOTLEG:
			ptb_data_string(f, &hdr->class_info.song.release_info.bootleg.title);
			ptb_data_uint16(f, &hdr->class_info.song.release_info.bootleg.day);
			ptb_data_uint16(f, &hdr->class_info.song.release_info.bootleg.month);
			ptb_data_uint16(f, &hdr->class_info.song.release_info.bootleg.year);
			break;
		case RELEASE_TYPE_UNRELEASED:
			break;

		default:
			fprintf(stderr, "Unknown release type: %d\n", hdr->class_info.song.release_type);
			break;
		}

		ptb_data_uint8(f, &hdr->class_info.song.is_original_author_unknown);
		ptb_data_string(f, &hdr->class_info.song.music_by);
		ptb_data_string(f, &hdr->class_info.song.words_by);
		ptb_data_string(f, &hdr->class_info.song.arranged_by);
		ptb_data_string(f, &hdr->class_info.song.guitar_transcribed_by);
		ptb_data_string(f, &hdr->class_info.song.bass_transcribed_by);
		ptb_data_string(f, &hdr->class_info.song.copyright);
		ptb_data_string(f, &hdr->class_info.song.lyrics);
		ptb_data_string(f, &hdr->guitar_notes);
		ptb_data_string(f, &hdr->bass_notes);
		break;
	case CLASSIFICATION_LESSON:
		ptb_data_string(f, &hdr->class_info.lesson.title);
		ptb_data_string(f, &hdr->class_info.lesson.artist);
		ptb_data_uint16(f, &hdr->class_info.lesson.style);
		ptb_data_uint8(f, &hdr->class_info.lesson.level);
		ptb_data_string(f, &hdr->class_info.lesson.author);
		ptb_data_string(f, &hdr->guitar_notes);
		ptb_data_string(f, &hdr->class_info.lesson.copyright);
		break;

	default:
		fprintf(stderr, "Unknown classification: %d\n", hdr->classification);
		break;

	}

	return 0;
}

static void default_error_fn(const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
}

static void (*error_fn) (const char *, va_list) = default_error_fn;
void ptb_set_error_fn(void (*fn) (const char *, va_list)) { error_fn = fn; }

static void ptb_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (error_fn) error_fn(fmt, ap);		
	va_end(ap);
}

static int debug_level = 0;

static void ptb_debug(const char *fmt, ...) 
{
	va_list ap;
	int i;
	if(debugging == 0) return;

	/* Add spaces */
	for(i = 0; i < debug_level; i++) fprintf(stderr, " ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

static int ptb_read_items(struct ptbf *bf, const char *assumed_type, struct ptb_list **result) {
	int i;
	uint16_t unknownval;
	uint16_t l;
	uint16_t length;
	uint16_t header;
	uint16_t nr_items;
	int ret = 0;

	*result = NULL;

	ret+=ptb_data_uint16(bf, &nr_items);	
	if(ret == 0 || nr_items == 0x0) return 1; 
	ret+=ptb_data_uint16(bf, &header);

	ptb_debug("Going to read %d items", nr_items);

	if(header == 0xffff) { /* New section */
		char *my_section_name;
		/* Read Section */
		ret+=ptb_data_uint16(bf, &unknownval);

		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return 0;
		}

		ret+=ptb_data_uint16(bf, &length);

		my_section_name = malloc_p(char, length + 1);
		ret+=ptb_data(bf, my_section_name, length);

	} else if(header & 0x8000) {
	} else { 
		ptb_error("Expected new item type (%s), got %04x %02x\n", assumed_type, nr_items, header);
		ptb_assert(bf, 0);
		return 0;
	}

	for(i = 0; ptb_section_handlers[i].name; i++) {
		if(!strcmp(ptb_section_handlers[i].name, assumed_type)) {
			break;
		}
	}

	if(!ptb_section_handlers[i].handler) {
		fprintf(stderr, "Unable to find handler for section %s\n", assumed_type);
		return 0;
	}

	for(l = 0; l < nr_items; l++) {
		struct ptb_list *item;
		int ret;
		uint16_t next_thing;

		ptb_debug("%04x ============= Handling %s (%d of %d) =============", bf->curpos, assumed_type, l+1, nr_items);
		debug_level++;
		item = NULL;
		ret = ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name, (struct ptb_list **)&item);
		if (!ret) item = NULL;
		debug_level--;

		ptb_debug("%04x ============= END Handling %s (%d of %d) =============", bf->curpos, ptb_section_handlers[i].name, l+1, nr_items);

		if(!item) {
			fprintf(stderr, "Error parsing section '%s'\n", ptb_section_handlers[i].name);
		} else {
			DLIST_ADD_END((*result), item, struct ptb_list *);
		}
		
		if(l < nr_items - 1) {
			ret+=ptb_data_uint16(bf, &next_thing);
			if(!(next_thing & 0x8000)) {
				ptb_error("Warning: got %04x, expected | 0x8000\n", next_thing);
				ptb_assert(bf, 0);
			}
		}
	}

	return 1;
}

static int generate_header_id(struct ptbf *bf, const char *assumed_type)
{
	static int i = 0;
	/* FIXME */
	return ++i;
}

static int ptb_write_items(struct ptbf *bf, const char *assumed_type, struct ptb_list **result) 
{
	int i;
	uint16_t length;
	uint16_t header;
	uint16_t id = 0x20;
	uint16_t nr_items;
	struct ptb_list *gl;
	int ret = 0;

	DLIST_LEN(*result, nr_items, struct ptb_list *);

	ret+=ptb_data_uint16(bf, &nr_items);	
	if(nr_items == 0x0) return 1; 

	header = generate_header_id(bf, assumed_type);
	if (header == 0) header = 0xffff;

	ret+=ptb_data_uint16(bf, &header);

	ptb_debug("Going to write %d items", nr_items);

	if (header == 0xffff) {
		uint16_t unknownval = 0x0001; /* FIXME */

		ret+=ptb_data_uint16(bf, &unknownval);
		length = strlen(assumed_type);
		ret+=ptb_data_uint16(bf, &length);
		ret+=ptb_data(bf, (void *)assumed_type, length);
	}


	for(i = 0; ptb_section_handlers[i].name; i++) {
		if(!strcmp(ptb_section_handlers[i].name, assumed_type)) {
			break;
		}
	}

	if(!ptb_section_handlers[i].handler) {
		fprintf(stderr, "Unable to find handler for section %s\n", assumed_type);
		return 0;
	}

	gl = *result;
	while(gl) 
	{
		debug_level++;
		ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name, (struct ptb_list **)&gl);
		debug_level--;

		if(gl->next) {
			ret+=ptb_data_uint16(bf, &id);
		}
		gl = gl->next;
	}

	return 1;

}

static int ptb_data_items(struct ptbf *bf, const char *assumed_type, struct ptb_list **result)
{
	switch (bf->mode) {
	case O_RDONLY: return ptb_read_items(bf, assumed_type, result);
	case O_WRONLY: return ptb_write_items(bf, assumed_type, result);
	default: ptb_assert(bf, 0);
	}
	return 0;
}



void ptb_set_debug(int level) { debugging = level; }

void ptb_set_asserts_fatal(int y) { assert_is_fatal = y; }

static void ptb_data_instrument(struct ptbf *bf, int i)
{
	ptb_data_items(bf, "CGuitar", (struct ptb_list **)&bf->instrument[i].guitars);
	ptb_data_items(bf, "CChordDiagram",(struct ptb_list **) &bf->instrument[i].chorddiagrams);
	ptb_data_items(bf, "CFloatingText",(struct ptb_list **) &bf->instrument[i].floatingtexts);
	ptb_data_items(bf, "CGuitarIn",(struct ptb_list **) &bf->instrument[i].guitarins);
	ptb_data_items(bf, "CTempoMarker", (struct ptb_list **)&bf->instrument[i].tempomarkers);
	ptb_data_items(bf, "CDynamic", (struct ptb_list **)&bf->instrument[i].dynamics);
	ptb_data_items(bf, "CSectionSymbol", (struct ptb_list **)&bf->instrument[i].sectionsymbols);
	ptb_data_items(bf, "CSection", (struct ptb_list **)&bf->instrument[i].sections);
}

static ssize_t ptb_data_file(struct ptbf *bf)
{
	int i;
	if(ptb_data_header(bf, &bf->hdr) < 0) {
		fprintf(stderr, "Error parsing header\n");	
		return -1;
	} else if(debugging) {
		fprintf(stderr, "Header parsed correctly\n");
	}

	for(i = 0; i < 2; i++) {
		ptb_data_instrument(bf, i);
	}

	ptb_data_font(bf, &bf->tablature_font);
	ptb_data_font(bf, &bf->chord_name_font);
	ptb_data_font(bf, &bf->default_font);

	ptb_data_uint32(bf, &bf->staff_line_space);
	ptb_data_uint32(bf, &bf->fade_in);
	ptb_data_uint32(bf, &bf->fade_out);
	return 0;
}

struct ptbf *ptb_read_file(const char *file)
{
	struct ptbf *bf = malloc_p(struct ptbf, 1);

	bf->mode = O_RDONLY;
	bf->fd = open(file, bf->mode
#ifdef O_BINARY
				  | O_BINARY
#endif
				  );

	strncpy(bf->data, "abc", 3);

	bf->filename = strdup(file);

	if(bf->fd < 0) return NULL;

	bf->curpos = 1;

	if (ptb_data_file(bf) == -1) 
		return NULL;

	close(bf->fd);
	return bf;
}

int ptb_write_file(const char *file, struct ptbf *bf)
{
	bf->mode = O_WRONLY;
	bf->fd = creat(file, 0644);

	strncpy(bf->data, "abc", 3);

	bf->filename = strdup(file);

	if(bf->fd < 0) return -1;

	bf->curpos = 1;

	if (ptb_data_file(bf) == -1) 
		return -1;

	close(bf->fd);
	return 0;
}

static int handle_unknown (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	fprintf(stderr, "Unknown section '%s'\n", section);	
	return 1; 
}

static int handle_CGuitar (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_guitar *guitar = GET_ITEM(bf, dest, struct ptb_guitar);

	ptb_data_uint8(bf, &guitar->index);
	ptb_data_string(bf, &guitar->title);

	ptb_data_uint8(bf, &guitar->midi_instrument);
	ptb_data_uint8(bf, &guitar->initial_volume);
	ptb_data_uint8(bf, &guitar->pan);
	ptb_data_uint8(bf, &guitar->reverb);
	ptb_data_uint8(bf, &guitar->chorus);
	ptb_data_uint8(bf, &guitar->tremolo);

	ptb_data_uint8(bf, &guitar->simulate);
	ptb_data_uint8(bf, &guitar->capo);
		
	ptb_data_string(bf, &guitar->type);

	ptb_data_uint8(bf, &guitar->half_up);
	ptb_data_uint8(bf, &guitar->nr_strings);

	if (bf->mode == O_RDONLY) {
		guitar->strings = malloc_p(uint8_t, guitar->nr_strings);
	}

	ptb_data(bf, guitar->strings, guitar->nr_strings);

	*dest = (struct ptb_list *)guitar;

	return 1;
}


static int handle_CFloatingText (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_floatingtext *text = GET_ITEM(bf, dest, struct ptb_floatingtext);

	ptb_data_string(bf, &text->text);
	ptb_data_rect(bf, &text->rect);
	ptb_data_uint8(bf, &text->alignment);
	ptb_debug("Align: %x", text->alignment);
	ptb_assert(bf, (text->alignment &~ ALIGN_BORDER &~ ALIGN_CENTER 
					&~ ALIGN_LEFT &~ ALIGN_RIGHT)  == 0);
	ptb_data_font(bf, &text->font);
	
	*dest = (struct ptb_list *)text;

	return 1;
}

static int handle_CSection (struct ptbf *bf, const char *sectionname, struct ptb_list **dest) { 
	struct ptb_section *section = GET_ITEM(bf, dest, struct ptb_section);

	ptb_data_constant(bf, 0x32);
	ptb_data_unknown(bf, 11, "FIXME");
	ptb_data_uint16(bf, &section->properties);
	ptb_data_unknown(bf, 2, "FIXME");
	ptb_data_uint8(bf, &section->end_mark);
	ptb_assert(bf, (section->end_mark &~ END_MARK_TYPE_NORMAL 
			   & ~END_MARK_TYPE_DOUBLELINE
			   & ~END_MARK_TYPE_REPEAT) < 24);
	ptb_data_uint8(bf, &section->position_width);
	ptb_data_unknown(bf, 5, "FIXME");
	ptb_data_uint8(bf, &section->key_extra);
	ptb_data_unknown(bf, 1, "FIXME");
	ptb_data_uint16(bf, &section->meter_type);
	ptb_data_uint8(bf, &section->beat_info);
	ptb_data_uint8(bf, &section->metronome_pulses_per_measure);
	ptb_data_uint8(bf, &section->letter);
	ptb_data_string(bf, &section->description);

	ptb_data_items(bf, "CDirection", (struct ptb_list **)&section->directions);
	ptb_data_items(bf, "CChordText", (struct ptb_list **)&section->chordtexts);
	ptb_data_items(bf, "CRhythmSlash", (struct ptb_list **)&section->rhythmslashes);
	ptb_data_items(bf, "CStaff", (struct ptb_list **)&section->staffs);
	/* FIXME: Barlinearray */
	/* FIXME: Barline */
	ptb_data_items(bf, "CMusicBar", (struct ptb_list **)&section->musicbars);

	*dest = (struct ptb_list *)section;
	return 1;
}

static int handle_CTempoMarker (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_tempomarker *tempomarker = GET_ITEM(bf, dest, struct ptb_tempomarker);

	ptb_data_uint8(bf, &tempomarker->section);
	ptb_data_constant(bf, 0);
	ptb_data_uint8(bf, &tempomarker->offset);
	ptb_data_uint8(bf, &tempomarker->bpm);
	ptb_data_constant(bf, 0);
	ptb_data_uint16(bf, &tempomarker->type);
	ptb_data_string(bf, &tempomarker->description);

	*dest = (struct ptb_list *)tempomarker;
	return 1;
}

static int ptb_data_chordname(struct ptbf *bf, struct ptb_chordname *dest)
{
	int ret = 0;
	ret += ptb_data(bf, dest->name, 2);
	ret += ptb_data_uint8(bf, &dest->formula);
	ret += ptb_data_uint16(bf, &dest->formula_mods);
	ptb_data_uint8(bf, &dest->type);
	return ret;
}

static int handle_CChordDiagram (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_chorddiagram *chorddiagram = GET_ITEM(bf, dest, struct ptb_chorddiagram);

	ptb_data_chordname(bf, &chorddiagram->name);
	ptb_data_uint8(bf, &chorddiagram->frets);
	ptb_data_uint8(bf, &chorddiagram->nr_strings);
	chorddiagram->tones = malloc_p(uint8_t, chorddiagram->nr_strings);
	ptb_data(bf, chorddiagram->tones, chorddiagram->nr_strings);

	*dest = (struct ptb_list *)chorddiagram;
	return 1;
}

static int handle_CLineData (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_linedata *linedata = GET_ITEM(bf, dest, struct ptb_linedata);

	ptb_data_uint8(bf, &linedata->tone);
	ptb_data_uint8(bf, &linedata->properties);
	ptb_assert_0(bf, linedata->properties
			   & ~LINEDATA_PROPERTY_GHOST_NOTE
			   & ~LINEDATA_PROPERTY_PULLOFF_FROM
			   & ~LINEDATA_PROPERTY_HAMMERON_FROM
			   & ~LINEDATA_PROPERTY_DEST_NOWHERE
			   & ~LINEDATA_PROPERTY_TIE
			   & ~LINEDATA_PROPERTY_NATURAL_HARMONIC
			   & ~LINEDATA_PROPERTY_CONTINUES
			   & ~LINEDATA_PROPERTY_MUTED);
	ptb_data_uint8(bf, &linedata->transcribe);
	ptb_assert(bf, linedata->transcribe == LINEDATA_TRANSCRIBE_8VA 
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_15MA
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_8VB
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_15MB
			   ||  linedata->transcribe == 0);
			   
	ptb_data_uint8(bf, &linedata->conn_to_next);
	
	if(linedata->conn_to_next) { 
		linedata->bends = malloc_p(struct ptb_bend, 1);
		ptb_data(bf, linedata->bends, 4*linedata->conn_to_next);
	} else {
		linedata->bends = NULL;
	}

	*dest = (struct ptb_list *)linedata;
	return 1;
}


static int handle_CChordText (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_chordtext *chordtext = GET_ITEM(bf, dest, struct ptb_chordtext);

	ptb_data_uint8(bf, &chordtext->offset);
	ptb_data_uint16(bf, chordtext->name);

	ptb_data_uint8(bf, &chordtext->properties);
	ptb_assert_0(bf, chordtext->properties 
			   & ~CHORDTEXT_PROPERTY_NOCHORD
			   & ~CHORDTEXT_PROPERTY_PARENTHESES
			   & ~0x0F /* Formula */
			   & ~0xC0 /*FIXME*/);
	ptb_data_uint8(bf, &chordtext->additions);
	ptb_assert_0(bf, chordtext->additions 
			   & ~CHORDTEXT_EXT_7_9
			   & ~CHORDTEXT_EXT_7_13
			   & ~CHORDTEXT_ADD_2
			   & ~CHORDTEXT_ADD_9
			   & ~CHORDTEXT_ADD_11
			   & ~CHORDTEXT_PLUS_5);
	ptb_data_uint8(bf, &chordtext->alterations);
	ptb_data_uint8(bf, &chordtext->VII);
	ptb_assert_0(bf, chordtext->VII 
				 & ~CHORDTEXT_VII 
				 & ~CHORDTEXT_VII_VI
				 & ~CHORDTEXT_VII_OPEN
				 & ~CHORDTEXT_VII_TYPE_2
				 & ~CHORDTEXT_VII_TYPE_3
				 	);

	*dest = (struct ptb_list *)chordtext;
	return 1;
}

static int handle_CGuitarIn (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_guitarin *guitarin = GET_ITEM(bf, dest, struct ptb_guitarin);

	ptb_data_uint8(bf, &guitarin->section);
	ptb_data_constant(bf, 0x0); 
	ptb_data_uint8(bf, &guitarin->staff);
	ptb_data_uint8(bf, &guitarin->offset);
	ptb_data_uint8(bf, &guitarin->rhythm_slash);
	ptb_data_uint8(bf, &guitarin->staff_in);

	*dest = (struct ptb_list *)guitarin;
	return 1;
}


static int handle_CStaff (struct ptbf *bf, const char *section, struct ptb_list **dest)
{ 
	uint16_t next;
	struct ptb_staff *staff = GET_ITEM(bf, dest, struct ptb_staff);

	ptb_data_uint8(bf, &staff->properties);
	ptb_debug("Properties: %02x", staff->properties);
	ptb_data_uint8(bf, &staff->highest_note_space);
	ptb_data_uint8(bf, &staff->lowest_note_space);
	ptb_data_uint8(bf, &staff->symbol_space);
	ptb_data_uint8(bf, &staff->tab_staff_space);

	/* FIXME! */
	ptb_data_items(bf, "CPosition", (struct ptb_list **)&staff->positions[0]);
	/* This is ugly, but at least it works... */
	if (bf->mode == O_RDONLY) {
		ptb_data_uint16(bf, &next);
		lseek(bf->fd, -2, SEEK_CUR); bf->curpos-=2;
		if(next & 0x8000) {
			*dest = (struct ptb_list *)staff;
			staff->positions[1] = NULL;
			return 1;
		}
	}

	ptb_data_items(bf, "CPosition", (struct ptb_list **)&staff->positions[1]);
	/* This is ugly, but at least it works... */
	if (bf->mode == O_RDONLY) {
		ptb_data_uint16(bf, &next);
		lseek(bf->fd, -2, SEEK_CUR);bf->curpos-=2;
		if(next & 0x8000) {
			*dest = (struct ptb_list *)staff;
			return 1;
		}
	}

	*dest = (struct ptb_list *)staff;
	return 1;
}


static int handle_CPosition (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_position *position = GET_ITEM(bf, dest, struct ptb_position);
	int i;

	ptb_data_uint8(bf, &position->offset);
	ptb_data_uint16(bf, &position->properties); 
	ptb_assert_0(bf, position->properties 
			   & ~POSITION_PROPERTY_IRREGULAR_GROUPING
			   & ~POSITION_PROPERTY_IN_SINGLE_BEAM
			   & ~POSITION_PROPERTY_IN_DOUBLE_BEAM
			   & ~POSITION_PROPERTY_IN_TRIPLE_BEAM
			   & ~POSITION_PROPERTY_FIRST_IN_BEAM
			   & ~POSITION_PROPERTY_PARTIAL_BEAM
			   & ~POSITION_PROPERTY_MIDDLE_IN_BEAM
			   & ~POSITION_PROPERTY_LAST_IN_BEAM);
	ptb_data_uint8(bf, &position->dots);
	ptb_assert_0(bf, position->dots 
				 	&~ POSITION_DOTS_1 
				 	& ~POSITION_DOTS_2 
					& ~POSITION_DOTS_REST 
					& ~POSITION_DOTS_ARPEGGIO_UP 
					& ~POSITION_DOTS_ARPEGGIO_DOWN
					& ~POSITION_DOTS_WIDE_VIBRATO
					& ~POSITION_DOTS_VIBRATO);
	ptb_data_uint8(bf, &position->palm_mute);
	ptb_assert_0(bf, position->palm_mute 
				 & ~POSITION_PALM_MUTE 
				 & ~POSITION_STACCATO 
				 & ~POSITION_ACCENT
				 & ~POSITION_HEAVY_ACCENT
				 & ~POSITION_PICKSTROKE_DOWN
				 & ~POSITION_TREMOLO_PICKING
				 );
	ptb_data_uint8(bf, &position->fermenta);
	ptb_assert_0(bf, position->fermenta
					& ~POSITION_FERMENTA_ACCIACCATURA
					& ~POSITION_FERMENTA_LET_RING
					& ~POSITION_FERMENTA_TRIPLET_FEEL_FIRST
					& ~POSITION_FERMENTA_TRIPLET_FEEL_SECOND
					& ~POSITION_FERMENTA_TRIPLET_1
					& ~POSITION_FERMENTA_TRIPLET_2
					& ~POSITION_FERMENTA_TRIPLET_3
					& ~POSITION_FERMENTA_FERMENTA);
	ptb_data_uint8(bf, &position->length);
	
	ptb_data_uint8(bf, &position->nr_additional_data);

	position->additional = malloc_p(struct ptb_position_additional, position->nr_additional_data);
	
	for (i = 0; i < position->nr_additional_data; i++) {
		ptb_data_uint8(bf, &position->additional[i].start_volume);
		ptb_data_uint8(bf, &position->additional[i].end_volume);
		ptb_data_uint8(bf, &position->additional[i].duration);
		ptb_data_unknown(bf, 1, "FIXME");
	}

	ptb_data_items(bf, "CLineData", (struct ptb_list **)&position->linedatas);

	*dest = (struct ptb_list *)position;
	return 1;
}

static int handle_CDynamic (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_dynamic *dynamic = GET_ITEM(bf, dest, struct ptb_dynamic);

	ptb_data_uint16(bf, &dynamic->section);
	ptb_data_uint8(bf, &dynamic->staff);
	ptb_data_uint8(bf, &dynamic->offset);
	ptb_data_uint16(bf, &dynamic->volume);

	*dest = (struct ptb_list *)dynamic;
	return 1;
}

static int handle_CSectionSymbol (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_sectionsymbol *sectionsymbol = GET_ITEM(bf, dest, struct ptb_sectionsymbol);

	ptb_data_unknown(bf, 5, "FIXME"); /* FIXME */
	ptb_data_uint16(bf, &sectionsymbol->repeat_ending);

	*dest = (struct ptb_list *)sectionsymbol;
	return 1;
}

static int handle_CMusicBar (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_musicbar *musicbar = GET_ITEM(bf, dest, struct ptb_musicbar);
											 
	ptb_data_uint8(bf, &musicbar->offset);
	ptb_data_uint8(bf, &musicbar->properties);
	ptb_data_unknown(bf, 6, "FIXME");
	ptb_data_uint8(bf, &musicbar->letter);
	ptb_data_string(bf, &musicbar->description);

	*dest = (struct ptb_list *)musicbar; 
	return 1;
}

static int handle_CRhythmSlash (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_rhythmslash *rhythmslash = GET_ITEM(bf, dest, struct ptb_rhythmslash);
	
	ptb_data_uint8(bf, &rhythmslash->offset);
	ptb_data_uint8(bf, &rhythmslash->properties);
	ptb_assert_0(bf, rhythmslash->properties 
				 & ~RHYTHMSLASH_PROPERTY_FIRST_IN_BEAM
				 & ~RHYTHMSLASH_PROPERTY_IN_SINGLE_BEAM
				 & ~RHYTHMSLASH_PROPERTY_IN_DOUBLE_BEAM
				 & ~RHYTHMSLASH_PROPERTY_LAST_IN_BEAM
				 & ~RHYTHMSLASH_PROPERTY_PARTIAL_BEAM
				 & ~RHYTHMSLASH_PROPERTY_TRIPLET_FIRST
				 & ~RHYTHMSLASH_PROPERTY_TRIPLET_SECOND
				 & ~RHYTHMSLASH_PROPERTY_TRIPLET_THIRD
				 );
	ptb_data_uint8(bf, &rhythmslash->dotted);
	ptb_data_uint8(bf, &rhythmslash->extra);
	ptb_assert_0(bf, rhythmslash->extra 
				 & ~RHYTHMSLASH_EXTRA_ARPEGGIO_UP
				 & ~RHYTHMSLASH_EXTRA_ACCENT
				 & ~RHYTHMSLASH_EXTRA_HEAVY_ACCENT
				 & ~0x18 /* FIXME */);
	ptb_data_uint8(bf, &rhythmslash->length);
	ptb_data_uint8(bf, &rhythmslash->singlenote);

	*dest = (struct ptb_list *)rhythmslash;
	return 1;
}

static int handle_CDirection (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_direction *direction = GET_ITEM(bf, dest, struct ptb_direction);

	ptb_data_unknown(bf, 1, "FIXME");
	ptb_data_uint8(bf, &direction->nr_items);
	ptb_data_unknown(bf, 2 * direction->nr_items, "FIXME");

	*dest = (struct ptb_list *)direction;
	return 1;
}

struct ptb_section_handler ptb_section_handlers[] = {
	{"CGuitar", handle_CGuitar },
	{"CFloatingText", handle_CFloatingText },
	{"CChordDiagram", handle_CChordDiagram },
	{"CTempoMarker", handle_CTempoMarker},
	{"CLineData", handle_CLineData },
	{"CChordText", handle_CChordText },
	{"CGuitarIn", handle_CGuitarIn },
	{"CStaff", handle_CStaff },
	{"CPosition", handle_CPosition },
	{"CSection", handle_CSection },
	{"CDynamic", handle_CDynamic },
	{"CSectionSymbol", handle_CSectionSymbol },
	{"CMusicBar", handle_CMusicBar },
	{"CRhythmSlash", handle_CRhythmSlash },
	{"CDirection", handle_CDirection },
	{ 0, handle_unknown}
};

const char *ptb_get_tone(ptb_tone id)
{
	const char *chords[] = { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", NULL };
	if((sizeof(chords)/sizeof(chords[0])) < id) return "_UNKNOWN_CHORD_";
	return chords[id];
}

const char *ptb_get_tone_full(ptb_tone id)
{
	const char *chords[] = { "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis", "a", "ais", "b", NULL };
	if(id < 16 || sizeof(chords) < id-16) return "_UNKNOWN_CHORD_";
	return chords[id-16];
}

static struct ptb_position *get_position(struct ptb_staff *staff, int offset)
{
	int i;
	for(i = 0; i < 2; i++) {
		struct ptb_position *p = staff->positions[i];
		while(p) {
			if(p->offset == offset) return p;
			p = p->next;
		}
	}
	return NULL;
}

void ptb_get_position_difference(struct ptb_section *section, int start, int end, int *bars, int *length)
{
	long l = 0;
	struct ptb_staff *staff = section->staffs;
	struct ptb_position *gl = NULL;

	while(staff) { 
		gl = get_position(staff, start);
		if(gl) break;
		staff = staff->next;
	}

	while(gl) {
		if(gl->offset >= end) break;
		l += 0x100 / gl->length;
		gl = gl->next;
	}

	*bars = l / 0x100;

	*length = 0;
	if(l % 0x100) *length = 0x100 / (l % 0x100) ;
}	

static void ptb_free_hdr(struct ptb_hdr *hdr)
{
	if (hdr->classification == CLASSIFICATION_SONG) { 
		switch (hdr->class_info.song.release_type) {
		case RELEASE_TYPE_PR_AUDIO:
			free(hdr->class_info.song.release_info.pr_audio.album_title);
			break;
		case RELEASE_TYPE_PR_VIDEO:
			free(hdr->class_info.song.release_info.pr_video.video_title);
			break;
		case RELEASE_TYPE_BOOTLEG:
			free(hdr->class_info.song.release_info.bootleg.title);
			break;
		default: break;
		}
		free (hdr->class_info.song.title);
		free (hdr->class_info.song.artist);
		free (hdr->class_info.song.words_by);
		free (hdr->class_info.song.music_by);
		free (hdr->class_info.song.arranged_by);
		free (hdr->class_info.song.guitar_transcribed_by);
		free (hdr->class_info.song.lyrics);
		free (hdr->class_info.song.copyright);
	} else if (hdr->classification == CLASSIFICATION_LESSON) {
		free(hdr->class_info.lesson.artist);
		free(hdr->class_info.lesson.title);
		free(hdr->class_info.lesson.author);
		free(hdr->class_info.lesson.copyright);
	}

	free(hdr->guitar_notes);
	free(hdr->bass_notes);
	free(hdr->drum_notes);
}

static void ptb_free_font(struct ptb_font *f)
{
	free(f->family);
}

#define FREE_LIST(ls, em, type) \
{ \
	type tmp; \
	type tmp_next; \
	for (tmp = ls; tmp; tmp = tmp_next) { \
		em; \
		tmp_next = tmp->next; \
		if (tmp->prev) free(tmp); \
	} \
}

static void ptb_free_position(struct ptb_position *pos)
{
	free(pos->additional);
	FREE_LIST(pos->linedatas, free(tmp->bends), struct ptb_linedata *);
}

static void ptb_free_staff(struct ptb_staff *staff)
{
	int i;
	for (i = 0; i < 2; i++) {
		FREE_LIST( staff->positions[i], ptb_free_position(tmp), struct ptb_position *);
	}
}

static void ptb_free_section(struct ptb_section *section)
{
	FREE_LIST(section->staffs, ptb_free_staff(tmp), struct ptb_staff *);
	FREE_LIST(section->chordtexts, {} , struct ptb_chordtext *);
	FREE_LIST(section->rhythmslashes, {}, struct ptb_rhythmslash *);
	FREE_LIST(section->directions, {}, struct ptb_direction *);
	FREE_LIST(section->musicbars, free(tmp->description), struct ptb_musicbar *);
}

void ptb_free(struct ptbf *bf)
{
	int i;
	ptb_free_hdr(&bf->hdr);
	ptb_free_font(&bf->default_font);
	ptb_free_font(&bf->chord_name_font);
	ptb_free_font(&bf->tablature_font);

	free(bf->filename);

	for (i = 0; i < 2; i++) 
	{
		FREE_LIST(
			bf->instrument[i].floatingtexts, 
			free(tmp->text); ptb_free_font(&tmp->font);,
			struct ptb_floatingtext *);
		
		FREE_LIST(
			bf->instrument[i].guitars,
			free(tmp->title); free(tmp->type),
			struct ptb_guitar *);

		FREE_LIST(
			bf->instrument[i].tempomarkers,
			free(tmp->description),
			struct ptb_tempomarker *);

		FREE_LIST(
			bf->instrument[i].chorddiagrams,
			free(tmp->tones),
			struct ptb_chorddiagram *);
			
		FREE_LIST(
			bf->instrument[i].sections,
			free(tmp->description);
			ptb_free_section(tmp),
			struct ptb_section *);

		FREE_LIST(
			bf->instrument[i].sectionsymbols,
		{},
			struct ptb_sectionsymbol *);
	}
	
	free(bf);
}

uint8_t ptb_get_octave(struct ptb_guitar *gtr, uint8_t string, uint8_t fret)
{
	int note = gtr->strings[string] + fret;

	return note / 12;
}

uint8_t ptb_get_step(struct ptb_guitar *gtr, uint8_t string, uint8_t fret)
{
	int note = gtr->strings[string] + fret;

	return note % 12;
}
