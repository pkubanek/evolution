/*
 * e-editor-dialog.h
 *
<<<<<<< HEAD
 * Copyright (C) 2012 Dan Vrátil <dvratil@redhat.com>
 *
=======
>>>>>>> Refactor EEditorDialog... classes
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
 */

#if !defined (__E_UTIL_H_INSIDE__) && !defined (LIBEUTIL_COMPILATION)
#error "Only <e-util/e-util.h> should be included directly."
#endif

#ifndef E_EDITOR_DIALOG_H
#define E_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include <e-util/e-editor.h>

/* Standard GObject macros */
#define E_TYPE_EDITOR_DIALOG \
	(e_editor_dialog_get_type ())
#define E_EDITOR_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_EDITOR_DIALOG, EEditorDialog))
#define E_EDITOR_DIALOG_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_EDITOR_DIALOG, EEditorDialogClass))
#define E_IS_EDITOR_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_EDITOR_DIALOG))
#define E_IS_EDITOR_DIALOG_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_EDITOR_DIALOG))
#define E_EDITOR_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_EDITOR_DIALOG, EEditorDialogClass))

G_BEGIN_DECLS

typedef struct _EEditorDialog EEditorDialog;
typedef struct _EEditorDialogClass EEditorDialogClass;
typedef struct _EEditorDialogPrivate EEditorDialogPrivate;

struct _EEditorDialog {
	GtkWindow parent;
<<<<<<< HEAD
=======

>>>>>>> Refactor EEditorDialog... classes
	EEditorDialogPrivate *priv;
};

struct _EEditorDialogClass {
	GtkWindowClass parent_class;
};

<<<<<<< HEAD
GType		e_editor_dialog_get_type	(void) G_GNUC_CONST;
EEditor *	e_editor_dialog_get_editor	(EEditorDialog *dialog);
GtkBox *	e_editor_dialog_get_button_box	(EEditorDialog *dialog);
GtkGrid *	e_editor_dialog_get_container	(EEditorDialog *dialog);
=======
GType		e_editor_dialog_get_type	(void);

EEditor *	e_editor_dialog_get_editor	(EEditorDialog *dialog);
>>>>>>> Refactor EEditorDialog... classes

G_END_DECLS

#endif /* E_EDITOR_DIALOG_H */
