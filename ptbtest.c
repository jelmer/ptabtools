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
#include <fcntl.h>
#include "ptb.h"


int main(int argc, char **argv) 
{
	int i;
	
	if(argc < 3) {
		fprintf(stderr, "Usage: %s <SectionName> <InputFile>\n");
		return 1;
	}

	debugging = 1;

	for(i = 0; default_section_handlers[i].name; i++) {
		if(!strcmp(default_section_handlers[i].name, argv[1])) {
			struct ptbf *bf = calloc(sizeof(struct ptbf), 1);
			bf->filename = strdup(argv[2]);
			bf->fd = open(bf->filename, O_RDONLY);
			if(default_section_handlers[i].handler(bf, argv[2]) != 0) {
				fprintf(stderr, "Parsing file '%s' as section '%s' unsuccessful!\n", argv[2], argv[1]);
				return 1;
			}
			return 0;
		}
	}

	fprintf(stderr, "Unknown section '%s'\n", argv[1]);
	
	return 1;
}
