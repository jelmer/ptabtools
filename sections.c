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
	struct ptb_track *prevtrack = NULL;
	
	while(!ptb_end_of_section(bf->fd)) {
		struct ptb_track *track = calloc(sizeof(struct ptb_track), 1);
	
		if(bf->tracks) prevtrack->next = track;
		else bf->tracks = track;
		
		/* Track number */
		read(bf->fd, &track->index, 1);
		if(track->index == 0xff) return 0;
		ptb_read_string(bf->fd, &track->title);

		read(bf->fd, &track->midi_instrument, 1);
		read(bf->fd, &track->initial_volume, 1);
		read(bf->fd, &track->pan, 1);
		read(bf->fd, &track->reverb, 1);
		read(bf->fd, &track->chorus, 1);
		read(bf->fd, &track->tremolo, 1);

		read(bf->fd, unknown, 1);
		fprintf(stderr, "%s %02x\n", bf->filename, unknown[0]);
		read(bf->fd, &track->capo, 1);
		
		ptb_read_string(bf->fd, &track->type);

		//FIXME
		read(bf->fd, unknown, 1);
		ptb_read_string(bf->fd, &track->tempo);
		//FIXME
		read(bf->fd, unknown, 2);

		prevtrack = track;
	}

	return 0;
}


int handle_CFloatingText (struct ptbf *bf, const char *section) { return 0; }

int handle_CGuitarIn (struct ptbf *bf, const char *section) { return 0; }
int handle_CTempoMarker (struct ptbf *bf, const char *section) { return 0; }
int handle_CDynamic (struct ptbf *bf, const char *section) { return 0; }
int handle_CSectionSymbol (struct ptbf *bf, const char *section) { return 0; }
int handle_CSection (struct ptbf *bf, const char *section) { return 0; }
int handle_CChordText (struct ptbf *bf, const char *section) { return 0; }
int handle_CStaff (struct ptbf *bf, const char *section) { return 0; }
int handle_CPosition (struct ptbf *bf, const char *section) { return 0; }
int handle_CLineData (struct ptbf *bf, const char *section) { return 0; }
int handle_CMusicBar (struct ptbf *bf, const char *section) { return 0; }
int handle_CChordDiagram (struct ptbf *bf, const char *section) { return 0; }
int handle_CRhythmSlash (struct ptbf *bf, const char *section) { return 0; }
int handle_CDirection (struct ptbf *bf, const char *section) { return 0; }

struct ptb_section default_sections[] = {
	{"CGuitar", handle_CGuitar },
	{"CFloatingText", handle_CFloatingText },
	{"CGuitarIn", handle_CGuitarIn },
	{"CTempoMarker", handle_CTempoMarker},
	{"CDynamic", handle_CDynamic },
	{"CSectionSymbol", handle_CSectionSymbol },
	{"CSection", handle_CSection },
	{"CChordText", handle_CChordText },
	{"CStaff", handle_CStaff },
	{"CPosition", handle_CPosition },
	{"CLineData", handle_CLineData },
	{"CMusicBar", handle_CMusicBar },
	{"CChordDiagram", handle_CChordDiagram },
	{"CRhythmSlash", handle_CRhythmSlash },
	{"CDirection", handle_CDirection },
	{ 0, handle_unknown}
};
