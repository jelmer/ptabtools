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

#include "ptb_internals.h"
#include <stdio.h>


void *handle_unknown (struct ptbf *bf, const char *section) {
	fprintf(stderr, "Unknown section '%s'\n", section);	
	return NULL; 
}

void *handle_CGuitar (struct ptbf *bf, const char *section) {
	struct ptb_guitar *guitar = g_new0(struct ptb_guitar, 1);

	ptb_read(bf, &guitar->index, 1);
	ptb_read_string(bf, &guitar->title);

	ptb_read(bf, &guitar->midi_instrument, 1);
	ptb_read(bf, &guitar->initial_volume, 1);
	ptb_read(bf, &guitar->pan, 1);
	ptb_read(bf, &guitar->reverb, 1);
	ptb_read(bf, &guitar->chorus, 1);
	ptb_read(bf, &guitar->tremolo, 1);

	ptb_read(bf, &guitar->simulate, 1);
	ptb_read(bf, &guitar->capo, 1);
		
	ptb_read_string(bf, &guitar->type);

	ptb_read(bf, &guitar->half_up, 1);
	ptb_read(bf, &guitar->nr_strings, 1);
	guitar->strings = g_new(guint8, guitar->nr_strings);
	ptb_read(bf, guitar->strings, guitar->nr_strings);

	return guitar;
}


void *handle_CFloatingText (struct ptbf *bf, const char *section) { 
	struct ptb_floatingtext *text = g_new0(struct ptb_floatingtext, 1);

	ptb_read_string(bf, &text->text);
	ptb_read(bf, &text->beginpos, 1);
	ptb_read_unknown(bf, 15);
	ptb_read(bf, &text->alignment, 1);
	ptb_read_font(bf, &text->font);
	
	return text;
}

void *handle_CSection (struct ptbf *bf, const char *sectionname) { 
	struct ptb_section *section = g_new0(struct ptb_section, 1);

	ptb_read_constant(bf, 0x32);
	ptb_read_unknown(bf, 11);
	ptb_read(bf, &section->properties, 2);
	ptb_read_unknown(bf, 2);
/*	175 -> 3 staffs
	07d -> 1 staff
	102 -> 2  staffs
	*/
	printf("%04x\n", section->properties);
	ptb_read(bf, &section->end_mark, 1);
	ptb_read(bf, &section->position_width, 1);
	ptb_read_unknown(bf, 5);
	ptb_read(bf, &section->key_extra, 1);
	ptb_read_unknown(bf, 1);
	ptb_read(bf, &section->meter_type, 2);
	ptb_read(bf, &section->beat_value, 1);
	ptb_read(bf, &section->metronome_pulses_per_measure, 1);
	ptb_read(bf, &section->letter, 1);
	ptb_read_string(bf, &section->description);

	section->directions = ptb_read_items(bf, "CDirection");
	section->chordtexts = ptb_read_items(bf, "CChordText");
	section->rhythmslashes = ptb_read_items(bf, "CRhythmSlash");
	section->staffs = ptb_read_items(bf, "CStaff");

	return section; 
}

void *handle_CTempoMarker (struct ptbf *bf, const char *section) {
	struct ptb_tempomarker *tempomarker = g_new0(struct ptb_tempomarker, 1);

	ptb_read_unknown(bf, 3);
	ptb_read(bf, &tempomarker->bpm, 1);
	ptb_read_unknown(bf, 1);
	ptb_read(bf, &tempomarker->type, 2);
	ptb_read_string(bf, &tempomarker->description);

	return tempomarker;
}


void *handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	struct ptb_chorddiagram *chorddiagram = g_new0(struct ptb_chorddiagram, 1);

	ptb_read(bf, chorddiagram->name, 2);
	ptb_read_unknown(bf, 3);
	ptb_read(bf, &chorddiagram->type, 1);
	ptb_read(bf, &chorddiagram->frets, 1);
	ptb_read(bf, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = g_new(guint8, chorddiagram->nr_strings);
	ptb_read(bf, chorddiagram->tones, chorddiagram->nr_strings);

	return chorddiagram;
}

void *handle_CLineData (struct ptbf *bf, const char *section) { 
	struct ptb_linedata *linedata = g_new0(struct ptb_linedata, 1);

	ptb_read(bf, &linedata->tone, 1);
	ptb_read(bf, &linedata->properties, 1);
	ptb_debug("Properties: %02x", linedata->properties);
	ptb_read(bf, &linedata->transcribe, 1);
	ptb_debug("Transcribe: %02x", linedata->transcribe);
	ptb_read(bf, &linedata->conn_to_next, 1);
	
	if(linedata->conn_to_next) { 
		ptb_debug("Conn to next!: %02x", linedata->conn_to_next);
		ptb_read_unknown(bf, 4*linedata->conn_to_next);
	}

	return linedata;
}


void *handle_CChordText (struct ptbf *bf, const char *section) {
	struct ptb_chordtext *chordtext = g_new0(struct ptb_chordtext, 1);

	ptb_read(bf, &chordtext->offset, 1);
	ptb_read(bf, chordtext->name, 2);

	ptb_read_unknown(bf, 1); /* FIXME */
	ptb_read(bf, &chordtext->additions, 1);
	ptb_read(bf, &chordtext->alterations, 1);
	ptb_read_unknown(bf, 1); /* FIXME */

	return chordtext;
}

void *handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	struct ptb_guitarin *guitarin = g_new0(struct ptb_guitarin, 1);

	ptb_read(bf, &guitarin->section, 1);
	ptb_read_unknown(bf, 1); /* FIXME */
	ptb_read(bf, &guitarin->staff, 1);
	ptb_read(bf, &guitarin->offset, 1);
	ptb_read(bf, &guitarin->rhythm_slash, 1);
	ptb_read(bf, &guitarin->staff_in, 1);

	return guitarin;
}


void *handle_CStaff (struct ptbf *bf, const char *section) { 
	struct ptb_staff *staff = g_new0(struct ptb_staff, 1);

	ptb_read(bf, &staff->properties, 1);
	ptb_read(bf, &staff->highest_note, 1);
	ptb_read(bf, &staff->lowest_note, 1);
	ptb_read_unknown(bf, 1); /* FIXME */
	ptb_read(bf, &staff->extra_data, 1);

	staff->positions1 = ptb_read_items(bf, "CPosition");
	staff->positions2 = ptb_read_items(bf, "CPosition");
	staff->musicbars = ptb_read_items(bf, "CMusicBar");
	return staff;
}


void *handle_CPosition (struct ptbf *bf, const char *section) { 
	struct ptb_position *position = g_new0(struct ptb_position, 1);

	ptb_read(bf, &position->offset, 1);
	ptb_read(bf, &position->properties, 2); /* FIXME */
	ptb_debug("Properties: %04x", position->properties);
	ptb_read_unknown(bf, 2);
	ptb_read(bf, &position->fermenta, 1);
	ptb_read(bf, &position->length, 1);
	ptb_read_constant(bf, 0x00);

	position->linedatas = ptb_read_items(bf, "CLineData");
	
	return position;
}

void *handle_CDynamic (struct ptbf *bf, const char *section) { 
	struct ptb_dynamic *dynamic = g_new0(struct ptb_dynamic, 1);

	ptb_read(bf, &dynamic->offset, 1);
	ptb_read_unknown(bf, 5); /* FIXME */

	return dynamic;
}

void *handle_CSectionSymbol (struct ptbf *bf, const char *section) {
	struct ptb_sectionsymbol *sectionsymbol = g_new0(struct ptb_sectionsymbol, 1);

	ptb_read_unknown(bf, 5); /* FIXME */
	ptb_read(bf, &sectionsymbol->repeat_ending, 2);

	return sectionsymbol;
}

void *handle_CMusicBar (struct ptbf *bf, const char *section) { 
	struct ptb_musicbar *musicbar = g_new0(struct ptb_musicbar, 1);

	ptb_read_unknown(bf, 8); /* FIXME */
	ptb_read(bf, &musicbar->letter, 1);
	ptb_read_string(bf, &musicbar->description);

	return musicbar; 
}

void *handle_CRhythmSlash (struct ptbf *bf, const char *section) { 
	struct ptb_rhythmslash *rhythmslash = g_new0(struct ptb_rhythmslash, 1);

	ptb_read_unknown(bf, 6); /* FIXME */

	return rhythmslash;
}

void *handle_CDirection (struct ptbf *bf, const char *section) { 
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
