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
#include <errno.h>
#include <popt.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "gp.h"

#define LILYPOND_VERSION "2.4"

static void foutnum(FILE *out, int id)
{
	do {
		fprintf(out, "%c", 'A' + (id % 26)); id/=26;
	} while (id > 0);
}

static void ly_write_header(FILE *out, struct gpf *gpf)
{
	int i;
	fprintf(out, "\\header {\n");
	fprintf(out, "\ttitle = \"%s\"\n", gpf->title);
	fprintf(out, "\tartist = \"%s\"\n", gpf->artist);
	fprintf(out, "\talbum = \"%s\"\n", gpf->album);
	fprintf(out, "\tsubtitle = \"%s\"\n", gpf->subtitle);
	fprintf(out, "\tenteredby = \"%s\"\n", gpf->tab_by);
	fprintf(out, "\tinstruction = \"%s\"\n", gpf->instruction);
	fprintf(out, "\tcomposer = \"%s\"\n", gpf->author);
	fprintf(out, "\tcopyright = \"%s\"\n", gpf->copyright);

	for (i = 0; i < gpf->notice_num_lines; i++)
	{
		fprintf(out, "\t%% %s\n", gpf->notice[i]);
	}

	fprintf(out, "}\n\n");
}



static void ly_write_lyrics(FILE *out, struct gpf *gpf)
{
	int i;
	for (i = 0; i < gpf->num_lyrics; i++)
	{
		if (!gpf->lyrics[i].data || !strlen(gpf->lyrics[i].data)) continue;
		fprintf(out, "lyrics"); 
		foutnum(out, gpf->lyrics[i].bar); 
		fprintf(out, " = \\lyrics {\n");
		fprintf(out, "\t%s\n", gpf->lyrics[i].data);
		fprintf(out, "}\n");
	}
}

static void ly_write_beat(FILE *out, struct gp_beat *b)
{
	if (b->properties & GP_BEAT_PROPERTY_TEXT) {
		fprintf(out, "^\\markup { %s } ", b->text);
	}

	if (b->properties & GP_BEAT_PROPERTY_DOTTED) {
		/* FIXME */
	}

	if (b->properties & GP_BEAT_PROPERTY_CHORD) {
		/* FIXME */
	}

	if (b->properties & GP_BEAT_PROPERTY_EFFECT) {
		/* FIXME */
	}

	if (b->properties & GP_BEAT_PROPERTY_CHANGE) {
		/* FIXME */
	}

	if (b->properties & GP_BEAT_PROPERTY_TUPLET) {
		/* FIXME */
	}

	if (b->properties & GP_BEAT_PROPERTY_REST) {
		fprintf(out, "r%d", b->duration);
		/* FIXME */
	} else {
		int j;
		fprintf(out, " <");
		for (j = 0; j < 7; j++) { /* FIXME: s/7/num_strings? */
			fprintf(out, "%d-%d ", b->notes[j].value, b->notes[j].duration);
		}
		fprintf(out, "> ");
	}
}

static void ly_write_track_bars(FILE *out, struct gpf *ret, int i)
{
	int j,k;
	
	for (j = 0; j < ret->num_bars; j++) 
	{
		fprintf(out, "Track"); 
		foutnum(out, i); fprintf(out, "x");
		foutnum(out, j); fprintf(out, " = {\n");
		
		for (k = 0; k < ret->bars[j].tracks[i].num_beats; k++)
		{
			ly_write_beat(out, &ret->bars[j].tracks[i].beats[k]);
		}

		fprintf(out, "}\n");
	}
}

static void ly_write_tracks(FILE *out, struct gpf *ret)
{
	int i, j;
	for (i = 0; i < ret->num_tracks; i++) {
		struct gp_track *t = &ret->tracks[i];
		fprintf(out, "%% Track %d: %s\n", i, t->name);
		fprintf(out, "%% %d frets, %d strings\n", t->num_frets, t->num_strings);

		ly_write_track_bars(out, ret, i);

		fprintf(out, "Track"); foutnum(out, i);
		fprintf(out, " = \\context StaffGroup <<\n");
		fprintf(out, "\t\\context Staff { \n");
		for (j = 0; j < ret->num_bars; j++) {
			fprintf(out, "\t\t\\Track");
			foutnum(out, i);
			fprintf(out, "Bar");
			foutnum(out, j);
			fprintf(out, "\n");
		}
		fprintf(out, "\t}\n");
		fprintf(out, "\t\\context TabStaff { \n");
		for (j = 0; j < ret->num_bars; j++) {
			fprintf(out, "\t\t\\Track");
			foutnum(out, i);
			fprintf(out, "Bar");
			foutnum(out, j);
			fprintf(out, "\n");
		}
		fprintf(out, "\t}\n");
		fprintf(out, ">>\n\n");
	}
}

static void ly_write_bars(FILE *out, struct gpf *ret)
{
	int i;
	
	for (i = 0; i < ret->num_tracks; i++) 
	{
		fprintf(out, "Bar%c = { }\n", 'A' + i);
	}
}

int main(int argc, const char **argv) 
{
	FILE *out;
	struct gpf *ret;
	int c;
	int version = 0;
	int quiet = 0;
	int i;
	const char *input;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"quiet", 'q', POPT_ARG_NONE, &quiet, 1, "Be quiet (no output to stderr)" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("gp2ly Version "PACKAGE_VERSION"\n");
			printf("(C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
			exit(0);
			break;
		}
	}
			
	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}
	input = poptGetArg(pc);
	
	if (!quiet) fprintf(stderr, "Parsing %s... \n", input);
					
	ret = gp_read_file(input);
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	if(!output) {
		int baselength = strlen(input);
		if (!strncmp(input + strlen(input) - 4, ".gp", 3)) {
			baselength -= 4;
		}
		output = malloc(baselength + 5);
		strncpy(output, input, baselength);
		strcpy(output + baselength, ".ly");
	}

	if (!quiet) fprintf(stderr, "Generating lilypond file in %s...\n", output);

	if (!strcmp(output, "-")) {
		out = stdout; 
	} else {
		out = fopen(output, "w+");
		if(!out) {
			perror("open");
			return -1;
		}
	}

	fprintf(out, "%% Generated by gp2ly (C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
	fprintf(out, "%% See http://jelmer.vernstok.nl/oss/ptabtools/ for more info\n\n");
	
	fprintf(out, "\\version \""LILYPOND_VERSION"\"\n");

	ly_write_header(out, ret);

	ly_write_lyrics(out, ret);

	ly_write_bars(out, ret);

	ly_write_tracks(out, ret);

	fprintf(out, "\n\\score { << \n");

	fprintf(out, "\t\\context ChordNames {\n");
	fprintf(out, "\t}\n");

	for (i = 0; i < ret->num_tracks; i++) {
		fprintf(out, "\t\\Track%c\n", 'A' + i);
	}

	fprintf(out, "\t>>\n");
	
	fprintf(out, "\t\\layout { }\n");
	fprintf(out, "\t\\midi { }\n");
	fprintf(out, "} \n");

	if(output)fclose(out);
	
	return (ret?0:1);
}
