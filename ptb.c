/*
   Parser library for the PowerTab (PTB) file format
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
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
typedef int ssize_t;
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#define PTB_CORE
#include "ptb.h"

int assert_is_fatal = 0;

#define ptb_assert(ptb, expr) \
	if (!(expr)) { ptb_debug("---------------------------------------------"); \
		ptb_debug("file: %s, line: %d (%s): assertion failed: %s. Current position: 0x%lx", __FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION, #expr, ptb->curpos); \
		if(assert_is_fatal) abort(); \
	}

#define ptb_assert_0(ptb, expr) \
	if(expr) ptb_debug("%s == 0x%x!", #expr, expr); \
/*	ptb_assert(ptb, (expr) == 0); */

struct ptb_section_handler {
	char *name;
	void *(*handler) (struct ptbf *, const char *section);
};

extern struct ptb_section_handler ptb_section_handlers[];

void ptb_debug(const char *fmt, ...);

int debugging = 0;

static ssize_t ptb_read(struct ptbf *f, void *data, size_t length)
{
	ssize_t ret = read(f->fd, data, length);
#define read DONT_USE_READ

	if(ret == -1) { 
		perror("read"); 
		ptb_assert(f, 0);
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
	default: ptb_assert(f, 0);
	}
	return 0;
}

static ssize_t ptb_data_constant(struct ptbf *f, unsigned char expected) 
{
	unsigned char real;
	ssize_t ret = ptb_data(f, &real, 1);
	
	if(real != expected) {
		ptb_debug("%04x: Expected %02x, got %02x", f->curpos-1, expected, real);
		ptb_assert(f, 0);
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

static ssize_t ptb_read_unknown(struct ptbf *f, size_t length) {
	char unknown[255];
	ssize_t ret;
	off_t oldpos = f->curpos;
	size_t i;

	ret = ptb_data(f, unknown, length);
	if(debugging) {
		for(i = 0; i < length; i++) 
			ptb_debug("Unknown[%04lx]: %02x", oldpos + i, unknown[i]);
	}
	return ret;
}

static ssize_t ptb_data_string(struct ptbf *f, char **dest) {
	guint8 shortlength;
	guint16 length;
	char *data;
	ptb_data(f, &shortlength, 1);

	/* If length is 0xff, this is followed by a guint16 length */
	if(shortlength == 0xff) {
		if(ptb_data(f, &length, 2) < 2) return -1;
	} else {
		length = shortlength;
	}

	if(length) {
		data = g_new0(char, length+1);
		if(ptb_data(f, data, length) < length) return -1;
		ptb_debug("Read string: %s", data);
		*dest = data;
	} else {
		ptb_debug("Empty string");
		*dest = NULL;
	}

	return length;
}

static ssize_t ptb_data_font(struct ptbf *f, struct ptb_font *dest) {
	int ret = 0;
	ret+=ptb_data_string(f, &dest->family);
	ret+=ptb_data(f, &dest->size, 1);
	ret+=ptb_read_unknown(f, 5);
	ret+=ptb_data(f, &dest->thickness, 1);
	ret+=ptb_read_unknown(f, 2);
	ret+=ptb_data(f, &dest->italic, 1);
	ret+=ptb_data(f, &dest->underlined, 1);
	ret+=ptb_read_unknown(f, 4);
	return ret;
}

static ssize_t ptb_data_header(struct ptbf *f, struct ptb_hdr *hdr)
{
	ptb_data_constant_string(f, "ptab", 4);

	ptb_data(f, &hdr->version, 2);
	ptb_data(f, &hdr->classification, 1);

	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
		ptb_read_unknown(f, 1); /* FIXME */
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

static gboolean ptb_read_items(struct ptbf *bf, const char *assumed_type, GList **result) {
	int i;
	guint16 unknownval;
	guint16 l;
	guint16 length;
	guint16 header;
	guint16 nr_items;
	int ret = 0;

	*result = NULL;

	ret+=ptb_data(bf, &nr_items, 2);	
	if(ret == 0 || nr_items == 0x0) return TRUE; 
	ret+=ptb_data(bf, &header, 2);

	ptb_debug("Going to read %d items", nr_items);

	if(header == 0xffff) { /* New section */
		gchar *my_section_name;
		/* Read Section */
		ret+=ptb_data(bf, &unknownval, 2);

		if(unknownval != 0x0001) {
			fprintf(stderr, "Unknownval: %04x\n", unknownval);
			return FALSE;
		}

		ret+=ptb_data(bf, &length, 2);

		my_section_name = g_new0(char, length + 1);
		ret+=ptb_data(bf, my_section_name, length);

	} else if(header & 0x8000) {
	} else { 
		ptb_debug("Expected new item type (%s), got %04x %02x\n", assumed_type, nr_items, header);
		ptb_assert(bf, 0);
		return FALSE;
	}

	for(i = 0; ptb_section_handlers[i].name; i++) {
		if(!strcmp(ptb_section_handlers[i].name, assumed_type)) {
			break;
		}
	}

	if(!ptb_section_handlers[i].handler) {
		fprintf(stderr, "Unable to find handler for section %s\n", assumed_type);
		return FALSE;
	}

	for(l = 0; l < nr_items; l++) {
		void *tmp;
		guint16 next_thing;

		ptb_debug("%04x ============= Handling %s (%d of %d) =============", bf->curpos, assumed_type, l+1, nr_items);
		debug_level++;
		tmp = ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name);
		debug_level--;

		ptb_debug("%04x ============= END Handling %s (%d of %d) =============", bf->curpos, ptb_section_handlers[i].name, l+1, nr_items);

		if(!tmp) {
			fprintf(stderr, "Error parsing section '%s'\n", ptb_section_handlers[i].name);
		}

		*result = g_list_append(*result, tmp);
		
		if(l < nr_items - 1) {
			ret+=ptb_data(bf, &next_thing, 2);
			if(!(next_thing & 0x8000)) {
				ptb_debug("Warning: got %04x, expected | 0x8000\n", next_thing);
				ptb_assert(bf, 0);
			}
		}
	}

	return TRUE;
}

static gboolean ptb_write_items(struct ptbf *bf, const char *assumed_type, GList **result) 
{
	int i;
	guint16 l;
	guint16 length;
	guint16 header;
	guint16 nr_items;
	int ret = 0;

	*result = NULL;

	nr_items = g_list_length(*result);

	ret+=ptb_data(bf, &nr_items, 2);	
	if(ret == 0 || nr_items == 0x0) return TRUE; 
	ret+=ptb_data(bf, &header, 2);

	ptb_debug("Going to read %d items", nr_items);

	if(header == 0xffff) { /* New section */
		char *section_type = g_strdup(assumed_type);
		guint16 unknownval = 0x0001; /* FIXME */

		ret+=ptb_data(bf, &unknownval, 2);

		length = strlen(assumed_type);
		ret+=ptb_data(bf, &length, 2);
		ret+=ptb_data(bf, section_type, length);
		g_free(section_type);

	} else if(header & 0x8000) {
	}

	for(i = 0; ptb_section_handlers[i].name; i++) {
		if(!strcmp(ptb_section_handlers[i].name, assumed_type)) {
			break;
		}
	}

	if(!ptb_section_handlers[i].handler) {
		fprintf(stderr, "Unable to find handler for section %s\n", assumed_type);
		return FALSE;
	}

	for(l = 0; l < nr_items; l++) {
		debug_level++;
		ptb_section_handlers[i].handler(bf, ptb_section_handlers[i].name);
		debug_level--;

		if(l < nr_items - 1) {
			guint16 next_thing;
			next_thing = 0x8000 | 0; /* FIXME */
			ret+=ptb_data(bf, &next_thing, 2);
		}
	}

	return TRUE;

}

static gboolean ptb_data_items(struct ptbf *bf, const char *assumed_type, GList **result)
{
	switch (bf->mode) {
	case O_RDONLY: return ptb_read_items(bf, assumed_type, result);
	case O_WRONLY: return ptb_write_items(bf, assumed_type, result);
	default: g_assert(0);
	}
	return FALSE;
}



void ptb_set_debug(int level) { debugging = level; }

void ptb_set_asserts_fatal(int y) { assert_is_fatal = y; }

static void ptb_data_instrument(struct ptbf *bf, int i)
{
	ptb_data_items(bf, "CGuitar", &bf->instrument[i].guitars);
	ptb_data_items(bf, "CChordDiagram", &bf->instrument[i].chorddiagrams);
	ptb_data_items(bf, "CFloatingText", &bf->instrument[i].floatingtexts);
	ptb_data_items(bf, "CGuitarIn", &bf->instrument[i].guitarins);
	ptb_data_items(bf, "CTempoMarker", &bf->instrument[i].tempomarkers);
	ptb_data_items(bf, "CDynamic", &bf->instrument[i].dynamics);
	ptb_data_items(bf, "CSectionSymbol", &bf->instrument[i].sectionsymbols);
	ptb_data_items(bf, "CSection", &bf->instrument[i].sections);
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

	ptb_read_unknown(bf, 12);
	ptb_data_constant(bf, 0);
	return 0;
}

struct ptbf *ptb_read_file(const char *file)
{
	struct ptbf *bf = g_new0(struct ptbf, 1);
	bf->mode = O_RDONLY;
	bf->fd = open(file, bf->mode);

	strncpy(bf->data, "abc", 3);

	bf->filename = g_strdup(file);

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
	bf->fd = open(file, bf->mode | O_CREAT);

	strncpy(bf->data, "abc", 3);

	bf->filename = g_strdup(file);

	if(bf->fd < 0) return -1;

	bf->curpos = 1;

	if (ptb_data_file(bf) == -1) 
		return -1;

	close(bf->fd);
	return 0;
}

static void *handle_unknown (struct ptbf *bf, const char *section) {
	fprintf(stderr, "Unknown section '%s'\n", section);	
	return NULL; 
}

static void *handle_CGuitar (struct ptbf *bf, const char *section) {
	struct ptb_guitar *guitar = g_new0(struct ptb_guitar, 1);

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
	guitar->strings = g_new(guint8, guitar->nr_strings);
	ptb_data(bf, guitar->strings, guitar->nr_strings);

	return guitar;
}


static void *handle_CFloatingText (struct ptbf *bf, const char *section) { 
	struct ptb_floatingtext *text = g_new0(struct ptb_floatingtext, 1);

	ptb_data_string(bf, &text->text);
	ptb_data(bf, &text->beginpos, 1);
	ptb_read_unknown(bf, 15);
	ptb_data(bf, &text->alignment, 1);
	ptb_debug("Align: %x", text->alignment);
	ptb_assert(bf, (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_LEFT || 
			   	   (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_CENTER || 
				   (text->alignment &~ ALIGN_TIMESTAMP) == ALIGN_RIGHT);
	ptb_data_font(bf, &text->font);
	
	return text;
}

static void *handle_CSection (struct ptbf *bf, const char *sectionname) { 
	struct ptb_section *section = g_new0(struct ptb_section, 1);

	ptb_data_constant(bf, 0x32);
	ptb_read_unknown(bf, 11);
	ptb_data(bf, &section->properties, 2);
	ptb_read_unknown(bf, 2);
	ptb_data(bf, &section->end_mark, 1);
	ptb_assert(bf, (section->end_mark &~ END_MARK_TYPE_NORMAL 
			   & ~END_MARK_TYPE_DOUBLELINE
			   & ~END_MARK_TYPE_REPEAT) < 24);
	ptb_data(bf, &section->position_width, 1);
	ptb_read_unknown(bf, 5);
	ptb_data(bf, &section->key_extra, 1);
	ptb_read_unknown(bf, 1);
	ptb_data(bf, &section->meter_type, 2);
	ptb_data(bf, &section->beat_info, 1);
	ptb_data(bf, &section->metronome_pulses_per_measure, 1);
	ptb_data(bf, &section->letter, 1);
	ptb_data_string(bf, &section->description);

	ptb_data_items(bf, "CDirection", &section->directions);
	ptb_data_items(bf, "CChordText", &section->chordtexts);
	ptb_data_items(bf, "CRhythmSlash", &section->rhythmslashes);
	ptb_data_items(bf, "CStaff", &section->staffs);

	return section; 
}

static void *handle_CTempoMarker (struct ptbf *bf, const char *section) {
	struct ptb_tempomarker *tempomarker = g_new0(struct ptb_tempomarker, 1);

	ptb_data(bf, &tempomarker->section, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &tempomarker->offset, 1);
	ptb_data(bf, &tempomarker->bpm, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &tempomarker->type, 2);
	ptb_data_string(bf, &tempomarker->description);

	return tempomarker;
}


static void *handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	struct ptb_chorddiagram *chorddiagram = g_new0(struct ptb_chorddiagram, 1);

	ptb_data(bf, chorddiagram->name, 2);
	ptb_read_unknown(bf, 3);
	ptb_data(bf, &chorddiagram->type, 1);
	ptb_data(bf, &chorddiagram->frets, 1);
	ptb_data(bf, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = g_new(guint8, chorddiagram->nr_strings);
	ptb_data(bf, chorddiagram->tones, chorddiagram->nr_strings);

	return chorddiagram;
}

static void *handle_CLineData (struct ptbf *bf, const char *section) { 
	struct ptb_linedata *linedata = g_new0(struct ptb_linedata, 1);

	ptb_data(bf, &linedata->tone, 1);
	ptb_data(bf, &linedata->properties, 1);
	ptb_assert_0(bf, linedata->properties
			   & ~LINEDATA_PROPERTY_GHOST_NOTE
			   & ~LINEDATA_PROPERTY_PULLOFF_FROM
			   & ~LINEDATA_PROPERTY_HAMMERON_FROM
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
		linedata->bends = calloc(linedata->conn_to_next, sizeof(struct bend));
		ptb_data(bf, &linedata->bends, 4*linedata->conn_to_next);
	}

	return linedata;
}


static void *handle_CChordText (struct ptbf *bf, const char *section) {
	struct ptb_chordtext *chordtext = g_new0(struct ptb_chordtext, 1);

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

	return chordtext;
}

static void *handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	struct ptb_guitarin *guitarin = g_new0(struct ptb_guitarin, 1);

	ptb_data(bf, &guitarin->section, 1);
	ptb_data_constant(bf, 0x0); /* FIXME */
	ptb_data(bf, &guitarin->staff, 1);
	ptb_data(bf, &guitarin->offset, 1);
	ptb_data(bf, &guitarin->rhythm_slash, 1);
	ptb_data(bf, &guitarin->staff_in, 1);

	return guitarin;
}


static void *handle_CStaff (struct ptbf *bf, const char *section) { 
	guint16 next;
	guint8 datasize;
	struct ptb_staff *staff = g_new0(struct ptb_staff, 1);

	ptb_data(bf, &staff->properties, 1);
	ptb_debug("Properties: %02x", staff->properties);
	ptb_data(bf, &staff->highest_note, 1);
	ptb_data(bf, &staff->lowest_note, 1);
	ptb_data(bf, &datasize, 1);
	ptb_debug("Datasize: %d", datasize);
	ptb_read_unknown(bf, 1);

	staff->positions[1] = NULL;
	/* FIXME! */
	ptb_data_items(bf, "CPosition", &staff->positions[0]);
	/* This is ugly, but at least it works... */
	{
		ptb_data(bf, &next, 2);
		lseek(bf->fd, -2, SEEK_CUR); bf->curpos-=2;
		if(next & 0x8000) return staff;
	}

	ptb_data_items(bf, "CPosition", &staff->positions[1]);
	/* This is ugly, but at least it works... */
	{
		ptb_data(bf, &next, 2);
		lseek(bf->fd, -2, SEEK_CUR);bf->curpos-=2;
		if(next & 0x8000) return staff;
	}

	ptb_data_items(bf, "CMusicBar", &staff->musicbars);

	return staff;
}


static void *handle_CPosition (struct ptbf *bf, const char *section) { 
	struct ptb_position *position = g_new0(struct ptb_position, 1);

	ptb_data(bf, &position->offset, 1);
	ptb_data(bf, &position->properties, 2); 
	ptb_assert_0(bf, position->properties 
			   & ~POSITION_PROPERTY_IN_SINGLE_BEAM
			   & ~POSITION_PROPERTY_IN_DOUBLE_BEAM
			   & ~POSITION_PROPERTY_FIRST_IN_BEAM
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
	
	ptb_data(bf, &position->conn_to_next, 1);
	
	/* FIXME 
	if(position->conn_to_next) { 
		ptb_debug("Conn to next!: %02x", position->conn_to_next);
		ptb_read_unknown(bf, 4*position->conn_to_next);
	}*/
	if(position->conn_to_next) {
		guint8 covers, start, end;
		ptb_debug("Conn to next: %02x", position->conn_to_next);
		ptb_data(bf, &start, 1);
		ptb_data(bf, &end, 1);
		ptb_data(bf, &covers, 1);
		ptb_read_unknown(bf, 1);
	}

	ptb_data_items(bf, "CLineData", &position->linedatas);

	return position;
}

static void *handle_CDynamic (struct ptbf *bf, const char *section) { 
	struct ptb_dynamic *dynamic = g_new0(struct ptb_dynamic, 1);

	ptb_data(bf, &dynamic->offset, 1);
	ptb_data(bf, &dynamic->staff, 1);
	ptb_read_unknown(bf, 3); /* FIXME */
	ptb_data(bf, &dynamic->volume, 1);

	return dynamic;
}

static void *handle_CSectionSymbol (struct ptbf *bf, const char *section) {
	struct ptb_sectionsymbol *sectionsymbol = g_new0(struct ptb_sectionsymbol, 1);

	ptb_read_unknown(bf, 5); /* FIXME */
	ptb_data(bf, &sectionsymbol->repeat_ending, 2);

	return sectionsymbol;
}

static void *handle_CMusicBar (struct ptbf *bf, const char *section) { 
	struct ptb_musicbar *musicbar = g_new0(struct ptb_musicbar, 1);

	ptb_read_unknown(bf, 8); /* FIXME */
	ptb_data(bf, &musicbar->letter, 1);
	ptb_data_string(bf, &musicbar->description);

	return musicbar; 
}

static void *handle_CRhythmSlash (struct ptbf *bf, const char *section) { 
	struct ptb_rhythmslash *rhythmslash = g_new0(struct ptb_rhythmslash, 1);
	
	ptb_data(bf, &rhythmslash->offset, 1);
	ptb_data(bf, &rhythmslash->properties, 1);
	ptb_assert_0(bf, rhythmslash->properties & ~RHYTHMSLASH_PROPERTY_FIRST_IN_BEAM);
	ptb_data(bf, &rhythmslash->dotted, 1);
	ptb_data_constant(bf, 0);
	ptb_data(bf, &rhythmslash->length, 1);
	ptb_data_constant(bf, 0);

	return rhythmslash;
}

static void *handle_CDirection (struct ptbf *bf, const char *section) { 
	struct ptb_direction *direction = g_new0(struct ptb_direction, 1);

	ptb_read_unknown(bf, 4); /* FIXME */

	return direction;
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

static GList *get_position(struct ptb_staff *staff, int offset)
{
	int i;
	for(i = 0; i < 2; i++) {
		GList *gl = staff->positions[i];
		while(gl) {
			struct ptb_position *p = gl->data;
			if(p->offset == offset) return gl;
			gl = gl->next;
		}
	}
	return NULL;
}

void ptb_get_position_difference(struct ptb_section *section, int start, int end, int *bars, int *length)
{
	long l = 0;
	GList *staff = section->staffs;
	GList *gl = NULL;

	while(staff) { 
		gl = get_position(staff->data, start);
		if(gl) break;
		staff = staff->next;
	}

	while(gl) {
		struct ptb_position *p = gl->data;
		if(p->offset >= end) break;
		l += 0x100 / p->length;
		gl = gl->next;
	}

	*bars = l / 0x100;

	*length = 0;
	if(l % 0x100) *length = 0x100 / (l % 0x100) ;
}	
