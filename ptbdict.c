/*
	(c) 2005: Jelmer Vernooij <jelmer@samba.org>

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

#include <string.h>

#include "ptb.h"

static void print_tuning(struct ptb_tuning *t, void *userdata)
{
	int j;
	printf("%s: ", t->name);
	for (j = 0; j < t->nr_strings; j++) {
		printf("%d ", t->strings[j]);
	}
	printf("\n");
}

static void del_tuning(struct ptb_tuning *t, void *userdata)
{
	struct ptb_tuning_dict *d = userdata;
	
	d->nr_tunings--;
	
	/* FIXME: Swap current and last item */
}

static void traverse_tunings(struct ptb_tuning_dict *ret, const char *name, void (*fn) (struct ptb_tuning *, void *userdata), void *userdata)
{
	int i;
	for (i = 0; i < ret->nr_tunings; i++) {
		if (name && strcmp(name, ret->tunings[i].name)) continue;

		fn (&ret->tunings[i], userdata);
	}
}

int main(int argc, const char **argv) 
{
	struct ptb_tuning_dict *ret;
	int c, version = 0;
	int del = 0, add =0;
	const char *tun;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		{"add", 'a', POPT_ARG_NONE, &add, 'a', "Add entry" },
		{"del", 'd', POPT_ARG_NONE, &del, 'd', "Delete entry" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "tunings.dat [tuning-name]");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptbdict Version "PACKAGE_VERSION"\n");
			printf("(C) 2005 Jelmer Vernooij <jelmer@samba.org>\n");
			exit(0);
			break;
		}
	}
			
	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}

	ret = ptb_read_tuning_dict(poptGetArg(pc));

	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	tun = poptGetArg(pc);

	if (add) {
		/* FIXME */
	} else {
		traverse_tunings(ret, tun, del?del_tuning:print_tuning, NULL);
	}

	return 0;
}
