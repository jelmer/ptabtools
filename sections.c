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
	int i;
	int ret = 0;
	struct ptb_guitar *guitar = calloc(sizeof(struct ptb_guitar), 1);


	bf->guitars = g_list_append(bf->guitars, guitar);
		
	ret+=ptb_read(bf->fd, &guitar->index, 1);
	ret+=ptb_read_string(bf->fd, &guitar->title);

	ret+=ptb_read(bf->fd, &guitar->midi_instrument, 1);
	ret+=ptb_read(bf->fd, &guitar->initial_volume, 1);
	ret+=ptb_read(bf->fd, &guitar->pan, 1);
	ret+=ptb_read(bf->fd, &guitar->reverb, 1);
	ret+=ptb_read(bf->fd, &guitar->chorus, 1);
	ret+=ptb_read(bf->fd, &guitar->tremolo, 1);

	ret+=ptb_read(bf->fd, &guitar->simulate, 1);
	ret+=ptb_read(bf->fd, &guitar->capo, 1);
		
	ret+=ptb_read_string(bf->fd, &guitar->type);

	ret+=ptb_read(bf->fd, &guitar->half_up, 1);
	ret+=ptb_read(bf->fd, &guitar->nr_strings, 1);
	guitar->strings = malloc(sizeof(guint8) * guitar->nr_strings);
	ret+=ptb_read(bf->fd, guitar->strings, guitar->nr_strings);
	
	return ret;
}


int handle_CFloatingText (struct ptbf *bf, const char *section) { 
	int ret = 0;
	struct ptb_floatingtext *text = calloc(sizeof(struct ptb_floatingtext), 1);

	bf->floatingtexts = g_list_append(bf->floatingtexts, text);

	ret+=ptb_read_string(bf->fd, &text->text);
	ret+=ptb_read(bf->fd, &text->beginpos, 1);
	ret+=ptb_read_unknown(bf->fd, 15);
	ret+=ptb_read_font(bf->fd, &text->font);
	ret+=ptb_read_unknown(bf->fd, 4);
	
	return ret;
}

int handle_CSection (struct ptbf *bf, const char *sectionname) { 
	int ret = 0;
	int cur = 0;
	struct ptb_section *section = calloc(sizeof(struct ptb_section), 1);

	bf->sections = g_list_append(bf->sections, section);

	ret+=ptb_read_unknown(bf->fd, 12);
	ret+=ptb_read(bf->fd, &section->child_size, 1);
	ret+=ptb_read_unknown(bf->fd, 3);
	ret+=ptb_read(bf->fd, &section->end_mark, 1);
	ret+=ptb_read_unknown(bf->fd, 6);
	ret+=ptb_read(bf->fd, &section->key_extra, 1);
	ret+=ptb_read_unknown(bf->fd, 1);
	ret+=ptb_read(bf->fd, &section->meter_type, 2);
	ret+=ptb_read(bf->fd, &section->beat_value, 1);
	ret+=ptb_read(bf->fd, &section->metronome_pulses_per_measure, 1);
	ret+=ptb_read(bf->fd, &section->letter, 1);
	ret+=ptb_read_string(bf->fd, &section->description);

	ret+=ptb_read_unknown(bf->fd, 2);

	while(cur < section->child_size) { cur+=ptb_read_items(bf, default_section_handlers); }

	g_assert(cur == section->child_size);

	return ret; 
}

int handle_CTempoMarker (struct ptbf *bf, const char *section) {
	int ret = 0;
	struct ptb_tempomarker *tempomarker = calloc(sizeof(struct ptb_tempomarker), 1);

	bf->tempomarkers = g_list_append(bf->tempomarkers, tempomarker);

	ret+=ptb_read_unknown(bf->fd, 3);
	ret+=ptb_read(bf->fd, &tempomarker->bpm, 1);
	ret+=ptb_read_unknown(bf->fd, 1);
	ret+=ptb_read(bf->fd, &tempomarker->type, 2);
	ret+=ptb_read_string(bf->fd, &tempomarker->description);

	return ret;
}


int handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	guint8 last;
	int ret = 0;
	struct ptb_chorddiagram *chorddiagram = calloc(sizeof(struct ptb_chorddiagram), 1);
	int i;

	bf->chorddiagrams = g_list_append(bf->chorddiagrams, chorddiagram);

	ret+=ptb_read(bf->fd, chorddiagram->name, 2);
	ret+=ptb_read_unknown(bf->fd, 3);
	ret+=ptb_read(bf->fd, &chorddiagram->type, 1);
	ret+=ptb_read(bf->fd, &chorddiagram->frets, 1);
	ret+=ptb_read(bf->fd, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = malloc(sizeof(guint8) * chorddiagram->nr_strings);
	ret+=ptb_read(bf->fd, chorddiagram->tones, chorddiagram->nr_strings);

	return ret;
}

int handle_CLineData (struct ptbf *bf, const char *section) { 
	struct ptb_linedata *linedata = calloc(sizeof(struct ptb_linedata), 1);
	int i;
	int ret = 0;

	bf->linedatas = g_list_append(bf->linedatas, linedata);

	ret+=ptb_read(bf->fd, &linedata->tone, 1);
	ret+=ptb_read(bf->fd, &linedata->properties, 1);
	ret+=ptb_read(bf->fd, &linedata->transcribe, 1);
	ret+=ptb_read_unknown(bf->fd, 1);

	return ret;
}


int handle_CChordText (struct ptbf *bf, const char *section) {
	guint8 last;
	int ret = 0;
	struct ptb_chordtext *chordtext = calloc(sizeof(struct ptb_chordtext), 1);

	bf->chordtexts = g_list_append(bf->chordtexts, chordtext);

	ret+=ptb_read(bf->fd, &chordtext->offset, 1);
	ret+=ptb_read(bf->fd, chordtext->name, 2);

	ret+=ptb_read_unknown(bf->fd, 1); /* FIXME */
	ret+=ptb_read(bf->fd, &chordtext->additions, 1);
	ret+=ptb_read(bf->fd, &chordtext->alterations, 1);
	ret+=ptb_read_unknown(bf->fd, 1); /* FIXME */

	return ret;
}

int handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	guint8 last;
	int ret = 0;
	struct ptb_guitarin *guitarin = calloc(sizeof(struct ptb_guitarin), 1);

	bf->guitarins = g_list_append(bf->guitarins, guitarin);

	ret+=ptb_read(bf->fd, &guitarin->offset, 1);
	ret+=ptb_read_unknown(bf->fd, 5); /* FIXME */

	return ret;
}


int handle_CStaff (struct ptbf *bf, const char *section) { 
	guint8 last;
	int ret = 0;
	int cur = 0;
	struct ptb_staff *staff = calloc(sizeof(struct ptb_staff), 1);

	bf->staffs = g_list_append(bf->staffs, staff);

	ret+=ptb_read(bf->fd, &staff->properties, 1);
	ret+=ptb_read(bf->fd, &staff->child_size, 1);
	ret+=ptb_read_unknown(bf->fd, 2); /* FIXME */
	ret+=ptb_read(bf->fd, &staff->extra_data, 1);

//	while(cur < staff->child_size) { cur+=ptb_read_items(bf, default_section_handlers); }

//	printf("Expect: %d, got: %d\n", staff->child_size, cur);
//	g_assert(cur == staff->child_size);

	return ret;
}


int handle_CPosition (struct ptbf *bf, const char *section) { 
	guint8 last;
	int ret = 0;
	struct ptb_position *position = calloc(sizeof(struct ptb_position), 1);

	bf->positions = g_list_append(bf->positions, position);

	ret+=ptb_read(bf->fd, &position->offset, 1);
	ret+=ptb_read(bf->fd, &position->properties, 2); /* FIXME */
	ret+=ptb_read(bf->fd, &position->length, 2);
	ret+=ptb_read(bf->fd, &position->fermenta, 1);
	ret+=ptb_read(bf->fd, &position->let_ring, 1);
	ret+=ptb_read_unknown(bf->fd, 1);
	
	return ret + ptb_read_items(bf, default_section_handlers);
}

int handle_CDynamic (struct ptbf *bf, const char *section) { 
	struct ptb_dynamic *dynamic = calloc(sizeof(struct ptb_dynamic), 1);
	int ret = 0;

	bf->dynamics = g_list_append(bf->dynamics, dynamic);

	ret+=ptb_read(bf->fd, &dynamic->offset, 1);
	ret+=ptb_read_unknown(bf->fd, 5); /* FIXME */

	return ret;

}
int handle_CSectionSymbol (struct ptbf *bf, const char *section) {
	int ret = 0;
	struct ptb_sectionsymbol *sectionsymbol = calloc(sizeof(struct ptb_sectionsymbol), 1);

	bf->sectionsymbols = g_list_append(bf->sectionsymbols, sectionsymbol);

	ret+=ptb_read_unknown(bf->fd, 5); /* FIXME */
	ret+=ptb_read(bf->fd, &sectionsymbol->repeat_ending, 2);

	return ret;
}

int handle_CMusicBar (struct ptbf *bf, const char *section) { 
	int ret = 0;
	struct ptb_musicbar *musicbar = calloc(sizeof(struct ptb_musicbar), 1);

	bf->musicbars = g_list_append(bf->musicbars, musicbar);

	ret+=ptb_read_unknown(bf->fd, 10); /* FIXME */

	return ret; 
}

int handle_CRhythmSlash (struct ptbf *bf, const char *section) { 
	int ret = 0;
	struct ptb_rhythmslash *rhythmslash = calloc(sizeof(struct ptb_rhythmslash), 1);

	bf->rhythmslashs = g_list_append(bf->rhythmslashs, rhythmslash);

	ret+=ptb_read_unknown(bf->fd, 6); /* FIXME */

	return ret;
}

int handle_CDirection (struct ptbf *bf, const char *section) { 
	int ret = 0;
	struct ptb_direction *direction = calloc(sizeof(struct ptb_direction), 1);

	bf->directions = g_list_append(bf->directions, direction);

	ret+=ptb_read_unknown(bf->fd, 4); /* FIXME */

	return ret;
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
