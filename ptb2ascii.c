/*
	(c) 2004: Jelmer Vernooij <jelmer@samba.org>

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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "ptb.h"


void ascii_write_header(FILE *out, struct ptbf *ret) 
{
	if(ret->hdr.classification == CLASSIFICATION_SONG) {
		if(ret->hdr.class_info.song.title) 	fprintf(out, "  Title: %s\n", ret->hdr.class_info.song.title);
		if(ret->hdr.class_info.song.music_by) fprintf(out, "  Music By: %s\n", ret->hdr.class_info.song.music_by);
		if(ret->hdr.class_info.song.words_by) fprintf(out, "  Words By: %s\n", ret->hdr.class_info.song.words_by);
		if(ret->hdr.class_info.song.copyright) fprintf(out, "  Copyright: %s\n", ret->hdr.class_info.song.copyright);
		if(ret->hdr.class_info.song.guitar_transcribed_by) fprintf(out, "  Transcribed By: %s\n", ret->hdr.class_info.song.guitar_transcribed_by);
		if(ret->hdr.class_info.song.release_type == RELEASE_TYPE_PR_AUDIO &&
		   ret->hdr.class_info.song.release_info.pr_audio.album_title) fprintf(out, "  Album Title: %s\n", ret->hdr.class_info.song.release_info.pr_audio.album_title);
	} else if(ret->hdr.classification == CLASSIFICATION_LESSON) {
		if(ret->hdr.class_info.lesson.title) 	fprintf(out, "  Title: %s\n", ret->hdr.class_info.lesson.title);
		if(ret->hdr.class_info.lesson.artist) fprintf(out, "  Artist: %s\n", ret->hdr.class_info.lesson.artist);
		if(ret->hdr.class_info.lesson.author) fprintf(out, "  Transcribed By: %s\n", ret->hdr.class_info.lesson.author);
		if(ret->hdr.class_info.lesson.copyright) fprintf(out, "  Copyright: %s\n", ret->hdr.class_info.lesson.copyright);
	}
	fprintf(out, "\n");
}

int ascii_write_position(FILE *out, struct ptb_position *pos, int string)
{
	struct ptb_linedata *d = pos->linedatas;

	while(d) {
		if(string == (int)d->detailed.string) {
			return fprintf(out, "%d", d->detailed.fret);
		} 

		d = d->next;
	}

	return 0;
}


void ascii_write_chordtext(FILE *out, struct ptb_chordtext *name) {
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

	fprintf(out, " ");
}

void ascii_write_staff(FILE *out, struct ptb_staff *s) 
{
	int i;

	for(i = 0; i < 6; i++) {
		int j;
		for(j = 0; j < 2; j++) {
			struct ptb_position *p = s->positions[j];
			while(p) {
				int ret = ascii_write_position(out, p, i);
				for(; ret < 4; ret++) fprintf(out, "-");
				p = p->next;
			}
		}

		fprintf(out, "\n");
	}
}

void ascii_write_section(FILE *out, struct ptb_section *s) 
{
	struct ptb_chordtext *ct = s->chordtexts;
	struct ptb_staff *st = s->staffs;

	if(s->letter != 0x7f) {
		fprintf(out, "%c. %s\n", s->letter, s->description);
	}

	while(ct) {
		ascii_write_chordtext(out, ct);
		ct = ct->next;
	}
	fprintf(out, "\n");
	
	while(st) {
		ascii_write_staff(out, st);
		st = st->next;
		if(st)fprintf(out, "|\n");
	}
}

int ascii_write_lyrics(FILE *out, struct ptbf *ret)
{
	if(ret->hdr.classification != CLASSIFICATION_SONG || !ret->hdr.class_info.song.lyrics) return 0;
	fprintf(out, "\nLyrics:\n");
	fprintf(out, "%s\n\n", ret->hdr.class_info.song.lyrics);
	return 1;
}

int ascii_write_chords(FILE *out, struct ptbf *ret)
{
	/* FIXME:
	 * acc =  \chords {
    % why don't \skip, s4 work?
        c2 c f c
        f c g:7 c
    g f c  g:7 % urg, bug!
        g f c  g:7
    % copy 1-8
        c2 c f c
        f c g:7 c
}
*/

	return 0;
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
			printf("ptb2ascii Version "PACKAGE_VERSION"\n");
			printf("(C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
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
		strcpy(output + baselength, ".txt");
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
	
	fprintf(out, "Generated by ptb2ascii (C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
	fprintf(out, "See https://samba.org/~jelmer/ptabtools/ for more info\n\n");
		
	ascii_write_header(out, ret);
	ascii_write_lyrics(out, ret);

	section = ret->instrument[instrument].sections;
	while(section) {
		ascii_write_section(out, section);
		fprintf(out, "\n\n");
		section = section->next;
	}

	if(output)fclose(out);
	
	return (ret?0:1);
}


