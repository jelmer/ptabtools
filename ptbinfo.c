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
#include "dlinklist.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "ptb.h"

#define COND_PRINTF(desc,field) if(field) printf("%s: %s\n", desc, field);

#define PRINT_LIST(ls,t,fn) { t l = ls; while(l) { fn(l); l = l->next; } }


static const char *notenames[] = { "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis", "a", "ais", "b" };

void write_musicbar(struct ptb_musicbar *mb)
{
	printf("\t\tOffset: %d\n", mb->offset);
	printf("\t\tProperties: %d\n", mb->properties);
	printf("\t\tLetter: %c\n", mb->letter);
	if (mb->description) printf("\t\tDescription: %s\n", mb->description);
	printf("\n");
}

void write_linedata(struct ptb_linedata *ld)
{
	printf("\t\t\t\tFret: %d, String: %d\n", ld->detailed.fret, ld->detailed.string);
	if (ld->properties & LINEDATA_PROPERTY_TIE) printf("\t\t\t\tTie\n");
	if (ld->properties & LINEDATA_PROPERTY_MUTED) printf("\t\t\t\tMuted\n");
	if (ld->properties & LINEDATA_PROPERTY_CONTINUES) printf("\t\t\t\tContinues\n");
	if (ld->properties & LINEDATA_PROPERTY_HAMMERON_FROM) printf("\t\t\t\tStart of hammer-on\n");
	if (ld->properties & LINEDATA_PROPERTY_PULLOFF_FROM) printf("\t\t\t\tStart of pull-off\n");
	if (ld->properties & LINEDATA_PROPERTY_DEST_NOWHERE) printf("\t\t\t\tDest-nowhere(?)\n");
	if (ld->properties & LINEDATA_PROPERTY_NATURAL_HARMONIC) printf("\t\t\t\tNatural Harmonic\n");
	if (ld->properties & LINEDATA_PROPERTY_GHOST_NOTE) printf("\t\t\t\tGhost Note\n");

	if (ld->transcribe) {
		printf("\t\t\t\tTranscribed: ");
		if (ld->transcribe & LINEDATA_TRANSCRIBE_8VA) printf("8va ");
		if (ld->transcribe & LINEDATA_TRANSCRIBE_15MA) printf("15ma ");
		if (ld->transcribe & LINEDATA_TRANSCRIBE_8VB) printf("8vb ");
		if (ld->transcribe & LINEDATA_TRANSCRIBE_15MB) printf("15mb ");
		printf("\n");
	}

	printf("\n");
}

void write_position(struct ptb_position *pos)
{
	printf("\t\t\tOffset: %d\n", pos->offset);
	printf("\t\t\tLength: %d\n", pos->length);
	printf("\t\t\t    Attached data:\n");
	PRINT_LIST(pos->linedatas, struct ptb_linedata *, write_linedata);
	printf("\n");
}

void write_staff(struct ptb_staff *staff)
{
	printf("\t\tLowest Note Space: %d, Highest Note Space: %d\n", staff->lowest_note_space, staff->highest_note_space);
	printf("\t\tSymbol Space: %d, Tablature Staff Space: %d\n", staff->symbol_space, staff->tab_staff_space);
	printf("\t\tProperties: %d\n", staff->properties);

	printf("\t\t    Positions:\n");
	PRINT_LIST(staff->positions[0], struct ptb_position *, write_position);
	PRINT_LIST(staff->positions[0], struct ptb_position *, write_position);
	printf("\n");
}

void write_chordtext(struct ptb_chordtext *ct)
{
	printf("\t\tName: %d(%s)/%d(%s)\n", ct->name[1], notenames[ct->name[1]-16],
		   ct->name[0], notenames[ct->name[0]-16]);
	printf("\t\tOffset: %d\n", ct->offset);
	printf("\n");
}

void write_rhythmslash(struct ptb_rhythmslash *sl)
{
	printf("\t\tOffset: %d\n", sl->offset);
	printf("\t\tLength: %d\n", sl->length);
	printf("\n");
}

void write_direction(struct ptb_direction *dr)
{
	printf("\n");
}

void write_section(struct ptb_section *section)
{
	printf("\tSection(%c): %s\n", section->letter, section->description);
	printf("\tEnd-Mark: %d\n", section->end_mark);
	printf("\tMeter Type: %d\n", section->meter_type);
	printf("\tMetronome Pulses Per Measure: %d\n", section->metronome_pulses_per_measure);
	printf("\tProperties: %d\n", section->properties);
	printf("\tKey Extra: %d\n", section->key_extra);
	printf("\tPosition Width: %d\n", section->position_width);

	if (section->staffs) {
		printf("\t    Staffs:\n");
		PRINT_LIST(section->staffs, struct ptb_staff *, write_staff);
	}

	if (section->chordtexts) {
		printf("\t    Chordtexts:\n");
		PRINT_LIST(section->chordtexts, struct ptb_chordtext *, write_chordtext);
	}

	if (section->rhythmslashes) {
		printf("\t    Rhythmslashes:\n");
		PRINT_LIST(section->rhythmslashes, struct ptb_rhythmslash *, write_rhythmslash);
	}

	if (section->directions) {
		printf("\t    Directions:\n");
		PRINT_LIST(section->directions, struct ptb_direction *, write_direction);
	}
	
	if (section->musicbars) {
		printf("\t    Music Bars:\n");
		PRINT_LIST(section->musicbars, struct ptb_musicbar *, write_musicbar);
	}

	printf("\n");
}

void write_sectionsymbol(struct ptb_sectionsymbol *ssb)
{
	printf("\tRepeat Ending: %d\n", ssb->repeat_ending);
	printf("\n");
}

void write_tempomarker(struct ptb_tempomarker *tm)
{
	printf("\tDescription: %s\n", tm->description);
	printf("\tType: %d\n", tm->type);
	printf("\tBPM: %d\n", tm->bpm);
	printf("\tSection: %d\n", tm->section);
	printf("\tOffset: %d\n", tm->offset);
	printf("\n");
}

void write_dynamic(struct ptb_dynamic *dn)
{
	printf("\tOffset: %d\n", dn->offset);
	printf("\tStaff: %d\n", dn->staff);
	printf("\tVolume: %d\n", dn->volume);
	printf("\n");
}

void write_guitar(struct ptb_guitar *gtr)
{
	int i;
	printf("\tNumber: %d\n", gtr->index);
	printf("\tTitle: %s\n", gtr->title);
	printf("\tType: %s\n", gtr->type);
	printf("\tStrings(%d):\n", gtr->nr_strings);
	for (i = 0; i < gtr->nr_strings; i++) 
		printf("\t\t%s at octave %d (%d)\n", notenames[gtr->strings[i]%12], gtr->strings[i]/12, gtr->strings[i]);

	printf("\tReverb: %d\n", gtr->reverb);
	printf("\tChorus: %d\n", gtr->chorus);
	printf("\tTremolo: %d\n", gtr->chorus);
	printf("\tPan: %d\n", gtr->pan);
	printf("\tCapo on fret: %d\n", gtr->capo);
	printf("\tInitial volume: %d\n", gtr->initial_volume);
	printf("\tMidi Instrument: %d\n", gtr->midi_instrument);
	printf("\tHalf up(?): %d\n", gtr->half_up);
	printf("\tSimulate (?): %d\n", gtr->simulate);
	printf("\n");
}

void write_guitarin(struct ptb_guitarin *gtr)
{
	printf("\tOffset: %d\n", gtr->offset);
	printf("\tSection: %d\n", gtr->section);
	printf("\tStaff: %d\n", gtr->staff);
	printf("\tRhythmslash: %d\n", gtr->rhythm_slash);
	printf("\tStaff In(?): %d\n", gtr->staff_in);
	printf("\n");
}

void write_chorddiagram(struct ptb_chorddiagram *chd)
{
	int i;
	printf("\tName: %c%c\n", chd->name.name[0], chd->name.name[1]);
	printf("\tFret Offset: %d\n", chd->frets);
	printf("\tType: %d\n", chd->type);
	printf("\tTones(%d): \n", chd->nr_strings);
	for (i = 0; i < chd->nr_strings; i++) 
		printf("\t\t%d\n", chd->tones[i]);
	printf("\n");
}

void write_font(struct ptb_font *font)
{
	printf("%s, Size: %d, Weight: %d, Underlined: %d, Italic: %d", font->family, font->pointsize, font->weight, font->underlined, font->italic);
}

void write_floatingtext(struct ptb_floatingtext *ft)
{
	printf("\tText: %s\n", ft->text);
	printf("\tAlignment: ");
	switch (ft->alignment & (ALIGN_LEFT|ALIGN_CENTER|ALIGN_RIGHT)) {
	case ALIGN_LEFT: printf("Left");break;
	case ALIGN_CENTER: printf("Center"); break;
	case ALIGN_RIGHT: printf("Right"); break;
	}
	if (ft->alignment & ALIGN_BORDER) printf(", Print Border");
	printf("\n");
	printf("\tFont: "); write_font(&ft->font); printf("\n");
	printf("\n");
}


void write_praudio_info(struct ptb_hdr *hdr)
{
	printf("Audio Type: ");
	switch(hdr->class_info.song.release_info.pr_audio.type)
	{ 
	case AUDIO_TYPE_SINGLE: printf("Single\n"); break;
	case AUDIO_TYPE_EP: printf("EP\n"); break;
	case AUDIO_TYPE_ALBUM: printf("Album\n"); break;
	case AUDIO_TYPE_DOUBLE_ALBUM: printf("Double Album\n"); break;
	case AUDIO_TYPE_TRIPLE_ALBUM: printf("Triple Album\n"); break;
	case AUDIO_TYPE_BOXSET: printf("Boxset\n"); break;
	}
	COND_PRINTF("Album Title", hdr->class_info.song.release_info.pr_audio.album_title);
	printf("Year: %d\n", hdr->class_info.song.release_info.pr_audio.year);
	printf("Live recording? %s\n", hdr->class_info.song.release_info.pr_audio.is_live_recording?"Yes":"No");
}

void write_prvideo_info(struct ptb_hdr *hdr)
{
	COND_PRINTF("Video Title", hdr->class_info.song.release_info.pr_video.video_title);
	printf("Year: %d\n", hdr->class_info.song.release_info.pr_video.year);
	printf("Live recording? %s\n", hdr->class_info.song.release_info.pr_video.is_live_recording?"Yes":"No");
}

void write_bootleg_info(struct ptb_hdr *hdr)
{
	COND_PRINTF("Bootleg Title", hdr->class_info.song.release_info.bootleg.title);
	printf("Date: %d-%d-%d\n", 
		   hdr->class_info.song.release_info.bootleg.day,
		   hdr->class_info.song.release_info.bootleg.month,
		   hdr->class_info.song.release_info.bootleg.year);
}

void write_song_info(struct ptb_hdr *hdr)
{
	COND_PRINTF("Title", hdr->class_info.song.title);
	COND_PRINTF("Artist", hdr->class_info.song.artist);
	COND_PRINTF("Words By", hdr->class_info.song.words_by);
	COND_PRINTF("Music By", hdr->class_info.song.music_by);
	COND_PRINTF("Arranged By", hdr->class_info.song.arranged_by);
	COND_PRINTF("Guitar Transcribed By", hdr->class_info.song.guitar_transcribed_by);
	COND_PRINTF("Bass Transcribed By", hdr->class_info.song.bass_transcribed_by);
	if(hdr->class_info.song.lyrics) 
		printf("Lyrics\n----------\n%s\n\n", hdr->class_info.song.lyrics);
	COND_PRINTF("Copyright", hdr->class_info.song.copyright);

	printf("Release type: ");
	switch(hdr->class_info.song.release_type) {
	case RELEASE_TYPE_PR_AUDIO:
		printf("Public Release (Audio)\n");
		write_praudio_info(hdr);
		break;
	case RELEASE_TYPE_PR_VIDEO:
		printf("Public Release (Video)\n");
		write_prvideo_info(hdr);
		break;
	case RELEASE_TYPE_BOOTLEG:
		printf("Bootleg\n");
		write_bootleg_info(hdr);
		break;
	case RELEASE_TYPE_UNRELEASED:
		printf("Unreleased\n");
		break;
	}
}

void write_lesson_info(struct ptb_hdr *hdr)
{
	COND_PRINTF("Title", hdr->class_info.lesson.title);
	COND_PRINTF("Artist", hdr->class_info.lesson.artist);
	printf("Style: %d\n", hdr->class_info.lesson.style);
	printf("Level: ");
	switch(hdr->class_info.lesson.level) {
	case LEVEL_BEGINNER: printf("Beginner"); break;
	case LEVEL_INTERMEDIATE: printf("Intermediate"); break;
	case LEVEL_ADVANCED: printf("Advanced"); break;
	}
	COND_PRINTF("Author", hdr->class_info.lesson.author);
	COND_PRINTF("Copyright", hdr->class_info.lesson.copyright);
}

int main(int argc, const char **argv) 
{
	struct ptbf *ret;
	int tree = 0;
	int debugging = 0;
	int c, tmp1, tmp2;
	int version = 0;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"tree", 't', POPT_ARG_NONE, &tree, 't', "Print tree of PowerTab file" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptbinfo Version "PACKAGE_VERSION"\n");
			printf("(C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
			exit(0);
			break;
		}
	}
			
	ptb_set_debug(debugging);
	ptb_set_asserts_fatal(0);
	
	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}
	ret = ptb_read_file(poptGetArg(pc));
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	printf("File type: ");
	switch(ret->hdr.classification) {
	case CLASSIFICATION_SONG: 
		printf("Song\n");
		write_song_info(&ret->hdr);
		break;
	case CLASSIFICATION_LESSON:
		printf("Lesson\n");
		write_lesson_info(&ret->hdr);
		break;
	default: 
		printf("Unknown\n");
		break;
	}

	DLIST_LEN(ret->instrument[0].sections, tmp1, struct ptb_section *);
	DLIST_LEN(ret->instrument[1].sections, tmp2, struct ptb_section *);
	printf("Number of sections: \tRegular: %d Bass: %d\n", tmp1, tmp2);

	DLIST_LEN(ret->instrument[0].guitars, tmp1, struct ptb_guitar *);
	DLIST_LEN(ret->instrument[1].guitars, tmp2, struct ptb_guitar *);

	printf("Number of guitars: \tRegular: %d Bass: %d\n", tmp1, tmp2);

	if (tree) 
	{
		int i;
		printf("\n");
		for (i = 0; i < 2; i++) {
			printf("\n");
			if (i == 0) printf("Guitar\n"); else printf("Bass\n");

			if (ret->instrument[i].guitars) {
				printf("    Guitars:\n");
				PRINT_LIST(ret->instrument[i].guitars, struct ptb_guitar *, write_guitar);
			}

			if (ret->instrument[i].guitarins) {
				printf("    GuitarIns:\n");
				PRINT_LIST(ret->instrument[i].guitarins, struct ptb_guitarin *, write_guitarin);
			}

			if (ret->instrument[i].chorddiagrams) {
				printf("    Chord Diagrams:\n");
				PRINT_LIST(ret->instrument[i].chorddiagrams, struct ptb_chorddiagram *, write_chorddiagram);
			}

			if (ret->instrument[i].tempomarkers) {
				printf("    Tempo Markers:\n");
				PRINT_LIST(ret->instrument[i].tempomarkers, struct ptb_tempomarker *, write_tempomarker);
			}

			if (ret->instrument[i].dynamics) {
				printf("    Dynamics:\n");
				PRINT_LIST(ret->instrument[i].dynamics, struct ptb_dynamic *, write_dynamic);
			}

			if (ret->instrument[i].floatingtexts) {
				printf("    Floating Texts:\n");
				PRINT_LIST(ret->instrument[i].floatingtexts, struct ptb_floatingtext *, write_floatingtext);
			}

			if (ret->instrument[i].sectionsymbols) {
				printf("    Section Symbols:\n");
				PRINT_LIST(ret->instrument[i].sectionsymbols, struct ptb_sectionsymbol *, write_sectionsymbol);
			}

			if (ret->instrument[i].sections) {
				printf("    Sections:\n");
				PRINT_LIST(ret->instrument[i].sections, struct ptb_section *, write_section);
			}
		}

		printf("Default Font: "); write_font(&ret->default_font); printf("\n");
		printf("Chord Name Font: "); write_font(&ret->chord_name_font); printf("\n");
		printf("Tablature Font: "); write_font(&ret->tablature_font); printf("\n");
	}

	ptb_free(ret);

	return (ret?0:1);
}
