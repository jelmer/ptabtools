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
	return -1; 
}

int handle_CGuitar (struct ptbf *bf, const char *section) {
	char unknown[256];
	int i;
	struct ptb_guitar *guitar = calloc(sizeof(struct ptb_guitar), 1);


	bf->guitars = g_list_append(bf->guitars, guitar);
		
	read(bf->fd, &guitar->index, 1);
	ptb_read_string(bf->fd, &guitar->title);

	read(bf->fd, &guitar->midi_instrument, 1);
	read(bf->fd, &guitar->initial_volume, 1);
	read(bf->fd, &guitar->pan, 1);
	read(bf->fd, &guitar->reverb, 1);
	read(bf->fd, &guitar->chorus, 1);
	read(bf->fd, &guitar->tremolo, 1);

	read(bf->fd, &guitar->simulate, 1);
	read(bf->fd, &guitar->capo, 1);
		
	ptb_read_string(bf->fd, &guitar->type);

	read(bf->fd, &guitar->half_up, 1);
	read(bf->fd, &guitar->nr_strings, 1);
	guitar->strings = malloc(sizeof(guint8) * guitar->nr_strings);
	read(bf->fd, guitar->strings, guitar->nr_strings);
	
	return 0;
}


int handle_CFloatingText (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_floatingtext *text = calloc(sizeof(struct ptb_floatingtext), 1);

	bf->floatingtexts = g_list_append(bf->floatingtexts, text);

	ptb_read_string(bf->fd, &text->text);
	read(bf->fd, &text->beginpos, 1);
	read(bf->fd, unknown, 15);
	ptb_read_font(bf->fd, &text->font);
	read(bf->fd, unknown, 4);
	
	return 0; 
}

int handle_CSection (struct ptbf *bf, const char *sectionname) { 
	char unknown[256];
	struct ptb_section *section = calloc(sizeof(struct ptb_section), 1);

	bf->sections = g_list_append(bf->sections, section);

	read(bf->fd, unknown, 29);
	read(bf->fd, &section->letter, 1);
	ptb_read_string(bf->fd, &section->description);

	while(ptb_read_items(bf, default_section_handlers) > 0);

	return 0; 
}

int handle_CTempoMarker (struct ptbf *bf, const char *section) {
	char unknown[256];
	struct ptb_tempomarker *tempomarker = calloc(sizeof(struct ptb_tempomarker), 1);

	bf->tempomarkers = g_list_append(bf->tempomarkers, tempomarker);

	read(bf->fd, unknown, 3);
	read(bf->fd, &tempomarker->bpm, 1);
	read(bf->fd, unknown, 3);
	ptb_read_string(bf->fd, &tempomarker->description);

	return 0; 
}


int handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_chorddiagram *chorddiagram = calloc(sizeof(struct ptb_chorddiagram), 1);
	int i;

	bf->chorddiagrams = g_list_append(bf->chorddiagrams, chorddiagram);

	read(bf->fd, chorddiagram->name, 2);
		
	read(bf->fd, unknown, 3);

	read(bf->fd, &chorddiagram->type, 1);
	read(bf->fd, &chorddiagram->frets, 1);
	read(bf->fd, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = malloc(sizeof(guint8) * chorddiagram->nr_strings);
	read(bf->fd, chorddiagram->tones, chorddiagram->nr_strings);

	return 0;
}

int handle_CLineData (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_linedata *linedata = calloc(sizeof(struct ptb_linedata), 1);
	int i;

	bf->linedatas = g_list_append(bf->linedatas, linedata);

	read(bf->fd, &linedata->tone, 1);
	read(bf->fd, unknown, 5);

//		fprintf(stderr, "%02x %02x %02x %02x %02x\n", unknown[0], unknown[1], unknown[2], unknown[3], unknown[4]);

	return 0; 
}


int handle_CChordText (struct ptbf *bf, const char *section) {
	char unknown[256];
	guint8 last;
	struct ptb_chordtext *chordtext = calloc(sizeof(struct ptb_chordtext), 1);

	bf->chordtexts = g_list_append(bf->chordtexts, chordtext);

	read(bf->fd, &chordtext->offset, 1);
	read(bf->fd, chordtext->name, 2);

	read(bf->fd, unknown, 1); /* FIXME */
	read(bf->fd, &chordtext->additions, 1);
	read(bf->fd, &chordtext->alterations, 1);
	read(bf->fd, unknown, 1); /* FIXME */

	return 0;
}

int handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_guitarin *guitarin = calloc(sizeof(struct ptb_guitarin), 1);

	bf->guitarins = g_list_append(bf->guitarins, guitarin);

	read(bf->fd, &guitarin->offset, 1);
	read(bf->fd, unknown, 5); /* FIXME */

	return 0;
}


int handle_CStaff (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_staff *staff = calloc(sizeof(struct ptb_staff), 1);

	bf->staffs = g_list_append(bf->staffs, staff);

	read(bf->fd, &staff->offset, 1);
	read(bf->fd, unknown, 4); /* FIXME */

	return 0;
}


int handle_CPosition (struct ptbf *bf, const char *section) { 
	char unknown[256];
	guint8 last;
	struct ptb_position *position = calloc(sizeof(struct ptb_position), 1);

	bf->positions = g_list_append(bf->positions, position);

	read(bf->fd, &position->offset, 1);
	read(bf->fd, &position->length, 1);
	read(bf->fd, unknown, 6); /* FIXME */
	while(ptb_read_items(bf, default_section_handlers) > 0);

	return 0;
}

int handle_CDynamic (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_dynamic *dynamic = calloc(sizeof(struct ptb_dynamic), 1);

	bf->dynamics = g_list_append(bf->dynamics, dynamic);

	read(bf->fd, &dynamic->offset, 1);
	read(bf->fd, unknown, 5); /* FIXME */

	return 0; 

}
int handle_CSectionSymbol (struct ptbf *bf, const char *section) {
	char unknown[256];
	struct ptb_sectionsymbol *sectionsymbol = calloc(sizeof(struct ptb_sectionsymbol), 1);

	bf->sectionsymbols = g_list_append(bf->sectionsymbols, sectionsymbol);

	read(bf->fd, unknown, 7); /* FIXME */

	return 0; 
}

int handle_CMusicBar (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_musicbar *musicbar = calloc(sizeof(struct ptb_musicbar), 1);

	bf->musicbars = g_list_append(bf->musicbars, musicbar);

	read(bf->fd, unknown, 10); /* FIXME */

	return 0; 
}

int handle_CRhythmSlash (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_rhythmslash *rhythmslash = calloc(sizeof(struct ptb_rhythmslash), 1);

	bf->rhythmslashs = g_list_append(bf->rhythmslashs, rhythmslash);

	read(bf->fd, unknown, 6); /* FIXME */

	return 0; 
}

int handle_CDirection (struct ptbf *bf, const char *section) { 
	char unknown[256];
	struct ptb_direction *direction = calloc(sizeof(struct ptb_direction), 1);

	bf->directions = g_list_append(bf->directions, direction);

	read(bf->fd, unknown, 4); /* FIXME */

	return 0; 
}

struct ptb_section_handler default_section_handlers[] = {
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
