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

#define SMART_ADD_CHILD_STRING(parent, name, contents) if(contents) { xmlNodePtr tmp = xmlNewNode(NULL, name); xmlNodeSetContent(tmp, contents); xmlAddChild(parent, tmp); }
#define SMART_ADD_CHILD_INT(parent, name, contents) { char tmpc[100]; xmlNodePtr tmp = xmlNewNode(NULL, name); g_snprintf(tmpc, 100, "%d", contents); xmlNodeSetContent(tmp, tmpc); xmlAddChild(parent, tmp); }

xmlNodePtr xml_write_directions(GList *directions)
{
	xmlNodePtr xdirections = xmlNewNode(NULL, "directions");
	GList *gl = directions;

	while(gl) {
		struct ptb_direction *direction = gl->data;
		xmlNodePtr xdirection = xmlNewNode(NULL, "direction");
		xmlAddChild(xdirections, xdirection);

		gl = gl->next;
	}
	return xdirections;
}

xmlNodePtr xml_write_rhythmslashes(GList *rhythmslashs)
{
	xmlNodePtr xrhythmslashs = xmlNewNode(NULL, "rhythmslashs");
	GList *gl = rhythmslashs;

	while(gl) {
		struct ptb_rhythmslash *rhythmslash = gl->data;
		xmlNodePtr xrhythmslash = xmlNewNode(NULL, "rhythmslash");
		xmlAddChild(xrhythmslashs, xrhythmslash);
		
		gl = gl->next;
	}
	
	return xrhythmslashs;
}

xmlNodePtr xml_write_chordtexts(GList *chordtexts)
{
	xmlNodePtr xchordtexts = xmlNewNode(NULL, "chordtexts");
	GList *gl = chordtexts;

	while(gl) {
		struct ptb_chordtext *chordtext = gl->data;
		xmlNodePtr xchordtext = xmlNewNode(NULL, "chordtext");
		xmlAddChild(xchordtexts, xchordtext);

		SMART_ADD_CHILD_INT(xchordtext, "note1", chordtext->name[0]);
		SMART_ADD_CHILD_INT(xchordtext, "note2", chordtext->name[1]);
		SMART_ADD_CHILD_INT(xchordtext, "offset", chordtext->offset);
		SMART_ADD_CHILD_INT(xchordtext, "additions", chordtext->additions);
		SMART_ADD_CHILD_INT(xchordtext, "alterations", chordtext->alterations);
		SMART_ADD_CHILD_INT(xchordtext, "properties", chordtext->properties);

		gl = gl->next;
	}
	return xchordtexts;
}

xmlNodePtr xml_write_staffs(GList *staffs)
{
	xmlNodePtr xstaffs = xmlNewNode(NULL, "staffs");
	GList *gl = staffs;

	while(gl) {
		struct ptb_staff *staff = gl->data;
		xmlNodePtr xstaff = xmlNewNode(NULL, "staff");
		xmlAddChild(xstaffs, xstaff);

		SMART_ADD_CHILD_INT(xstaff, "highest_note", staff->highest_note);
		SMART_ADD_CHILD_INT(xstaff, "lowest_note", staff->lowest_note);
		SMART_ADD_CHILD_INT(xstaff, "properties", staff->properties);

		/* FIXME: Positions */
		/* FIXME: Musicbars */
		
		gl = gl->next;
	}
	return xstaffs;
}

xmlNodePtr xml_write_sections(GList *sections) 
{
	xmlNodePtr sctns = xmlNewNode(NULL, "sections");
	GList *gl = sections;

	while(gl) {
		struct ptb_section *section = gl->data;
		xmlNodePtr meter_type;
		xmlNodePtr xsection = xmlNewNode(NULL, "section");

		xmlAddChild(sctns, xsection);

		{
		char tmp[100];
		g_snprintf(tmp, 100, "%c", section->letter);
		xmlSetProp(xsection, "letter", tmp);
		}

		switch(section->end_mark) {
		case END_MARK_TYPE_NORMAL:
			SMART_ADD_CHILD_STRING(xsection, "end-mark", "normal");
			break;
		case END_MARK_TYPE_REPEAT:
			SMART_ADD_CHILD_STRING(xsection, "end-mark", "repeat");
			break;
		default:
			xmlAddChild(xsection, xmlNewComment("Unknown end-mark! FIXME"));
			break;
		}

		meter_type = xmlNewNode(NULL, "meter-type");
		xmlAddChild(xsection, meter_type);

		if(section->meter_type & METER_TYPE_BEAM_2) SMART_ADD_CHILD_STRING(meter_type, "beam_2", "");
		if(section->meter_type & METER_TYPE_BEAM_4) SMART_ADD_CHILD_STRING(meter_type, "beam_4", "");
		if(section->meter_type & METER_TYPE_BEAM_3) SMART_ADD_CHILD_STRING(meter_type, "beam_3", "");
		if(section->meter_type & METER_TYPE_BEAM_5) SMART_ADD_CHILD_STRING(meter_type, "beam_5", "");
		if(section->meter_type & METER_TYPE_BEAM_6) SMART_ADD_CHILD_STRING(meter_type, "beam_6", "");
		if(section->meter_type & METER_TYPE_COMMON) SMART_ADD_CHILD_STRING(meter_type, "common", "");
		if(section->meter_type & METER_TYPE_CUT) SMART_ADD_CHILD_STRING(meter_type, "cut", "");
		if(section->meter_type & METER_TYPE_SHOW) SMART_ADD_CHILD_STRING(meter_type, "show", "");

		SMART_ADD_CHILD_INT(xsection, "beat", section->beat_value);
		SMART_ADD_CHILD_INT(xsection, "metronome-pulses-per-measure", section->metronome_pulses_per_measure);
		SMART_ADD_CHILD_INT(xsection, "properties", section->properties);
		SMART_ADD_CHILD_INT(xsection, "key-extra", section->key_extra);
		SMART_ADD_CHILD_INT(xsection, "position-width", section->position_width);
		SMART_ADD_CHILD_STRING(xsection, "description", section->description);

//		xmlAddChild(xsection, xml_write_chordtexts(section->chordtexts));
//		xmlAddChild(xsection, xml_write_rhythmslashes(section->rhythmslashes));
//		xmlAddChild(xsection, xml_write_directions(section->directions));
//		xmlAddChild(xsection, xml_write_staffs(section->staffs));

		gl = gl->next;
	}

	return sctns;
}

xmlNodePtr xml_write_guitars(GList *guitars) 
{
	xmlNodePtr gtrs = xmlNewNode(NULL, "guitars");
	GList *gl = guitars;

	while(gl) {
		char tmp[100];
		int i;
		struct ptb_guitar *gtr = gl->data;
		xmlNodePtr xgtr = xmlNewNode(NULL, "guitar");
		xmlNodePtr strings;
		xmlAddChild(gtrs, xgtr);

		g_snprintf(tmp, 100, "%d", gtr->index);
		xmlSetProp(xgtr, "id", tmp);

		strings = xmlNewNode(NULL, "strings");
		xmlAddChild(xgtr, strings);

		for(i = 0; i < gtr->nr_strings; i++) {
			SMART_ADD_CHILD_INT(strings, "string", gtr->strings[i]);
		}

		SMART_ADD_CHILD_INT(xgtr, "reverb", gtr->reverb);
		SMART_ADD_CHILD_INT(xgtr, "chorus", gtr->chorus);
		SMART_ADD_CHILD_INT(xgtr, "tremolo", gtr->tremolo);
		SMART_ADD_CHILD_INT(xgtr, "pan", gtr->pan);
		SMART_ADD_CHILD_INT(xgtr, "capo", gtr->capo);
		SMART_ADD_CHILD_INT(xgtr, "initial_volume", gtr->initial_volume);
		SMART_ADD_CHILD_INT(xgtr, "midi_instrument", gtr->midi_instrument);
		SMART_ADD_CHILD_INT(xgtr, "half_up", gtr->half_up);
		SMART_ADD_CHILD_INT(xgtr, "simulate", gtr->simulate);

		gl = gl->next;
	}
	
	return gtrs;
}

xmlNodePtr xml_write_instrument(struct ptbf *bf, int i)
{
	char tmp[100];
	xmlNodePtr instrument = xmlNewNode(NULL, "instrument");
	g_snprintf(tmp, 100, "%d", i);
	xmlSetProp(instrument, "id", tmp);

	xmlAddChild(instrument, xml_write_guitars(bf->instrument[i].guitars));
	xmlAddChild(instrument, xml_write_sections(bf->instrument[i].guitars));
	/*FIXME*/
	return instrument;
}

xmlNodePtr xml_write_song_header(struct ptb_hdr *hdr)
{
	xmlNodePtr song = xmlNewNode(NULL, "song");

	SMART_ADD_CHILD_STRING(song, "title", hdr->class_info.song.title); 
	SMART_ADD_CHILD_STRING(song, "artist", hdr->class_info.song.artist); 
	SMART_ADD_CHILD_STRING(song, "words-by", hdr->class_info.song.words_by); 
	SMART_ADD_CHILD_STRING(song, "music-by", hdr->class_info.song.music_by); 
	SMART_ADD_CHILD_STRING(song, "arranged-by", hdr->class_info.song.arranged_by); 
	SMART_ADD_CHILD_STRING(song, "guitar-transcribed-by", hdr->class_info.song.guitar_transcribed_by); 
	SMART_ADD_CHILD_STRING(song, "bass-transcribed-by", hdr->class_info.song.bass_transcribed_by); 
	SMART_ADD_CHILD_STRING(song, "lyrics", hdr->class_info.song.lyrics);
	SMART_ADD_CHILD_STRING(song, "copyright", hdr->class_info.song.copyright);

	/* FIXME: Sub stuff */

	return song;
}

xmlNodePtr xml_write_lesson_header(struct ptb_hdr *hdr)
{
	xmlNodePtr lesson = xmlNewNode(NULL, "lesson");

	SMART_ADD_CHILD_STRING(lesson, "title", hdr->class_info.lesson.title); 
	SMART_ADD_CHILD_STRING(lesson, "artist", hdr->class_info.lesson.artist); 
	SMART_ADD_CHILD_STRING(lesson, "author", hdr->class_info.lesson.author);
	SMART_ADD_CHILD_STRING(lesson, "copyright", hdr->class_info.lesson.copyright);

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
	xmlNodePtr comment;
	int c, i;
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

	comment = xmlNewComment("\nGenerated by ptb2xml, part of ptabtools. \n"
							"(C) 2004 by Jelmer Vernooij <jelmer@samba.org>\n"
							"See http://jelmer.vernstok.nl/oss/ptabtools/ for details\n");
	xmlAddChild(root_node, comment);

	xmlAddChild(root_node, xml_write_header(&ret->hdr));

	for(i = 0; i < 2; i++) {
		xmlAddChild(root_node, xml_write_instrument(ret, i));
	}

	xmlSaveFormatFileEnc(output?output:"-", doc, "UTF-8", 1);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}
