/*
    testsuite for ptabtools
    (c) 2007 Jelmer Vernooij <jelmer@samba.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ptb.h"

START_TEST(test_get_step)
END_TEST

START_TEST(test_get_tone)
	fail_unless(strcmp(ptb_get_tone(18), "D") == 0, "got %s", ptb_get_tone(18));
	fail_unless(strcmp(ptb_get_tone(17), "C#") == 0, "got %s", ptb_get_tone(17));
END_TEST

START_TEST(test_get_tone_empty)
	fail_unless(strcmp(ptb_get_tone(1), "") == 0, "got %s", ptb_get_tone(1));
END_TEST

START_TEST(test_get_tone_invalid)
	fail_unless(strcmp(ptb_get_tone(30), "_UNKNOWN_CHORD_") == 0, "got %s", ptb_get_tone(30));
END_TEST


START_TEST(test_get_tone_full)
	fail_unless(strcmp(ptb_get_tone_full(18), "d") == 0, "got %s", ptb_get_tone_full(18));
	fail_unless(strcmp(ptb_get_tone_full(17), "cis") == 0, "got %s", ptb_get_tone_full(17));
END_TEST

START_TEST(test_get_tone_full_empty)
	fail_unless(strcmp(ptb_get_tone_full(1), "_UNKNOWN_CHORD_") == 0, "got %s", ptb_get_tone_full(1));
END_TEST

START_TEST(test_get_tone_full_invalid)
	fail_unless(strcmp(ptb_get_tone_full(15), "_UNKNOWN_CHORD_") == 0, "got %s", ptb_get_tone_full(15));
END_TEST

Suite *ptb_suite()
{
	Suite *s = suite_create("ptb");
	TCase *tc_core = tcase_create("core");
	suite_add_tcase(s, tc_core);
	tcase_add_test(tc_core, test_get_step);
	tcase_add_test(tc_core, test_get_tone);
	tcase_add_test(tc_core, test_get_tone_empty);
	tcase_add_test(tc_core, test_get_tone_invalid);
	tcase_add_test(tc_core, test_get_tone_full);
	tcase_add_test(tc_core, test_get_tone_full_empty);
	tcase_add_test(tc_core, test_get_tone_full_invalid);
	return s;
}
