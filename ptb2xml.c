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
#include <sys/time.h>
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "ptb.h"

#define SMART_ADD_CHILD(parent, name, contents) { xmlNodePtr tmp = xmlNewNode(NULL, name); xmlNodeSetContent(tmp, contents); xmlAddChild(parent, tmp); }

xmlNodePtr xml_write_song_header(struct ptb_hdr *hdr)
{
	xmlNodePtr song = xmlNewNode(NULL, "song");

	SMART_ADD_CHILD(song, "title", hdr->class_info.song.title); 
	SMART_ADD_CHILD(song, "artist", hdr->class_info.song.artist); 
	SMART_ADD_CHILD(song, "words-by", hdr->class_info.song.words_by); 
	SMART_ADD_CHILD(song, "music-by", hdr->class_info.song.music_by); 
	SMART_ADD_CHILD(song, "arranged-by", hdr->class_info.song.arranged_by); 
	SMART_ADD_CHILD(song, "guitar-transcribed-by", hdr->class_info.song.guitar_transcribed_by); 
	SMART_ADD_CHILD(song, "bass-transcribed-by", hdr->class_info.song.bass_transcribed_by); 
	SMART_ADD_CHILD(song, "lyrics", hdr->class_info.song.lyrics);
	SMART_ADD_CHILD(song, "copyright", hdr->class_info.song.copyright);

	/* FIXME: Sub stuff */

	return song;
}

xmlNodePtr xml_write_lesson_header(struct ptb_hdr *hdr)
{
	xmlNodePtr lesson = xmlNewNode(NULL, "lesson");

	SMART_ADD_CHILD(lesson, "title", hdr->class_info.lesson.title); 
	SMART_ADD_CHILD(lesson, "artist", hdr->class_info.lesson.artist); 
	SMART_ADD_CHILD(lesson, "author", hdr->class_info.lesson.author);
	SMART_ADD_CHILD(lesson, "copyright", hdr->class_info.lesson.copyright);

	switch(hdr->class_info.lesson.level) {
	case LEVEL_BEGINNER: xmlSetProp(lesson, "level", "beginner"); break;
	case LEVEL_INTERMEDIATE: xmlSetProp(lesson, "level", "intermediate"); break;
	case LEVEL_ADVANCED: xmlSetProp(lesson, "level", "advanced"); break;
	}

	/* FIXME: Style */

	return lesson;
}

xmlNodePtr xml_write_header(struct ptb_hdr *hdr) 
{
	xmlNodePtr header = xmlNewNode(NULL, "header");
	switch(hdr->classification) {
	case CLASSIFICATION_SONG:
		xmlSetProp(header, "classification", "song");
		xmlAddChild(header, xml_write_song_header(hdr));
		break;
	case CLASSIFICATION_LESSON:
		xmlSetProp(header, "classification", "lesson");
		xmlAddChild(header, xml_write_lesson_header(hdr));
		break;
	}
	return header;
}

int main(int argc, const char **argv) 
{
	struct ptbf *ret;
	int debugging = 0;
	xmlNodePtr root_node;
	xmlDocPtr doc;
	xmlDtdPtr dtd;
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
			printf("ptb2ascii Version "PTB_VERSION"\n");
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

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "powertab");
	xmlDocSetRootElement(doc, root_node);

	dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, NULL);

	xmlAddChild(root_node, xml_write_header(&ret->hdr));

	xmlSaveFormatFileEnc(output?output:"-", doc, "UTF-8", 1);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}
