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

#include "ptb.h"
#include <stdio.h>


int handle_unknown (struct ptbf *bf, const char *section) {
	fprintf(stderr, "Unknown section '%s'\n", section);	
    while(!ptb_end_of_section(bf->fd)) {
		lseek(bf->fd, 1, SEEK_CUR);
    }

	return 0; 
}

int handle_CGuitar (struct ptbf *bf, const char *section) {
	char unknown[256];
	struct ptb_guitar *prevguitar = NULL;
	int i;

	for(i = 0; i < bf->hdr.nr_guitars; i++) {
		struct ptb_guitar *guitar = calloc(sizeof(struct ptb_guitar), 1);
	
		if(bf->guitars) prevguitar->next = guitar;
		else bf->guitars = guitar;
		
		/* guitar number */
		read(bf->fd, &guitar->index, 1);
		ptb_read_string(bf->fd, &guitar->title);

		read(bf->fd, &guitar->midi_instrument, 1);
		read(bf->fd, &guitar->initial_volume, 1);
		read(bf->fd, &guitar->pan, 1);
		read(bf->fd, &guitar->reverb, 1);
		read(bf->fd, &guitar->chorus, 1);
		read(bf->fd, &guitar->tremolo, 1);

		read(bf->fd, unknown, 1); /* FIXME */
		read(bf->fd, &guitar->capo, 1);
		
		ptb_read_string(bf->fd, &guitar->type);

		read(bf->fd, &guitar->half_up, 1);
		read(bf->fd, &guitar->nr_strings, 1);
		guitar->strings = malloc(sizeof(guint8) * guitar->nr_strings);
		read(bf->fd, guitar->strings, guitar->nr_strings);
		read(bf->fd, &guitar->rev_gtr, 1);
		read(bf->fd, &guitar->string_12, 1);
		
		prevguitar = guitar;
	}

	/* FIXME: */
	while(!ptb_end_of_section(bf->fd)) {
		read(bf->fd, unknown, 1);
		fprintf(stderr, "x: %02x\n", unknown[0]);
	}

	return 0;
}


int handle_CFloatingText (struct ptbf *bf, const char *section) { 
	struct ptb_floatingtext *prevfloatingtext = NULL;
	char unknown[256];

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_floatingtext *text = calloc(sizeof(struct ptb_floatingtext), 1);

		if(bf->floatingtexts) prevfloatingtext->next = text;
		else bf->floatingtexts = text;

		ptb_read_string(bf->fd, &text->text);

		read(bf->fd, &text->beginpos, 1);

		read(bf->fd, unknown, 15);

		ptb_read_font(bf->fd, &text->font);

		read(bf->fd, unknown, 11);
		
		prevfloatingtext = text;
	}
	return 0; 
}

int handle_CSection (struct ptbf *bf, const char *section) { 
	
	return 0; 
}

int handle_CTempoMarker (struct ptbf *bf, const char *section) {
	struct ptb_tempomarker *prevtempomarker = NULL;
	char unknown[256];

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_tempomarker *tempomarker = calloc(sizeof(struct ptb_tempomarker), 1);

		if(bf->tempomarkers) prevtempomarker->next = tempomarker;
		else bf->tempomarkers = tempomarker;

		read(bf->fd, unknown, 3);
		read(bf->fd, &tempomarker->bpm, 1);
		read(bf->fd, unknown, 3);
		ptb_read_string(bf->fd, &tempomarker->description);
		read(bf->fd, unknown, 2);

		prevtempomarker = tempomarker;
	}
	return 0; 
}


int handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_chorddiagram *prevchorddiagram = NULL;

	while(!ptb_end_of_section(bf->fd)) {
		guint8 last;
		struct ptb_chorddiagram *chorddiagram = calloc(sizeof(struct ptb_chorddiagram), 1);
		int i;

		if(bf->chorddiagrams) prevchorddiagram->next = chorddiagram;
		else bf->chorddiagrams = chorddiagram;

		read(bf->fd, chorddiagram->name, 2);
		
		read(bf->fd, unknown, 3);

		read(bf->fd, &chorddiagram->type, 1);
		read(bf->fd, &chorddiagram->frets, 1);
		read(bf->fd, &chorddiagram->nr_strings, 1);
		chorddiagram->tones = malloc(sizeof(guint8) * chorddiagram->nr_strings);
		for(i = 0; i < chorddiagram->nr_strings; i++) {
			read(bf->fd, &chorddiagram->tones[i], 1);
		}

		read(bf->fd, unknown, 1);

		read(bf->fd, &last, 1);
		if(last == 0) break;
		else if(last == 0x80) {} /* Ok */
		else fprintf(stderr, "Unknown value %02x at end of chord\n", last);

		prevchorddiagram = chorddiagram;
	}

	read(bf->fd, unknown, 2);
	
	return !ptb_end_of_section(bf->fd); 
}

int handle_CLineData (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_linedata *prevlinedata = NULL;

	read(bf->fd, unknown, 1);

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_linedata *linedata = calloc(sizeof(struct ptb_linedata), 1);
		int i;

		if(bf->linedatas) prevlinedata->next = linedata;
		else bf->linedatas = linedata;

		read(bf->fd, unknown, 3);
		read(bf->fd, unknown, 5);

//		fprintf(stderr, "%02x %02x %02x %02x %02x\n", unknown[0], unknown[1], unknown[2], unknown[3], unknown[4]);

		prevlinedata = linedata;
	}

	return 0; 
}


int handle_CChordText (struct ptbf *bf, const char *section) {
	char unknown[256];
	guint8 last;
	struct ptb_chordtext *prevchordtext = NULL;


	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_chordtext *chordtext = calloc(sizeof(struct ptb_chordtext), 1);

		if(bf->chordtexts) prevchordtext->next = chordtext;
		else bf->chordtexts = chordtext;

		read(bf->fd, &chordtext->offset, 1);
		read(bf->fd, chordtext->name, 2);

		read(bf->fd, unknown, 1); /* FIXME */
		read(bf->fd, &chordtext->additions, 1);
		read(bf->fd, &chordtext->alterations, 1);
		
		read(bf->fd, unknown, 2); /* FIXME */

		read(bf->fd, &last, 1);
		if(last == 0x80) {} /* Ok */
		else if(last == 0x0) break;
		else fprintf(stderr, "unknown end of item character: %02x\n", last);

		prevchordtext = chordtext;
	}

	read(bf->fd, unknown, 2);

	return !ptb_end_of_section(bf->fd); 
}

int handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_guitarin *prevguitarin = NULL;

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_guitarin *guitarin = calloc(sizeof(struct ptb_guitarin), 1);

		if(bf->guitarins) prevguitarin->next = guitarin;
		else bf->guitarins = guitarin;

		read(bf->fd, &guitarin->offset, 1);
		read(bf->fd, unknown, 6); /* FIXME */

		read(bf->fd, &last, 1);
		if(last == 0x80) {} /* Ok */
		else if(last == 0x0) break;
		else fprintf(stderr, "unknown end of item character: %02x\n", last);

		prevguitarin = guitarin;
	}

	return !ptb_end_of_section(bf->fd); 
}


int handle_CStaff (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_staff *prevstaff = NULL;

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_staff *staff = calloc(sizeof(struct ptb_staff), 1);

		if(bf->staffs) prevstaff->next = staff;
		else bf->staffs = staff;

		read(bf->fd, &staff->offset, 1);
		read(bf->fd, unknown, 47); /* FIXME */

		read(bf->fd, &last, 1);
		if(last == 0x80) {} /* Ok */
		else if(last == 0x0) break;
		else fprintf(stderr, "unknown end of item character: %02x\n", last);

		prevstaff = staff;
	}

	return !ptb_end_of_section(bf->fd); 
}


int handle_CPosition (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_position *prevposition = NULL;

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_position *position = calloc(sizeof(struct ptb_position), 1);

		if(bf->positions) prevposition->next = position;
		else bf->positions = position;

		read(bf->fd, &position->offset, 1);
		read(bf->fd, unknown, 10); /* FIXME */

		read(bf->fd, &last, 1);
		if(last == 0x80) {} /* Ok */
		else if(last == 0x0) break;
		else fprintf(stderr, "unknown end of item character: %02x\n", last);

		prevposition = position;
	}

	return !ptb_end_of_section(bf->fd); 
}

int handle_CDynamic (struct ptbf *bf, const char *section) { return 0; }
int handle_CSectionSymbol (struct ptbf *bf, const char *section) { return 0; }
int handle_CMusicBar (struct ptbf *bf, const char *section) { return 0; }
int handle_CRhythmSlash (struct ptbf *bf, const char *section) { return 0; }
int handle_CDirection (struct ptbf *bf, const char *section) { return 0; }

struct ptb_section default_sections[] = {
	{"CGuitar", handle_CGuitar },
	{"CFloatingText", handle_CFloatingText },
	{"CChordDiagram", handle_CChordDiagram },
	{"CTempoMarker", handle_CTempoMarker},
	{"CLineData", handle_CLineData },
	{"CChordText", handle_CChordText },
	{"CGuitarIn", handle_CGuitarIn },
	{"CStaff", handle_CStaff },
	{"CPosition", handle_CPosition },
/*	{"CSection", handle_CSection },
	{"CDynamic", handle_CDynamic },
	{"CSectionSymbol", handle_CSectionSymbol },
	{"CMusicBar", handle_CMusicBar },
	{"CRhythmSlash", handle_CRhythmSlash },
	{"CDirection", handle_CDirection },*/
	{ 0, handle_unknown}
};
