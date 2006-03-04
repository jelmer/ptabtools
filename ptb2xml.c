/*
	(c) 2004-2005: Jelmer Vernooij <jelmer@samba.org>

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
#include <string.h>
#include <popt.h>
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#define DTD_URL "http://jelmer.vernstok.nl/oss/ptabtools/svn/trunk/ptbxml.dtd"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "ptb.h"

#ifdef HAVE_XSLT
#  include <libxslt/xslt.h>
#  include <libxslt/transform.h>
#endif

#define SMART_ADD_PROP_INT(parent, name, contents) { \
	char tmpc[100]; \
	snprintf(tmpc, 100, "%d", contents); \
	xmlSetProp(parent, name, tmpc); \
}


#define SMART_ADD_CHILD_STRING(parent, name, contents) xmlNewTextChild(parent, NULL, name, contents)
#define SMART_ADD_CHILD_INT(parent, name, contents) { \
	char tmpc[100]; \
	xmlNodePtr tmp = xmlNewNode(NULL, name); \
	snprintf(tmpc, 100, "%d", contents); \
	xmlNodeSetContent(tmp, tmpc); \
	xmlAddChild(parent, tmp); \
}

#define SMART_ADD_CHILD_XINT(parent, name, contents) { \
	char tmpc[100]; \
	xmlNodePtr tmp = xmlNewNode(NULL, name); \
	snprintf(tmpc, 100, "%x", contents); \
	xmlNodeSetContent(tmp, tmpc); \
	xmlAddChild(parent, tmp); \
}

xmlNodePtr xml_write_font(const char *name, struct ptb_font *font)
{
	xmlNodePtr xfont = xmlNewNode(NULL, "font");
	xmlSetProp(xfont, "function", name);
	SMART_ADD_PROP_INT(xfont, "pointsize", font->pointsize);
	SMART_ADD_PROP_INT(xfont, "weight", font->weight);
	SMART_ADD_PROP_INT(xfont, "underlined", font->underlined);
	SMART_ADD_PROP_INT(xfont, "italic", font->italic);
	xmlSetProp(xfont, "family", font->family);
	return xfont;
}

xmlNodePtr xml_write_directions(struct ptb_direction *directions)
{
	xmlNodePtr xdirections = xmlNewNode(NULL, "directions");
	struct ptb_direction *direction = directions;

	while(direction) {
		xmlNodePtr xdirection = xmlNewNode(NULL, "direction");
		xmlAddChild(xdirections, xdirection);

		direction = direction->next;
	}
	return xdirections;
}

xmlNodePtr xml_write_rhythmslashes(struct ptb_rhythmslash *rhythmslashs)
{
	xmlNodePtr xrhythmslashs = xmlNewNode(NULL, "rhythmslashs");
	struct ptb_rhythmslash *rhythmslash = rhythmslashs;

	while(rhythmslash) {
		xmlNodePtr xrhythmslash = xmlNewNode(NULL, "rhythmslash");
		xmlAddChild(xrhythmslashs, xrhythmslash);
		SMART_ADD_CHILD_INT(xrhythmslash, "properties", rhythmslash->properties);
		SMART_ADD_PROP_INT(xrhythmslash, "offset", rhythmslash->offset);
		SMART_ADD_CHILD_INT(xrhythmslash, "dotted", rhythmslash->dotted);
		SMART_ADD_CHILD_INT(xrhythmslash, "length", rhythmslash->length);
		
		rhythmslash = rhythmslash->next;
	}
	
	return xrhythmslashs;
}

xmlNodePtr xml_write_chordtexts(struct ptb_chordtext *chordtexts)
{
	xmlNodePtr xchordtexts = xmlNewNode(NULL, "chordtexts");
	struct ptb_chordtext *chordtext = chordtexts;

	while(chordtext) {
		xmlNodePtr xchordtext = xmlNewNode(NULL, "chordtext");
		xmlAddChild(xchordtexts, xchordtext);

		SMART_ADD_CHILD_STRING(xchordtext, "note1", ptb_get_tone(chordtext->name[0]));
		SMART_ADD_CHILD_STRING(xchordtext, "note2", ptb_get_tone(chordtext->name[1]));
		SMART_ADD_PROP_INT(xchordtext, "offset", chordtext->offset);
		SMART_ADD_CHILD_INT(xchordtext, "additions", chordtext->additions);
		SMART_ADD_CHILD_INT(xchordtext, "alterations", chordtext->alterations);
		SMART_ADD_CHILD_INT(xchordtext, "properties", chordtext->properties);
		SMART_ADD_CHILD_INT(xchordtext, "VII", chordtext->VII);

		chordtext = chordtext->next;
	}
	return xchordtexts;
}

xmlNodePtr xml_write_musicbars(struct ptb_musicbar *musicbars)
{
	xmlNodePtr xmusicbars = xmlNewNode(NULL, "musicbars");
	struct ptb_musicbar *musicbar = musicbars;

	while(musicbar) {
		xmlNodePtr xmusicbar = SMART_ADD_CHILD_STRING(xmusicbars, "musicbar", musicbar->description);

		if(musicbar->letter != 0x7f) {
			char tmp[100];
			snprintf(tmp, 100, "%c", musicbar->letter);
			xmlSetProp(xmusicbar, "letter", tmp);
		}

		musicbar = musicbar->next;
	}
	return xmusicbars;
}

xmlNodePtr xml_write_linedatas(struct ptb_linedata *linedatas)
{
	xmlNodePtr xlinedatas = xmlNewNode(NULL, "linedatas");
	struct ptb_linedata *linedata = linedatas;

	while(linedata) {
		xmlNodePtr xlinedata = xmlNewNode(NULL, "linedata");
		xmlAddChild(xlinedatas, xlinedata);

		SMART_ADD_CHILD_INT(xlinedata, "string", linedata->detailed.string);
		SMART_ADD_CHILD_INT(xlinedata, "fret", linedata->detailed.fret);
		SMART_ADD_CHILD_INT(xlinedata, "properties", linedata->properties);
		SMART_ADD_CHILD_INT(xlinedata, "transcribe", linedata->transcribe);
		SMART_ADD_CHILD_INT(xlinedata, "conn_to_next", linedata->conn_to_next);

		linedata = linedata->next;
	}
	return xlinedatas;
}

xmlNodePtr xml_write_positions(struct ptb_position *positions)
{
	xmlNodePtr xpositions = xmlNewNode(NULL, "positions");
	struct ptb_position *position = positions;

	while(position) {
		xmlNodePtr xposition = xmlNewNode(NULL, "position");
		xmlAddChild(xpositions, xposition);

		SMART_ADD_PROP_INT(xposition, "offset", position->offset);
		SMART_ADD_CHILD_INT(xposition, "dots", position->dots);
		SMART_ADD_CHILD_INT(xposition, "length", position->length);
		SMART_ADD_CHILD_INT(xposition, "properties", position->properties);
		SMART_ADD_CHILD_INT(xposition, "fermenta", position->fermenta);

		xmlAddChild(xposition, xml_write_linedatas(position->linedatas));

		position = position->next;
	}
	return xpositions;
}

xmlNodePtr xml_write_staffs(struct ptb_staff *staffs)
{
	xmlNodePtr xstaffs = xmlNewNode(NULL, "staffs");
	struct ptb_staff *staff = staffs;

	while(staff) {
		int i;
		xmlNodePtr xstaff = xmlNewNode(NULL, "staff");
		xmlAddChild(xstaffs, xstaff);

		SMART_ADD_CHILD_INT(xstaff, "highest_note_space", staff->highest_note_space);
		SMART_ADD_CHILD_INT(xstaff, "lowest_note_space", staff->lowest_note_space);
		SMART_ADD_CHILD_INT(xstaff, "symbol_space", staff->symbol_space);
		SMART_ADD_CHILD_INT(xstaff, "tab_staff_space", staff->tab_staff_space);
		SMART_ADD_CHILD_INT(xstaff, "properties", staff->properties);

		for(i = 0; i < 2; i++) 
			xmlAddChild(xstaff, xml_write_positions(staff->positions[i]));
		
		staff = staff->next;
	}
	return xstaffs;
}

xmlNodePtr xml_write_sections(struct ptb_section *sections) 
{
	xmlNodePtr sctns = xmlNewNode(NULL, "sections");
	struct ptb_section *section = sections;

	while(section) {
		xmlNodePtr meter_type;
		xmlNodePtr xsection = xmlNewNode(NULL, "section");

		xmlAddChild(sctns, xsection);

		if(section->letter != 0x7f) {
		char tmp[100];
		snprintf(tmp, 100, "%c", section->letter);
		xmlSetProp(xsection, "letter", tmp);
		}

		switch(section->end_mark) {
		case END_MARK_TYPE_NORMAL:
			SMART_ADD_CHILD_STRING(xsection, "end-mark", "normal");
			break;
		case END_MARK_TYPE_REPEAT:
			SMART_ADD_CHILD_STRING(xsection, "end-mark", "repeat");
			break;
		}

		meter_type = xmlNewNode(NULL, "meter-type");
		xmlAddChild(xsection, meter_type);

		if(section->meter_type & METER_TYPE_BEAM_2) SMART_ADD_CHILD_STRING(meter_type, "beam_2", "");
		if(section->meter_type & METER_TYPE_BEAM_3) SMART_ADD_CHILD_STRING(meter_type, "beam_3", "");
		if(section->meter_type & METER_TYPE_BEAM_4) SMART_ADD_CHILD_STRING(meter_type, "beam_4", "");
		if(section->meter_type & METER_TYPE_BEAM_5) SMART_ADD_CHILD_STRING(meter_type, "beam_5", "");
		if(section->meter_type & METER_TYPE_BEAM_6) SMART_ADD_CHILD_STRING(meter_type, "beam_6", "");
		if(section->meter_type & METER_TYPE_COMMON) SMART_ADD_CHILD_STRING(meter_type, "common", "");
		if(section->meter_type & METER_TYPE_CUT) SMART_ADD_CHILD_STRING(meter_type, "cut", "");
		if(section->meter_type & METER_TYPE_SHOW) SMART_ADD_CHILD_STRING(meter_type, "show", "");

		SMART_ADD_CHILD_INT(xsection, "beat", section->detailed.beat);
		SMART_ADD_CHILD_INT(xsection, "beat-value", section->detailed.beat_value);
		SMART_ADD_CHILD_INT(xsection, "metronome-pulses-per-measure", section->metronome_pulses_per_measure);
		SMART_ADD_CHILD_INT(xsection, "properties", section->properties);
		SMART_ADD_CHILD_INT(xsection, "key-extra", section->key_extra);
		SMART_ADD_CHILD_INT(xsection, "position-width", section->position_width);
		SMART_ADD_CHILD_STRING(xsection, "description", section->description);

		xmlAddChild(xsection, xml_write_chordtexts(section->chordtexts));
		xmlAddChild(xsection, xml_write_rhythmslashes(section->rhythmslashes));
		xmlAddChild(xsection, xml_write_directions(section->directions));
		xmlAddChild(xsection, xml_write_staffs(section->staffs));

		xmlAddChild(xsection, xml_write_musicbars(section->musicbars));

		section = section->next;
	}

	return sctns;
}

xmlNodePtr xml_write_guitars(struct ptb_guitar *guitars, int instr) 
{
	xmlNodePtr gtrs = xmlNewNode(NULL, "guitars");
	struct ptb_guitar *gtr = guitars;

	while(gtr) {
		char tmp[100];
		int i;
		xmlNodePtr xgtr = xmlNewNode(NULL, "guitar");
		xmlNodePtr strings;
		xmlAddChild(gtrs, xgtr);

		snprintf(tmp, 100, "gtr-%d-%d", instr, gtr->index);
		xmlSetProp(xgtr, "id", tmp);

		strings = xmlNewNode(NULL, "tuning");
		xmlAddChild(xgtr, strings);

		for(i = 0; i < gtr->nr_strings; i++) {
			const char *notenames[] = { "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis", "a", "ais", "b" };
			xmlNodePtr string = xmlNewNode(NULL, "stringtuning");
			SMART_ADD_PROP_INT(string, "octave", gtr->strings[i]/12);
			xmlSetProp(string, "note", notenames[gtr->strings[i]%12]);
			xmlAddChild(strings, string);
		}

		SMART_ADD_CHILD_STRING(xgtr, "title", gtr->title);
		SMART_ADD_CHILD_STRING(xgtr, "type", gtr->type);
		SMART_ADD_CHILD_INT(xgtr, "reverb", gtr->reverb);
		SMART_ADD_CHILD_INT(xgtr, "chorus", gtr->chorus);
		SMART_ADD_CHILD_INT(xgtr, "tremolo", gtr->tremolo);
		SMART_ADD_CHILD_INT(xgtr, "pan", gtr->pan);
		SMART_ADD_CHILD_INT(xgtr, "capo", gtr->capo);
		SMART_ADD_CHILD_INT(xgtr, "initial_volume", gtr->initial_volume);
		SMART_ADD_CHILD_INT(xgtr, "midi_instrument", gtr->midi_instrument);
		SMART_ADD_CHILD_INT(xgtr, "half_up", gtr->half_up);
		SMART_ADD_CHILD_INT(xgtr, "simulate", gtr->simulate);

		gtr = gtr->next;
	}
	
	return gtrs;
}

xmlNodePtr xml_write_guitarins(struct ptb_guitarin *guitarins)
{
	struct ptb_guitarin *guitarin = guitarins;
	xmlNodePtr xguitarins = xmlNewNode(NULL, "guitarins");
	
	while(guitarin) {
		xmlNodePtr xguitarin = xmlNewNode(NULL, "guitarin");
		xmlAddChild(xguitarins, xguitarin);

		SMART_ADD_PROP_INT(xguitarin, "offset", guitarin->offset);
		SMART_ADD_PROP_INT(xguitarin, "section", guitarin->section);
		SMART_ADD_PROP_INT(xguitarin, "staff", guitarin->staff);
		SMART_ADD_CHILD_INT(xguitarin, "rhythm_slash", guitarin->rhythm_slash);
		SMART_ADD_CHILD_INT(xguitarin, "staff_in", guitarin->staff_in);

		guitarin = guitarin->next;
	}

	return xguitarins;
}

xmlNodePtr xml_write_tempomarkers(struct ptb_tempomarker *tempomarkers)
{
	struct ptb_tempomarker *tempomarker = tempomarkers;
	xmlNodePtr xtempomarkers = xmlNewNode(NULL, "tempomarkers");
	
	while(tempomarker) {
		xmlNodePtr xtempomarker = SMART_ADD_CHILD_STRING(xtempomarkers, "tempomarker", tempomarker->description);
		
		SMART_ADD_CHILD_INT(xtempomarker, "type", tempomarker->type);
		SMART_ADD_PROP_INT(xtempomarker, "section", tempomarker->section);
		SMART_ADD_PROP_INT(xtempomarker, "offset", tempomarker->offset);
		SMART_ADD_CHILD_INT(xtempomarker, "bpm", tempomarker->bpm);

		tempomarker = tempomarker->next;
	}

	return xtempomarkers;
}

xmlNodePtr xml_write_dynamics(struct ptb_dynamic *dynamics)
{
	struct ptb_dynamic *dynamic = dynamics;
	xmlNodePtr xdynamics = xmlNewNode(NULL, "dynamics");
	
	while(dynamic) {
		xmlNodePtr xdynamic = xmlNewNode(NULL, "dynamic");
		xmlAddChild(xdynamics, xdynamic);
		
		SMART_ADD_PROP_INT(xdynamic, "offset", dynamic->offset);

		dynamic = dynamic->next;
	}

	return xdynamics;
}

xmlNodePtr xml_write_chorddiagrams(struct ptb_chorddiagram *chorddiagrams)
{
	struct ptb_chorddiagram *chorddiagram = chorddiagrams;
	xmlNodePtr xchorddiagrams = xmlNewNode(NULL, "chorddiagrams");
	
	while(chorddiagram) {
		int i;
		xmlNodePtr xchorddiagram = xmlNewNode(NULL, "chorddiagram");
		xmlNodePtr strings = xmlNewNode(NULL, "strings");
		xmlAddChild(xchorddiagrams, xchorddiagram);
		xmlAddChild(xchorddiagram, strings);
		
		SMART_ADD_CHILD_STRING(xchorddiagram, "note1", ptb_get_tone(chorddiagram->name[0]));
		SMART_ADD_CHILD_STRING(xchorddiagram, "note2", ptb_get_tone(chorddiagram->name[1]));
		SMART_ADD_CHILD_INT(xchorddiagram, "frets", chorddiagram->frets);
		SMART_ADD_CHILD_INT(xchorddiagram, "type", chorddiagram->type);

		for(i = 0; i < chorddiagram->nr_strings; i++) {
			SMART_ADD_CHILD_INT(strings, "string", chorddiagram->tones[i]);
		}
		
		chorddiagram = chorddiagram->next;
	}

	return xchorddiagrams;
}

xmlNodePtr xml_write_sectionsymbols(struct ptb_sectionsymbol *sectionsymbols)
{
	struct ptb_sectionsymbol *sectionsymbol = sectionsymbols;
	xmlNodePtr xsectionsymbols = xmlNewNode(NULL, "sectionsymbols");
	
	while(sectionsymbol) {
		xmlNodePtr xsectionsymbol = xmlNewNode(NULL, "sectionsymbol");
		xmlAddChild(xsectionsymbols, xsectionsymbol);
		
		SMART_ADD_CHILD_INT(xsectionsymbol, "repeat-ending", sectionsymbol->repeat_ending);

		sectionsymbol = sectionsymbol->next;
	}

	return xsectionsymbols;
}

xmlNodePtr xml_write_floatingtexts(struct ptb_floatingtext *floatingtexts)
{
	struct ptb_floatingtext *floatingtext = floatingtexts;
	xmlNodePtr xfloatingtexts = xmlNewNode(NULL, "floatingtexts");
	
	while(floatingtext) {
		xmlNodePtr xfloatingtext = SMART_ADD_CHILD_STRING(xfloatingtexts, "floatingtext", floatingtext->text);
		
		SMART_ADD_PROP_INT(xfloatingtext, "offset", floatingtext->offset);

		switch(floatingtext->alignment) {
		case ALIGN_LEFT:
			SMART_ADD_CHILD_STRING(xfloatingtext, "alignment", "left");
			break;
		case ALIGN_RIGHT:
			SMART_ADD_CHILD_STRING(xfloatingtext, "alignment", "right");
			break;
		case ALIGN_CENTER:
			SMART_ADD_CHILD_STRING(xfloatingtext, "alignment", "center");
			break;
		}

		xmlAddChild(xfloatingtext, xml_write_font("font", &floatingtext->font));

		floatingtext = floatingtext->next;
	}

	return xfloatingtexts;
}

xmlNodePtr xml_write_instrument(struct ptbf *bf, int i)
{
	char tmp[100];
	xmlNodePtr instrument = xmlNewNode(NULL, "instrument");
	snprintf(tmp, 100, "instr-%d", i);
	xmlSetProp(instrument, "id", tmp);

	xmlAddChild(instrument, xml_write_guitars(bf->instrument[i].guitars, i));
	xmlAddChild(instrument, xml_write_sections(bf->instrument[i].sections));
	xmlAddChild(instrument, xml_write_guitarins(bf->instrument[i].guitarins));
	xmlAddChild(instrument, xml_write_chorddiagrams(bf->instrument[i].chorddiagrams));
	xmlAddChild(instrument, xml_write_tempomarkers(bf->instrument[i].tempomarkers));
	xmlAddChild(instrument, xml_write_dynamics(bf->instrument[i].dynamics));
	xmlAddChild(instrument, xml_write_floatingtexts(bf->instrument[i].floatingtexts));
	xmlAddChild(instrument, xml_write_sectionsymbols(bf->instrument[i].sectionsymbols));
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
	xmlNodePtr fonts;
	xmlDtdPtr dtd;
	int c, i, musicxml = 0;
	int version = 0;
	const char *input = NULL;
	char *output = NULL;
	poptContext pc;
	int quiet = 0;
	int format_output = 1;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"musicxml", 'm', POPT_ARG_NONE, &musicxml, 'm', "Output MusicXML" },
		{"no-format", 'f', POPT_ARG_NONE, &format_output, 0, "Don't format output" },
		{"quiet", 'q', POPT_ARG_NONE, &quiet, 1, "Be quiet (no output to stderr)" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2xml Version "PACKAGE_VERSION"\n");
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
	if (!quiet) fprintf(stderr, "Parsing %s...\n", input);
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
		output = malloc(baselength + 6);
		strncpy(output, input, baselength);
		strcpy(output + baselength, ".xml");
	}

	if (!quiet) fprintf(stderr, "Building DOM tree...\n");

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "powertab");
	dtd = xmlCreateIntSubset(doc, "powertab", NULL, DTD_URL);
	xmlDocSetRootElement(doc, root_node);

	comment = xmlNewComment("\nGenerated by ptb2xml, part of ptabtools. \n"
							"(C) 2004-2005 by Jelmer Vernooij <jelmer@samba.org>\n"
							"See http://jelmer.vernstok.nl/oss/ptabtools/ for details\n");
	xmlAddChild(root_node, comment);

	xmlAddChild(root_node, xml_write_header(&ret->hdr));

	for(i = 0; i < 2; i++) {
		xmlAddChild(root_node, xml_write_instrument(ret, i));
	}

	fonts = xmlNewNode( NULL, "fonts"); xmlAddChild(root_node, fonts);

	xmlAddChild(fonts, xml_write_font("default_font", &ret->default_font));
	xmlAddChild(fonts, xml_write_font("chord_name_font", &ret->chord_name_font));
	xmlAddChild(fonts, xml_write_font("tablature_font", &ret->tablature_font));

	if (musicxml)
	{
		if (!quiet) fprintf(stderr, "Converting to MusicXML...\n");
#ifdef HAVE_XSLT
		xsltStylesheetPtr stylesheet = xsltParseStylesheetFile(MUSICXMLSTYLESHEET);
		doc = xsltApplyStylesheet(stylesheet, doc, NULL);
		xsltFreeStylesheet(stylesheet);
#else
		fprintf(stderr, "Conversion to MusicXML not possible in this version: libxslt not compiled in\n");
		return -1;
#endif
	}

	if (!quiet) fprintf(stderr, "Writing output to %s...\n", output);

	if (xmlSaveFormatFile(output, doc, format_output) < 0) {
		return -1;
	}

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}
