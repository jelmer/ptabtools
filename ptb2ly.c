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
#include "ptb.h"

void ly_write_header(FILE *out, struct ptbf *ret) 
{
	fprintf(out, "\\header {\n");
	if(ret->hdr.classification == CLASSIFICATION_SONG) {
		if(ret->hdr.class_info.song.title) 	fprintf(out, "  title = \"%s\"\n", ret->hdr.class_info.song.title);
		if(ret->hdr.class_info.song.artist) fprintf(out, "  composer = \"%s\"\n", ret->hdr.class_info.song.artist);
		if(ret->hdr.class_info.song.copyright) fprintf(out, "  copyright = \"%s\"\n", ret->hdr.class_info.song.copyright);
	} else if(ret->hdr.classification == CLASSIFICATION_LESSON) {
		if(ret->hdr.class_info.lesson.title) 	fprintf(out, "  title = \"%s\"\n", ret->hdr.class_info.lesson.title);
		if(ret->hdr.class_info.lesson.artist) fprintf(out, "  composer = \"%s\"\n", ret->hdr.class_info.lesson.author);
		if(ret->hdr.class_info.lesson.copyright) fprintf(out, "  copyright = \"%s\"\n", ret->hdr.class_info.lesson.copyright);
	}
	fprintf(out, "}\n");
}

int main(int argc, char **argv) 
{
	FILE *out = stdout;
	struct ptbf *ret;
	debugging = 1;
	ret = ptb_read_file(argv[1], default_sections);
	if(!ret) {
		perror("Read error: ");
		return -1;
	}

	fprintf(out, "\\version \"1.9.8\"\n\n");

	ly_write_header(out, ret);
	
	return (ret?0:1);
}
