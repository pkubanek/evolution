/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* string-util : utilities for normal gchar * strings  */

/* 
 *
 * Author : 
 *  Bertrand Guiheneuf <Bertrand.Guiheneuf@aful.org>
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com) .
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */



#ifndef STRING_UTIL_H
#define STRING_UTIL_H 1


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus }*/

#include <glib.h>

typedef enum {
	STRING_DICHOTOMY_NONE            =     0,
	STRING_DICHOTOMY_RIGHT_DIR       =     1,
	STRING_DICHOTOMY_STRIP_TRAILING  =     2,
	STRING_DICHOTOMY_STRIP_LEADING   =     4
	
} StringDichotomyOption;

typedef enum {
	STRING_TRIM_NONE            =     0,
	STRING_TRIM_STRIP_TRAILING  =     1,
	STRING_TRIM_STRIP_LEADING   =     2
} StringTrimOption;

gboolean string_equal_for_glist (gconstpointer v, gconstpointer v2);

gchar    string_dichotomy       (const gchar *string, gchar sep,
				 gchar **prefix, gchar **suffix,
				 StringDichotomyOption options);
void     string_list_free       (GList *string_list);

GList   *string_split           (const gchar *string, char sep,
				 const gchar *trim_chars, StringTrimOption trim_options);
void     string_trim            (gchar *string, const gchar *chars,
				 StringTrimOption options);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STRING_UTIL_H */
