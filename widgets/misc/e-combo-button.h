/*
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
 *		Ettore Perazzoli <ettore@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifndef _E_COMBO_BUTTON_H_
#define _E_COMBO_BUTTON_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define E_TYPE_COMBO_BUTTON			(e_combo_button_get_type ())
#define E_COMBO_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), E_TYPE_COMBO_BUTTON, EComboButton))
#define E_COMBO_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), E_TYPE_COMBO_BUTTON, EComboButtonClass))
#define E_IS_COMBO_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), E_TYPE_COMBO_BUTTON))
#define E_IS_COMBO_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((obj), E_TYPE_COMBO_BUTTON))


typedef struct _EComboButton        EComboButton;
typedef struct _EComboButtonPrivate EComboButtonPrivate;
typedef struct _EComboButtonClass   EComboButtonClass;

struct _EComboButton {
	GtkButton parent;

	EComboButtonPrivate *priv;
};

struct _EComboButtonClass {
	GtkButtonClass parent_class;

	/* Signals.  */
	void (* activate_default) (EComboButton *combo_button);
};


GType      e_combo_button_get_type   (void);
void       e_combo_button_construct  (EComboButton *combo_button);
GtkWidget *e_combo_button_new        (void);

void       e_combo_button_set_icon   (EComboButton *combo_button,
				      GdkPixbuf    *pixbuf);
void       e_combo_button_set_label  (EComboButton *combo_button,
				      const char   *label);
void       e_combo_button_set_menu   (EComboButton *combo_button,
				      GtkMenu      *menu);
void       e_combo_button_pack_vbox  (EComboButton *combo_button);
void       e_combo_button_pack_hbox  (EComboButton *combo_button);

GtkWidget *e_combo_button_get_label  (EComboButton *combo_button);

gboolean   e_combo_button_popup_menu (EComboButton *combo_button);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _E_COMBO_BUTTON_H_ */
