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


ssize_t handle_unknown (struct ptbf *bf, const char *section) {
	fprintf(stderr, "Unknown section '%s'\n", section);	
	return -1; 
}

ssize_t handle_CGuitar (struct ptbf *bf, const char *section) {
	ssize_t ret = 0;
	struct ptb_guitar *guitar = g_new0(struct ptb_guitar, 1);

	bf->guitars = g_list_append(bf->guitars, guitar);
		
	ret+=ptb_read(bf, &guitar->index, 1);
	ret+=ptb_read_string(bf, &guitar->title);

	ret+=ptb_read(bf, &guitar->midi_instrument, 1);
	ret+=ptb_read(bf, &guitar->initial_volume, 1);
	ret+=ptb_read(bf, &guitar->pan, 1);
	ret+=ptb_read(bf, &guitar->reverb, 1);
	ret+=ptb_read(bf, &guitar->chorus, 1);
	ret+=ptb_read(bf, &guitar->tremolo, 1);

	ret+=ptb_read(bf, &guitar->simulate, 1);
	ret+=ptb_read(bf, &guitar->capo, 1);
		
	ret+=ptb_read_string(bf, &guitar->type);

	ret+=ptb_read(bf, &guitar->half_up, 1);
	ret+=ptb_read(bf, &guitar->nr_strings, 1);
	guitar->strings = g_new(guint8, guitar->nr_strings);
	ret+=ptb_read(bf, guitar->strings, guitar->nr_strings);

	return ret;
}


ssize_t handle_CFloatingText (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_floatingtext *text = g_new0(struct ptb_floatingtext, 1);

	bf->floatingtexts = g_list_append(bf->floatingtexts, text);

	ret+=ptb_read_string(bf, &text->text);
	ret+=ptb_read(bf, &text->beginpos, 1);
	ret+=ptb_read_unknown(bf, 15);
	ret+=ptb_read(bf, &text->alignment, 1);
	ret+=ptb_read_font(bf, &text->font);
	ret+=ptb_read_unknown(bf, 4);
	
	return ret;
}

ssize_t handle_CSection (struct ptbf *bf, const char *sectionname) { 
	ssize_t ret = 0;
	struct ptb_section *section = g_new0(struct ptb_section, 1);

	bf->sections = g_list_append(bf->sections, section);

	ret+=ptb_read_constant(bf, 0x32);
	ret+=ptb_read_unknown(bf, 11);
	ret+=ptb_read(bf, &section->properties, 2);
	ret+=ptb_read_unknown(bf, 2);
/*	175 -> 3 staffs
	07d -> 1 staff
	102 -> 2  staffs
	*/
	printf("%04x\n", section->properties);
	ret+=ptb_read(bf, &section->end_mark, 1);
	ret+=ptb_read(bf, &section->position_width, 1);
	ret+=ptb_read_unknown(bf, 5);
	ret+=ptb_read(bf, &section->key_extra, 1);
	ret+=ptb_read_unknown(bf, 1);
	ret+=ptb_read(bf, &section->meter_type, 2);
	ret+=ptb_read(bf, &section->beat_value, 1);
	ret+=ptb_read(bf, &section->metronome_pulses_per_measure, 1);
	ret+=ptb_read(bf, &section->letter, 1);
	ret+=ptb_read_string(bf, &section->description);

	ret+=ptb_read_unknown(bf, 2);

	ptb_debug("Section prt 1");
	ret+=ptb_read_items(bf);
	ptb_debug("Section prt 2");
	ret+=ptb_read_items(bf);
	ptb_debug("Section prt 3");
	ret+=ptb_read_items(bf);
//	ret+=ptb_read_stuff(bf);
/*	ret+=ptb_read_items(bf);
	ret+=ptb_read_items(bf);
	ret+=ptb_read_items(bf);*/

	return ret; 
}

ssize_t handle_CTempoMarker (struct ptbf *bf, const char *section) {
	ssize_t ret = 0;
	struct ptb_tempomarker *tempomarker = g_new0(struct ptb_tempomarker, 1);

	bf->tempomarkers = g_list_append(bf->tempomarkers, tempomarker);

	ret+=ptb_read_unknown(bf, 3);
	ret+=ptb_read(bf, &tempomarker->bpm, 1);
	ret+=ptb_read_unknown(bf, 1);
	ret+=ptb_read(bf, &tempomarker->type, 2);
	ret+=ptb_read_string(bf, &tempomarker->description);

	return ret;
}


ssize_t handle_CChordDiagram (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_chorddiagram *chorddiagram = g_new0(struct ptb_chorddiagram, 1);

	bf->chorddiagrams = g_list_append(bf->chorddiagrams, chorddiagram);

	ret+=ptb_read(bf, chorddiagram->name, 2);
	ret+=ptb_read_unknown(bf, 3);
	ret+=ptb_read(bf, &chorddiagram->type, 1);
	ret+=ptb_read(bf, &chorddiagram->frets, 1);
	ret+=ptb_read(bf, &chorddiagram->nr_strings, 1);
	chorddiagram->tones = g_new(guint8, chorddiagram->nr_strings);
	ret+=ptb_read(bf, chorddiagram->tones, chorddiagram->nr_strings);

	return ret;
}

ssize_t handle_CLineData (struct ptbf *bf, const char *section) { 
	struct ptb_linedata *linedata = g_new0(struct ptb_linedata, 1);
	ssize_t ret = 0;

	bf->linedatas = g_list_append(bf->linedatas, linedata);

	ret+=ptb_read(bf, &linedata->tone, 1);
	ret+=ptb_read(bf, &linedata->properties, 1);
	ret+=ptb_read(bf, &linedata->transcribe, 1);
	ret+=ptb_read(bf, &linedata->conn_to_next, 1);
	
	if(linedata->conn_to_next) { 
		ptb_read_unknown(bf, 4);
	}

	return ret;
}


ssize_t handle_CChordText (struct ptbf *bf, const char *section) {
	ssize_t ret = 0;
	struct ptb_chordtext *chordtext = g_new0(struct ptb_chordtext, 1);

	bf->chordtexts = g_list_append(bf->chordtexts, chordtext);

	ret+=ptb_read(bf, &chordtext->offset, 1);
	ret+=ptb_read(bf, chordtext->name, 2);

	ret+=ptb_read_unknown(bf, 1); /* FIXME */
	ret+=ptb_read(bf, &chordtext->additions, 1);
	ret+=ptb_read(bf, &chordtext->alterations, 1);
	ret+=ptb_read_unknown(bf, 1); /* FIXME */

	return ret;
}

ssize_t handle_CGuitarIn (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_guitarin *guitarin = g_new0(struct ptb_guitarin, 1);

	bf->guitarins = g_list_append(bf->guitarins, guitarin);

	ret+=ptb_read(bf, &guitarin->section, 1);
	ret+=ptb_read_unknown(bf, 1); /* FIXME */
	ret+=ptb_read(bf, &guitarin->staff, 1);
	ret+=ptb_read(bf, &guitarin->offset, 1);
	ret+=ptb_read(bf, &guitarin->rhythm_slash, 1);
	ret+=ptb_read(bf, &guitarin->staff_in, 1);

	return ret;
}


ssize_t handle_CStaff (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_staff *staff = g_new0(struct ptb_staff, 1);

	bf->staffs = g_list_append(bf->staffs, staff);

	ret+=ptb_read(bf, &staff->properties, 1);
	ret+=ptb_read(bf, &staff->highest_note, 1);
	ret+=ptb_read(bf, &staff->lowest_note, 1);
	ret+=ptb_read_unknown(bf, 1); /* FIXME */
	ret+=ptb_read(bf, &staff->extra_data, 1);

	ptb_debug("Staff prt 1");
	ret+=ptb_read_items(bf);
	ptb_debug("Staff prt 2");
	ret+=ptb_read_items(bf);
	ptb_debug("Staff prt 3");
	ret+=ptb_read_items(bf);
//	ret+=ptb_read_unknown(bf, 2);
//	ret+=ptb_read_items(bf);
//	ret+=ptb_read_unknown(bf, staff->extra_data);
	return ret;
}


ssize_t handle_CPosition (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_position *position = g_new0(struct ptb_position, 1);

	bf->positions = g_list_append(bf->positions, position);

	ret+=ptb_read(bf, &position->offset, 1);
	ret+=ptb_read(bf, &position->properties, 2); /* FIXME */
	ret+=ptb_read(bf, &position->length, 2);
	ret+=ptb_read(bf, &position->fermenta, 1);
	ret+=ptb_read(bf, &position->let_ring, 1);
	ret+=ptb_read_constant(bf, 0x00);
	
	return ret + ptb_read_items(bf);
}

ssize_t handle_CDynamic (struct ptbf *bf, const char *section) { 
	struct ptb_dynamic *dynamic = g_new0(struct ptb_dynamic, 1);
	ssize_t ret = 0;

	bf->dynamics = g_list_append(bf->dynamics, dynamic);

	ret+=ptb_read(bf, &dynamic->offset, 1);
	ret+=ptb_read_unknown(bf, 5); /* FIXME */

	return ret;

}
ssize_t handle_CSectionSymbol (struct ptbf *bf, const char *section) {
	ssize_t ret = 0;
	struct ptb_sectionsymbol *sectionsymbol = g_new0(struct ptb_sectionsymbol, 1);

	bf->sectionsymbols = g_list_append(bf->sectionsymbols, sectionsymbol);

	ret+=ptb_read_unknown(bf, 5); /* FIXME */
	ret+=ptb_read(bf, &sectionsymbol->repeat_ending, 2);

	return ret;
}

ssize_t handle_CMusicBar (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_musicbar *musicbar = g_new0(struct ptb_musicbar, 1);

	bf->musicbars = g_list_append(bf->musicbars, musicbar);

	ret+=ptb_read_unknown(bf, 10); /* FIXME */

	return ret; 
}

ssize_t handle_CRhythmSlash (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_rhythmslash *rhythmslash = g_new0(struct ptb_rhythmslash, 1);

	bf->rhythmslashs = g_list_append(bf->rhythmslashs, rhythmslash);

	ret+=ptb_read_unknown(bf, 6); /* FIXME */

	return ret;
}

ssize_t handle_CDirection (struct ptbf *bf, const char *section) { 
	ssize_t ret = 0;
	struct ptb_direction *direction = g_new0(struct ptb_direction, 1);

	bf->directions = g_list_append(bf->directions, direction);

	ret+=ptb_read_unknown(bf, 4); /* FIXME */

	return ret;
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
