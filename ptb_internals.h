/*
   (c) 2004 Jelmer Vernooij <jelmer@samba.org>

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

#ifndef __PTB_INTERNALS_H__
#define __PTB_INTERNALS_H__

#include "ptb.h"

ssize_t ptb_read_string(struct ptbf *, char **);
ssize_t ptb_read(struct ptbf *, void *data, size_t len);
ssize_t ptb_read_unknown(struct ptbf *, size_t len);
ssize_t ptb_read_items(struct ptbf *bf);
ssize_t ptb_read_font(struct ptbf *, struct ptb_font *);
ssize_t ptb_read_stuff(struct ptbf *);

struct ptb_section_handler {
	char *name;
	ssize_t (*handler) (struct ptbf *, const char *section);
	int index;
};

extern struct ptb_section_handler ptb_section_handlers[];

void ptb_debug(const char *fmt, ...);

#define read DONT_USE_READ

#endif /* __PTB_H__ */
