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
 *		JP Rosevear <jpr@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifndef _E_DAY_VIEW_CONFIG_H_
#define _E_DAY_VIEW_CONFIG_H_

#include "e-day-view.h"

G_BEGIN_DECLS

#define E_DAY_VIEW_CONFIG(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, e_day_view_config_get_type (), EDayViewConfig)
#define E_DAY_VIEW_CONFIG_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, e_day_view_config_get_type (), EDayViewConfigClass)
#define E_IS_DAY_VIEW_CONFIG(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, e_day_view_config_get_type ())

typedef struct _EDayViewConfig        EDayViewConfig;
typedef struct _EDayViewConfigClass   EDayViewConfigClass;
typedef struct _EDayViewConfigPrivate EDayViewConfigPrivate;

struct _EDayViewConfig {
	GObject parent;

	EDayViewConfigPrivate *priv;
};

struct _EDayViewConfigClass {
	GObjectClass parent_class;
};

GType          e_day_view_config_get_type (void);
EDayViewConfig *e_day_view_config_new (EDayView *day_view);
EDayView *e_day_view_config_get_view (EDayViewConfig *view_config);
void e_day_view_config_set_view (EDayViewConfig *view_config, EDayView *day_view);

G_END_DECLS

#endif
