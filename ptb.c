/*
   Parser library for the PowerTab (PTB) file format
   (c) 2004-2005: Jelmer Vernooij <jelmer@samba.org>

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

#define ptb_error printf

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

static ssize_t ptb_data_constant(struct ptbf *f, unsigned char expected) 
{
	unsigned char real;
	ssize_t ret;
	
	if (f->mode == O_WRONLY) {
		ret = ptb_data(f, &expected, 1);
	} else {
			ret = ptb_data(f, &real, 1);
	
		if(real != expected) {
			ptb_error("%04lx: Expected %02x, got %02x", f->curpos-1, expected, real);
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

static ssize_t ptb_data_unknown(struct ptbf *f, size_t length) {
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
	ptb_data(f, &shortlength, 1);

	/* If length is 0xff, this is followed by a uint16_t length */
	if(shortlength == 0xff) {
		if(ptb_data(f, &length, 2) < 2) return -1;
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
	
	ptb_data(f, &shortlength, 1);

	/* If length is 0xff, this is followed by a uint16_t length */
	if(shortlength == 0xff) {
		if(ptb_data(f, &length, 2) < 2) return -1;
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

static ssize_t ptb_data_font(struct ptbf *f, struct ptb_font *dest) {
	int ret = 0;
	ret+=ptb_data_string(f, &dest->family);
	ret+=ptb_data(f, &dest->size, 1);
	ret+=ptb_data_unknown(f, 5);
	ret+=ptb_data(f, &dest->thickness, 1);
	ret+=ptb_data_unknown(f, 2);
	ret+=ptb_data(f, &dest->italic, 1);
	ret+=ptb_data(f, &dest->underlined, 1);
	ret+=ptb_data_unknown(f, 4);
	return ret;
}

static ssize_t ptb_data_header(struct ptbf *f, struct ptb_hdr *hdr)
{
	ptb_data_constant_string(f, "ptab", 4);

	ptb_data(f, &hdr->version, 2);
	ptb_data(f, &hdr->classification, 1);

	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
		ptb_data_unknown(f, 1); /* FIXME */
		ptb_data_string(f, &hdr->class_info.song.title);
		ptb_data_string(f, &hdr->class_info.song.artist);

		ptb_data(f, &hdr->class_info.song.release_type, 1);

		switch(hdr->class_info.song.release_type) {
		case RELEASE_TYPE_PR_AUDIO:
			ptb_data(f, &hdr->class_info.song.release_info.pr_audio.type, 1);
			ptb_data_string(f, &hdr->class_info.song.release_info.pr_audio.album_title);
			ptb_data(f, &hdr->class_info.song.release_info.pr_audio.year, 2);
			ptb_data(f, &hdr->class_info.song.release_info.pr_audio.is_live_recording, 1);
			break;
		case RELEASE_TYPE_PR_VIDEO:
			ptb_data_string(f, &hdr->class_info.song.release_info.pr_video.video_title);
			ptb_data(f, &hdr->class_info.song.release_info.pr_video.is_live_recording, 1);
			break;
		case RELEASE_TYPE_BOOTLEG:
			ptb_data_string(f, &hdr->class_info.song.release_info.bootleg.title);
			ptb_data(f, &hdr->class_info.song.release_info.bootleg.day, 2);
			ptb_data(f, &hdr->class_info.song.release_info.bootleg.month, 2);
			ptb_data(f, &hdr->class_info.song.release_info.bootleg.year, 2);
			break;
		case RELEASE_TYPE_UNRELEASED:
			break;

		default:
			fprintf(stderr, "Unknown release type: %d\n", hdr->class_info.song.release_type);
			break;
		}

		ptb_data(f, &hdr->class_info.song.is_original_author_unknown, 1);
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
		ptb_data(f, &hdr->class_info.lesson.style, 2);
		ptb_data(f, &hdr->class_info.lesson.level, 1);
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

	ret+=ptb_data(bf, &nr_items, 2);	
	if(ret == 0 || nr_items == 0x0) return 1; 
	ret+=ptb_data(bf, &header, 2);

	ptb_debug("Going to read %d items", nr_items);

	if(header == 0xffff) { /* New section */
		char *my_section_name;
		/* Read Section */
		ret+=ptb_data(bf, &unknownval, 2);

		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return 0;
		}

		ret+=ptb_data(bf, &length, 2);

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
			ret+=ptb_data(bf, &next_thing, 2);
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

	ret+=ptb_data(bf, &nr_items, 2);	
	if(nr_items == 0x0) return 1; 

	header = generate_header_id(bf, assumed_type);
	if (header == 0) header = 0xffff;

	ret+=ptb_data(bf, &header, 2);

	ptb_debug("Going to write %d items", nr_items);

	if (header == 0xffff) {
		uint16_t unknownval = 0x0001; /* FIXME */

		ret+=ptb_data(bf, &unknownval, 2);
		length = strlen(assumed_type);
		ret+=ptb_data(bf, &length, 2);
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
			ret+=ptb_data(bf, &id, 2);
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

	ptb_data_unknown(bf, 12);
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

	ptb_data(bf, &guitar->index, 1);
	ptb_data_string(bf, &guitar->title);

	ptb_data(bf, &guitar->midi_instrument, 1);
	ptb_data(bf, &guitar->initial_volume, 1);
	ptb_data(bf, &guitar->pan, 1);
	ptb_data(bf, &guitar->reverb, 1);
	ptb_data(bf, &guitar->chorus, 1);
	ptb_data(bf, &guitar->tremolo, 1);

	ptb_data(bf, &guitar->simulate, 1);
	ptb_data(bf, &guitar->capo, 1);
		
	ptb_data_string(bf, &guitar->type);

	ptb_data(bf, &guitar->half_up, 1);
	ptb_data(bf, &guitar->nr_strings, 1);

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
	ptb_data(bf, &text->beginpos, 1);
	ptb_data_unknown(bf, 15);
	ptb_data(bf, &text->alignment, 1);
	ptb_debug("Align: %x", text->alignment);
	ptb_assert(bf, (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_LEFT || 
			   	   (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_CENTER || 
				   (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_RIGHT);
	ptb_data_font(bf, &text->font);
	
	*dest = (struct ptb_list *)text;

	return 1;
}

static int handle_CSection (struct ptbf *bf, const char *sectionname, struct ptb_list **dest) { 
	struct ptb_section *section = GET_ITEM(bf, dest, struct ptb_section);

	ptb_data_constant(bf, 0x32);
	ptb_data_unknown(bf, 11);
	ptb_data(bf, &section->properties, 2);
	ptb_data_unknown(bf, 2);
	ptb_data(bf, &section->end_mark, 1);
	ptb_assert(bf, (section->end_mark &~ END_MARK_TYPE_NORMAL 
			   & ~END_MARK_TYPE_DOUBLELINE
			   & ~END_MARK_TYPE_REPEAT) < 24);
	ptb_data(bf, &section->position_width, 1);
	ptb_data_unknown(bf, 5);
	ptb_data(bf, &section->key_extra, 1);
	ptb_data_unknown(bf, 1);
	ptb_data(bf, &section->meter_type, 2);
	ptb_data(bf, &section->beat_info, 1);
	ptb_data(bf, &section->metronome_pulses_per_measure, 1);
	ptb_data(bf, &section->letter, 1);
	ptb_data_string(bf, &section->description);

	ptb_data_items(bf, "CDirection", (struct ptb_list **)&section->directions);
	ptb_data_items(bf, "CChordText", (struct ptb_list **)&section->chordtexts);
	ptb_data_items(bf, "CRhythmSlash", (struct ptb_list **)&section->rhythmslashes);
	ptb_data_items(bf, "CStaff", (struct ptb_list **)&section->staffs);
	ptb_data_items(bf, "CMusicBar", (struct ptb_list **)&section->musicbars);

	*dest = (struct ptb_list *)section;
	return 1;
}

static int handle_CTempoMarker (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_tempomarker *tempomarker = GET_ITEM(bf, dest, struct ptb_tempomarker);

	ptb_data(bf, &tempomarker->section, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &tempomarker->offset, 1);
	ptb_data(bf, &tempomarker->bpm, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &tempomarker->type, 2);
	ptb_data_string(bf, &tempomarker->description);

	*dest = (struct ptb_list *)tempomarker;
	return 1;
}


static int handle_CChordDiagram (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_chorddiagram *chorddiagram = GET_ITEM(bf, dest, struct ptb_chorddiagram);

	ptb_data(bf, chorddiagram->name, 2);
	ptb_data_unknown(bf, 3);
	ptb_data(bf, &chorddiagram->type, 1);
	ptb_data(bf, &chorddiagram->frets, 1);
	ptb_data(bf, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = malloc_p(uint8_t, chorddiagram->nr_strings);
	ptb_data(bf, chorddiagram->tones, chorddiagram->nr_strings);

	*dest = (struct ptb_list *)chorddiagram;
	return 1;
}

static int handle_CLineData (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_linedata *linedata = GET_ITEM(bf, dest, struct ptb_linedata);

	ptb_data(bf, &linedata->tone, 1);
	ptb_data(bf, &linedata->properties, 1);
	ptb_assert_0(bf, linedata->properties
			   & ~LINEDATA_PROPERTY_GHOST_NOTE
			   & ~LINEDATA_PROPERTY_PULLOFF_FROM
			   & ~LINEDATA_PROPERTY_HAMMERON_FROM
			   & ~LINEDATA_PROPERTY_DEST_NOWHERE
			   & ~LINEDATA_PROPERTY_TIE
			   & ~LINEDATA_PROPERTY_NATURAL_HARMONIC
			   & ~LINEDATA_PROPERTY_MUTED);
	ptb_data(bf, &linedata->transcribe, 1);
	ptb_assert(bf, linedata->transcribe == LINEDATA_TRANSCRIBE_8VA 
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_15MA
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_8VB
			   ||  linedata->transcribe == LINEDATA_TRANSCRIBE_15MB
			   ||  linedata->transcribe == 0);
			   
	ptb_data(bf, &linedata->conn_to_next, 1);
	
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

	ptb_data(bf, &chordtext->offset, 1);
	ptb_data(bf, chordtext->name, 2);

	ptb_data(bf, &chordtext->properties, 1);
	ptb_assert_0(bf, chordtext->properties 
			   & ~CHORDTEXT_PROPERTY_NOCHORD
			   & ~CHORDTEXT_PROPERTY_PARENTHESES
			   & ~0xc0 /*FIXME*/
			   & ~CHORDTEXT_PROPERTY_FORMULA_M
			   & ~CHORDTEXT_PROPERTY_FORMULA_MAJ7);
	ptb_data(bf, &chordtext->additions, 1);
	ptb_assert_0(bf, chordtext->additions 
			   & ~CHORDTEXT_ADD_9);
	ptb_data(bf, &chordtext->alterations, 1);
	ptb_data(bf, &chordtext->VII, 1);
	ptb_assert_0(bf, chordtext->VII & ~CHORDTEXT_VII);

	*dest = (struct ptb_list *)chordtext;
	return 1;
}

static int handle_CGuitarIn (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_guitarin *guitarin = GET_ITEM(bf, dest, struct ptb_guitarin);

	ptb_data(bf, &guitarin->section, 1);
	ptb_data_constant(bf, 0x0); /* FIXME */
	ptb_data(bf, &guitarin->staff, 1);
	ptb_data(bf, &guitarin->offset, 1);
	ptb_data(bf, &guitarin->rhythm_slash, 1);
	ptb_data(bf, &guitarin->staff_in, 1);

	*dest = (struct ptb_list *)guitarin;
	return 1;
}


static int handle_CStaff (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	uint16_t next;
	struct ptb_staff *staff = GET_ITEM(bf, dest, struct ptb_staff);

	ptb_data(bf, &staff->properties, 1);
	ptb_debug("Properties: %02x", staff->properties);
	ptb_data(bf, &staff->highest_note, 1);
	ptb_data(bf, &staff->lowest_note, 1);
	ptb_data_unknown(bf, 2);

	/* FIXME! */
	ptb_data_items(bf, "CPosition", (struct ptb_list **)&staff->positions[0]);
	/* This is ugly, but at least it works... */
	if (bf->mode == O_RDONLY) {
		ptb_data(bf, &next, 2);
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
		ptb_data(bf, &next, 2);
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

	ptb_data(bf, &position->offset, 1);
	ptb_data(bf, &position->properties, 2); 
	ptb_assert_0(bf, position->properties 
			   & ~POSITION_PROPERTY_IN_SINGLE_BEAM
			   & ~POSITION_PROPERTY_IN_DOUBLE_BEAM
			   & ~POSITION_PROPERTY_IN_TRIPLE_BEAM
			   & ~POSITION_PROPERTY_FIRST_IN_BEAM
			   & ~POSITION_PROPERTY_MIDDLE_IN_BEAM
			   & ~POSITION_PROPERTY_LAST_IN_BEAM);
	ptb_data(bf, &position->dots, 1);
	ptb_assert_0(bf, position->dots &~ POSITION_DOTS_1 
				 	 & ~POSITION_DOTS_2 & ~POSITION_DOTS_REST & ~POSITION_DOTS_ARPEGGIO_UP);
	ptb_data(bf, &position->palm_mute, 1);
	ptb_assert_0(bf, position->palm_mute & ~POSITION_PALM_MUTE & ~POSITION_STACCATO & ~POSITION_ACCENT);
	ptb_data(bf, &position->fermenta, 1);
	ptb_assert_0(bf, position->fermenta
					& ~POSITION_FERMENTA_LET_RING
					& ~POSITION_FERMENTA_TRIPLET_1
					& ~POSITION_FERMENTA_TRIPLET_2
					& ~POSITION_FERMENTA_TRIPLET_3
					& ~POSITION_FERMENTA_FERMENTA);
	ptb_data(bf, &position->length, 1);
	
	ptb_data(bf, &position->nr_additional_data, 1);

	position->additional = malloc_p(struct ptb_position_additional, position->nr_additional_data);
	
	for (i = 0; i < position->nr_additional_data; i++) {
		ptb_data(bf, &position->additional[i].start_volume, 1);
		ptb_data(bf, &position->additional[i].end_volume, 1);
		ptb_data(bf, &position->additional[i].duration, 1);
		ptb_data_unknown(bf, 1);
	}

	ptb_data_items(bf, "CLineData", (struct ptb_list **)&position->linedatas);

	*dest = (struct ptb_list *)position;
	return 1;
}

static int handle_CDynamic (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_dynamic *dynamic = GET_ITEM(bf, dest, struct ptb_dynamic);

	ptb_data(bf, &dynamic->offset, 1);
	ptb_data(bf, &dynamic->staff, 1);
	ptb_data_unknown(bf, 3); /* FIXME */
	ptb_data(bf, &dynamic->volume, 1);

	*dest = (struct ptb_list *)dynamic;
	return 1;
}

static int handle_CSectionSymbol (struct ptbf *bf, const char *section, struct ptb_list **dest) {
	struct ptb_sectionsymbol *sectionsymbol = GET_ITEM(bf, dest, struct ptb_sectionsymbol);

	ptb_data_unknown(bf, 5); /* FIXME */
	ptb_data(bf, &sectionsymbol->repeat_ending, 2);

	*dest = (struct ptb_list *)sectionsymbol;
	return 1;
}

static int handle_CMusicBar (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_musicbar *musicbar = GET_ITEM(bf, dest, struct ptb_musicbar);
											 
	ptb_data(bf, &musicbar->offset, 1);
	ptb_data(bf, &musicbar->properties, 1);
	ptb_data_unknown(bf, 6);
	ptb_data(bf, &musicbar->letter, 1);
	ptb_data_string(bf, &musicbar->description);

	*dest = (struct ptb_list *)musicbar; 
	return 1;
}

static int handle_CRhythmSlash (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_rhythmslash *rhythmslash = GET_ITEM(bf, dest, struct ptb_rhythmslash);
	
	ptb_data(bf, &rhythmslash->offset, 1);
	ptb_data(bf, &rhythmslash->properties, 1);
	ptb_assert_0(bf, rhythmslash->properties & ~RHYTHMSLASH_PROPERTY_FIRST_IN_BEAM);
	ptb_data(bf, &rhythmslash->dotted, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &rhythmslash->length, 1);
	ptb_data_constant(bf, 0);

	*dest = (struct ptb_list *)rhythmslash;
	return 1;
}

static int handle_CDirection (struct ptbf *bf, const char *section, struct ptb_list **dest) { 
	struct ptb_direction *direction = GET_ITEM(bf, dest, struct ptb_direction);

	ptb_data_unknown(bf, 1);
	ptb_data(bf, &direction->nr_items, 1);
	ptb_data_unknown(bf, 2 * direction->nr_items); /* FIXME */

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
