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

#define COND_PRINTF(desc,field) if(field) printf("%s: %s\n", desc, field);

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
	int debugging = 0;
	int c;
	int version = 0;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptbinfo Version "PTB_VERSION"\n");
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

	printf("Number of sections: \tRegular: %d Bass: %d\n", 
		   g_list_length(ret->instrument[0].sections),
		   g_list_length(ret->instrument[1].sections)
		   );
	printf("Number of guitars: \tRegular: %d Bass: %d\n", 
		   g_list_length(ret->instrument[0].guitars),
		   g_list_length(ret->instrument[1].guitars));

	return (ret?0:1);
}
