/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>  
 *
 *
 * Authors:
 *		Jeffrey Stedfast <fejj@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifndef __FILTER_LABEL__
#define __FILTER_LABEL__

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#include "filter-option.h"

#define FILTER_TYPE_LABEL            (filter_label_get_type ())
#define FILTER_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILTER_TYPE_LABEL, FilterLabel))
#define FILTER_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FILTER_TYPE_LABEL, FilterLabelClass))
#define IS_FILTER_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILTER_TYPE_LABEL))
#define IS_FILTER_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FILTER_TYPE_LABEL))
#define FILTER_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FILTER_TYPE_LABEL, FilterLabelClass))

typedef struct _FilterLabel FilterLabel;
typedef struct _FilterLabelClass FilterLabelClass;

struct _FilterLabel {
	FilterOption parent_object;

};

struct _FilterLabelClass {
	FilterOptionClass parent_class;

	/* virtual methods */

	/* signals */
};

GType filter_label_get_type (void);

FilterLabel *filter_label_new (void);

/* Sigh, this is a mess, but its cleaner than the original mess */
int filter_label_count(void);
const char *filter_label_label(int i);
int filter_label_index(const char *label);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FILTER_LABEL__ */
