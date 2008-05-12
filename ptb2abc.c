/*
	(c) 2005-2006 Jelmer Vernooij <jelmer@samba.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
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
#include <errno.h>
#include <popt.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "ptb.h"

void abc_write_header(FILE *out, struct ptbf *ret) 
{
	if(ret->hdr.classification == CLASSIFICATION_SONG) {
		if(ret->hdr.class_info.song.title) 	fprintf(out, "T: %s\n", ret->hdr.class_info.song.title);
		if(ret->hdr.class_info.song.music_by) fprintf(out, "C: %s\n", ret->hdr.class_info.song.music_by);
		if(ret->hdr.class_info.song.words_by) fprintf(out, "%%  Words By: %s\n", ret->hdr.class_info.song.words_by);
		if(ret->hdr.class_info.song.copyright) fprintf(out, "%%  Copyright: %s\n", ret->hdr.class_info.song.copyright);
		if(ret->hdr.class_info.song.guitar_transcribed_by) fprintf(out, "Z: %s\n", ret->hdr.class_info.song.guitar_transcribed_by);
		if(ret->hdr.class_info.song.release_type == RELEASE_TYPE_PR_AUDIO &&
		   ret->hdr.class_info.song.release_info.pr_audio.album_title) fprintf(out, "%%  Album Title: %s\n", ret->hdr.class_info.song.release_info.pr_audio.album_title);
	} else if(ret->hdr.classification == CLASSIFICATION_LESSON) {
		if(ret->hdr.class_info.lesson.title) 	fprintf(out, "T:%s\n", ret->hdr.class_info.lesson.title);
		if(ret->hdr.class_info.lesson.artist) fprintf(out, "C: %s\n", ret->hdr.class_info.lesson.artist);
		if(ret->hdr.class_info.lesson.author) fprintf(out, "Z: %s\n", ret->hdr.class_info.lesson.author);
		if(ret->hdr.class_info.lesson.copyright) fprintf(out, "%%  Copyright: %s\n", ret->hdr.class_info.lesson.copyright);
	}
	fprintf(out, "\n");
}

static char *note_names[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };

void abc_write_linedata(FILE *out, struct ptb_guitar *gtr, struct ptb_linedata *ld)
{
	uint8_t octave = ptb_get_octave(gtr, ld->detailed.string, ld->detailed.fret);
	uint8_t step = ptb_get_step(gtr, ld->detailed.string, ld->detailed.fret);
	int i;
	char n[3];

	strcpy(n, note_names[step]);

	if (octave < 4) n[0] = toupper(n[0]);

	fprintf(out, "%s", n);

	for (i = octave; i < 4; i++) fprintf(out, ",");
	for (i = 5; i < octave; i++) fprintf(out, "'");
}

void abc_write_position(FILE *out, struct ptb_guitar *gtr, struct ptb_position *ps)
{
	struct ptb_linedata *ld;

	for (ld = ps->linedatas; ld; ld = ld->next) {
		abc_write_linedata(out, gtr, ld);
	}
}

void abc_write_staff(FILE *out, struct ptb_guitar *gtr, struct ptb_staff *staff)
{
	struct ptb_position *ps;
	
	for (ps = staff->positions[0]; ps; ps = ps->next) {
		abc_write_position(out, gtr, ps);
	}

	fprintf(out, "\n");
}

void abc_write_section(FILE *out, struct ptb_guitar *gtr, struct ptb_section *sec)
{
	struct ptb_staff *st;
	
	for (st = sec->staffs; st; st = st->next) {
		abc_write_staff(out, gtr, st);
	}
}

void abc_write_chordtext(FILE *out, struct ptb_chordtext *name) {
	fprintf(out, "\"");

	if(name->properties & CHORDTEXT_PROPERTY_NOCHORD) {
		fprintf(out, "N.C.");
	}

	if(name->properties & CHORDTEXT_PROPERTY_PARENTHESES) {
		fprintf(out, "(");
	}

	if(!(name->properties & CHORDTEXT_PROPERTY_NOCHORD) | 
	   (name->properties & CHORDTEXT_PROPERTY_PARENTHESES)) {
		if(name->name[0] == name->name[1]) {
			fprintf(out, "%s", ptb_get_tone(name->name[0]));
		} else { 
			fprintf(out, "%s/%s", ptb_get_tone(name->name[0]),
						ptb_get_tone(name->name[1]));
		}
	}

	if(name->properties & CHORDTEXT_PROPERTY_PARENTHESES) {
		fprintf(out, ")");
	}

	fprintf(out, "\" ");
}

int abc_write_lyrics(FILE *out, struct ptbf *ret)
{
	if(ret->hdr.classification != CLASSIFICATION_SONG || !ret->hdr.class_info.song.lyrics) return 0;
	fprintf(out, "W: %s\n\n", ret->hdr.class_info.song.lyrics);
	return 1;
}

int main(int argc, const char **argv) 
{
	FILE *out;
	struct ptbf *ret;
	int debugging = 0;
	struct ptb_section *section;
	int instrument = 0;
	int c;
	int version = 0;
	const char *input;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"regular", 'r', POPT_ARG_NONE, &instrument, 0, "Write tabs for regular guitar" },
		{"bass", 'b', POPT_ARG_NONE, &instrument, 1, "Write tabs for bass guitar"},
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2abc Version "PACKAGE_VERSION"\n");
			printf("(C) 2005-2006 Jelmer Vernooij <jelmer@samba.org>\n");
			exit(0);
			break;
		}
	}
			
	ptb_set_debug(debugging);
	
	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}
	input = poptGetArg(pc);
	ret = ptb_read_file(input);
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	if(!output) {
		int baselength = strlen(input);
		if (!strcmp(input + strlen(input) - 4, ".ptb")) {
			baselength -= 4;
		}
		output = malloc(baselength + 6);
		strncpy(output, input, baselength);
		strcpy(output + baselength, ".abc");
	}

	if(!strcmp(output, "-")) {
		out = stdout;
	} else {
		out = fopen(output, "w+");
		if(!out) {
			perror("open");
			return -1;
		}
	} 
	
	fprintf(out, "%% Generated by ptb2abc (C) 2005-2006 Jelmer Vernooij <jelmer@samba.org>\n");
	fprintf(out, "%% See http://jelmer.vernstok.nl/oss/ptabtools/ for more info\n\n");
	fprintf(out, "X:1\n");
		
	abc_write_header(out, ret);
	abc_write_lyrics(out, ret);

	fprintf(out, "M: C\n");
	fprintf(out, "K: Cm\n");
	fprintf(out, "L: 1/4\n");

	section = ret->instrument[instrument].sections;
	while(section) {
		abc_write_section(out, ret->instrument[instrument].guitars, section);
		fprintf(out, "\n\n");
		section = section->next;
	}

	if(output)fclose(out);
	
	return (ret?0:1);
}
