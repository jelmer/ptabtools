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

int write_section(struct ptbf *bf, const char *sectionname)
{
	int efd;
	char *outname = g_strdup_printf("%s_%s", bf->filename, sectionname);
	efd = creat(outname, 0755);
	if(efd < 0) {
		perror("Open");
		return -1;
	}

	while(!ptb_end_of_section(bf->fd)) {
		char byte;
		if(read(bf->fd, &byte, 1) < 1) perror("read");
		if(write(efd, &byte, 1) < 1) perror("write");
	}

	close(efd);
	return 0;
}

struct ptb_section_handler sections[] = {
	{NULL, write_section }
};

int main(int argc, char **argv) 
{
	struct ptbf *ret = ptb_read_file(argv[1], sections);
	if(ret) {
		printf("Read successful!\n");
	} else {
		perror("Read error: ");
	}
	return (ret?0:1);
}
