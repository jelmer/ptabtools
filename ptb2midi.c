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
#include "ptb.h"

#define MIDI_COMMAND_

void midi_write_meta_data(FILE *out, char command, void *data, guint8 length)
{
	fwrite(out, &command, 1);
	fwrite(out, &length, sizeof(length));
	fwrite(out, data, length);
}

void midi_write_channel_data(FILE *out, char command, char channel, void *data, size_t length)
{
	guint8 first;
	first = command ^ 4 + channel;
	fwrite(out, &first, 1);
	fwrite(out, data, length);
}

int main(int argc, const char **argv) 
{
	FILE *out = stdout;
	struct ptbf *ret;
	guint16 uint16;
	int debugging = 0;
	GList *gl;
	int c;
	int version = 0;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2midi Version "PTB_VERSION"\n");
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
	ret = ptb_read_file(poptGetArg(pc));
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	if(output) {
		out = fopen(output, "w+");
		if(!out) {
			perror("open");
			return -1;
		}
	} 
	
	fprintf(out, "MThd\00\00\00\06");
	fprintf(out, "\00\01"); /* Multiple tracks, synchronous */
	fprintf(out, "\00\06"); /* Max. number of tracks */
	fprintf(out, "\01\01"); /* Number of delta-time ticks per quarter */

	gl = ret->instrument[instrument].sections;
	while(gl) {
		midi_write_section(out, (struct ptb_section *)gl->data);
		fprintf(out, "\n\n");
		gl = gl->next;
	}

	if(output)fclose(out);
	
	return (ret?0:1);
}


