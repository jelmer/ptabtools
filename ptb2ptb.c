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

#include "ptb.h"


int main(int argc, const char **argv) 
{
	struct ptbf *ret;
	int debugging = 0;
	int version = 0;
	int quiet = 0;
	const char *input;
	int c;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2ptb Version "PACKAGE_VERSION"\n");
			printf("(C) 2004-2005 Jelmer Vernooij <jelmer@samba.org>\n");
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
	
	if (!quiet) fprintf(stderr, "Parsing %s... \n", input);
					
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
		output = malloc(baselength + 9);
		strncpy(output, input, baselength);
		strcpy(output + baselength, ".ptb.2");
	}

	if (!quiet) fprintf(stderr, "Generating new powertab file in %s...\n", output);

	if (ptb_write_file(output, ret) == -1) {
		fprintf(stderr, "Error while generating PowerTab file\n");
	}

	free(output);
	
	return (ret?0:1);
}
