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
	
	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_guitar *guitar = calloc(sizeof(struct ptb_guitar), 1);
		int i;
	
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

		read(bf->fd, unknown, 1);
		read(bf->fd, &guitar->capo, 1);
		
		ptb_read_string(bf->fd, &guitar->type);

		read(bf->fd, &guitar->half_up, 1);
		read(bf->fd, &guitar->nr_strings, 1);
		guitar->strings = malloc(sizeof(guint8) * guitar->nr_strings);
		for(i = 0; i < guitar->nr_strings; i++)
			read(bf->fd, &guitar->strings[i], 1);
		read(bf->fd, &guitar->rev_gtr, 1);
		read(bf->fd, &guitar->string_12, 1);
		
		prevguitar = guitar;
	}

	return 0;
}


int handle_CFloatingText (struct ptbf *bf, const char *section) { 
	struct ptb_floatingtext *prevfloatingtext = NULL;
	char unknown[256];

	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_floatingtext *text = calloc(sizeof(struct ptb_floatingtext), 1);

		if(bf->floating_texts) prevfloatingtext->next = text;
		else bf->floating_texts = text;

		ptb_read_string(bf->fd, &text->text);

		read(bf->fd, unknown, 17);

		ptb_read_font(bf->fd, &text->font);

		read(bf->fd, unknown, 16);
		
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
		struct ptb_chorddiagram *chorddiagram = calloc(sizeof(struct ptb_chorddiagram), 1);
		int i;

		if(bf->chorddiagrams) prevchorddiagram->next = chorddiagram;
		else bf->chorddiagrams = chorddiagram;

		read(bf->fd, &chorddiagram->name, 1);
		
		/* FIXME: This appears to be the same value twice ? */
		read(bf->fd, &chorddiagram->name, 1);
		
		read(bf->fd, unknown, 3);

		read(bf->fd, &chorddiagram->type, 1);
		read(bf->fd, &chorddiagram->frets, 1);
		read(bf->fd, &chorddiagram->nr_strings, 1);
		chorddiagram->tones = malloc(sizeof(guint8) * chorddiagram->nr_strings);
		for(i = 0; i < chorddiagram->nr_strings; i++) {
			read(bf->fd, &chorddiagram->tones[i], 1);
		}

		read(bf->fd, unknown, 2);

		prevchorddiagram = chorddiagram;
	}

	
	return 0; 
}

int handle_CGuitarIn (struct ptbf *bf, const char *section) { return 0; }
int handle_CDynamic (struct ptbf *bf, const char *section) { return 0; }
int handle_CSectionSymbol (struct ptbf *bf, const char *section) { return 0; }
int handle_CChordText (struct ptbf *bf, const char *section) { return 0; }
int handle_CStaff (struct ptbf *bf, const char *section) { return 0; }
int handle_CPosition (struct ptbf *bf, const char *section) { return 0; }
int handle_CLineData (struct ptbf *bf, const char *section) { return 0; }
int handle_CMusicBar (struct ptbf *bf, const char *section) { return 0; }
int handle_CRhythmSlash (struct ptbf *bf, const char *section) { return 0; }
int handle_CDirection (struct ptbf *bf, const char *section) { return 0; }

struct ptb_section default_sections[] = {
	{"CGuitar", handle_CGuitar },
	{"CFloatingText", handle_CFloatingText },
	{"CChordDiagram", handle_CChordDiagram },
	{"CTempoMarker", handle_CTempoMarker},
/*	{"CGuitarIn", handle_CGuitarIn },
	{"CSection", handle_CSection },
	{"CDynamic", handle_CDynamic },
	{"CSectionSymbol", handle_CSectionSymbol },
	{"CChordText", handle_CChordText },
	{"CStaff", handle_CStaff },
	{"CPosition", handle_CPosition },
	{"CLineData", handle_CLineData },
	{"CMusicBar", handle_CMusicBar },
	{"CRhythmSlash", handle_CRhythmSlash },
	{"CDirection", handle_CDirection },*/
	{ 0, handle_unknown}
};
