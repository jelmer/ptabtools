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

xmlNodePtr xml_write_part_list(struct ptbf *ptb)
{
	return NULL;
}

xmlNodePtr xml_write_identification(struct ptb_hdr *hdr)
{
	time_t t;
	xmlNodePtr identification = xmlNewNode(NULL, "identification");
	xmlNodePtr software = xmlNewNode(NULL, "software");
	xmlNodePtr encoding_date = xmlNewNode(NULL,"encoding-date");
	xmlNodePtr misc = xmlNewNode(NULL, "miscellaneous");
	
	xmlNodeSetContent(software, "ptb2musicxml "PTB_VERSION);
	t = time(NULL);
	xmlNodeSetContent(encoding_date, ctime(&t));

	xmlAddChild(identification, software);
	xmlAddChild(identification, encoding_date);
	xmlAddChild(identification, misc);

	/* FIXME */	
	
	return identification;
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
	root_node = xmlNewNode(NULL, BAD_CAST "score-partwise");
	xmlDocSetRootElement(doc, root_node);

	xmlAddChild(root_node, xml_write_identification(&ret->hdr));
	xmlAddChild(root_node, xml_write_part_list(ret));

	dtd = xmlCreateIntSubset(doc, BAD_CAST "root", BAD_CAST "-//Recordare//DTD MusicXML 1.0 Partwise//EN", BAD_CAST "http://www.musicxml.org/dtds/partwise.dtd");
		
	xmlSaveFormatFileEnc(output?output:"-", doc, "UTF-8", 1);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}
