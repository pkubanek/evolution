/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* e-shortcuts.c
 *
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ettore Perazzoli
 */

/* The shortcut list goes like this:

   <?xml version="1.0"?>
   <shortcuts>
           <group title="Evolution shortcuts">
	           <item>evolution:/local/Inbox</item>
	           <item>evolution:/local/Trash</item>
	           <item>evolution:/local/Calendar</item>
	   </group>

	   <group title="Personal shortcuts">
	           <item>evolution:/local/Personal</item>
	   </group>
   </shortcuts>

   FIXME: Do we want to use a namespace for this?
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkobject.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktypeutils.h>

#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gal/util/e-util.h>
#include <gal/util/e-xml-utils.h>
#include <gal/shortcut-bar/e-shortcut-bar.h>

#include "e-shortcuts-view.h"

#include "e-shortcuts.h"
#include "e-shell-constants.h"


#define PARENT_TYPE GTK_TYPE_OBJECT
static GtkObjectClass *parent_class = NULL;

struct _ShortcutGroup {
	/* Title of the group.  */
	char *title;

	/* A list of strings with the URI for the shortcut.  */
	GList *shortcuts;
};
typedef struct _ShortcutGroup ShortcutGroup;

struct _EShortcutsPrivate {
	/* Name of the file associated with these shortcuts.  Changes in the shortcuts
           will update this file automatically.  */
	char *file_name;

	/* ID of the idle function that will be called to save the shortcuts when they are
           changed.  */
	int save_idle_id;

	/* Whether these shortcuts need to be saved to disk.  */
	gboolean dirty;

	/* The storage set to which these shortcuts are associated.  */
	EStorageSet *storage_set;

	/* The folder type registry.  */
	EFolderTypeRegistry *folder_type_registry;

	/* A list of ShortcutGroups.  */
	GList *groups;

	/* A list of ShortcutViews.  */
	GList *views;

	/* A hash table to get a group given its name.  */
	GHashTable *title_to_group;
};

enum {
	NEW_GROUP,
	REMOVE_GROUP,
	NEW_SHORTCUT,
	REMOVE_SHORTCUT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


static void
unload_shortcuts (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;
	GList *orig_groups;
	GList *p, *q;

	priv = shortcuts->priv;
	orig_groups = priv->groups;

	for (p = priv->groups; p != NULL; p = p->next) {
		ShortcutGroup *group;

		gtk_signal_emit (GTK_OBJECT (shortcuts), signals[REMOVE_GROUP], 0);

		group = (ShortcutGroup *) p->data;

		g_hash_table_remove (priv->title_to_group, group->title);

		for (q = group->shortcuts; q != NULL; q = q->next)
			g_free (q->data);
		g_free (group->title);

		g_list_free (group->shortcuts);

		priv->groups = priv->groups->next;
	}

	if (orig_groups != NULL)
		g_list_free (orig_groups);

	priv->groups = NULL;

	g_hash_table_destroy (priv->title_to_group);
	priv->title_to_group = g_hash_table_new (g_str_hash, g_str_equal);
}

static gboolean
load_shortcuts (EShortcuts *shortcuts,
		const char *file_name)
{
	EShortcutsPrivate *priv;
	xmlDoc *doc;
	xmlNode *root;
	xmlNode *p, *q;

	/* FIXME: Update the views by emitting the appropriate signals.  */

	priv = shortcuts->priv;

	doc = xmlParseFile (file_name);
	if (doc == NULL)
		return FALSE;

	root = xmlDocGetRootElement (doc);
	if (root == NULL || strcmp (root->name, "shortcuts") != 0) {
		xmlFreeDoc (doc);
		return FALSE;
	}

	unload_shortcuts (shortcuts);

	for (p = root->childs; p != NULL; p = p->next) {
		ShortcutGroup *shortcut_group;
		char *shortcut_group_title;

		if (strcmp ((char *) p->name, "group") != 0)
			continue;

		shortcut_group_title = (char *) xmlGetProp (p, "title");
		if (shortcut_group_title == NULL)
			continue;

		shortcut_group = g_hash_table_lookup (priv->title_to_group,
						      shortcut_group_title);
		if (shortcut_group != NULL) {
			g_warning ("Duplicate shortcut group title -- %s",
				   shortcut_group_title);
			xmlFree (shortcut_group_title);
			continue;
		}

		shortcut_group = g_new (ShortcutGroup, 1);
		shortcut_group->title = g_strdup (shortcut_group_title);
		xmlFree (shortcut_group_title);

		shortcut_group->shortcuts = NULL;
		for (q = p->childs; q != NULL; q = q->next) {
			char *content;

			if (strcmp ((char *) q->name, "item") != 0)
				continue;

			content = xmlNodeListGetString (doc, q->childs, 1);
			shortcut_group->shortcuts = g_list_prepend (shortcut_group->shortcuts,
								    g_strdup (content));
			xmlFree (content);
		}
		shortcut_group->shortcuts = g_list_reverse (shortcut_group->shortcuts);

		priv->groups = g_list_prepend (priv->groups, shortcut_group);
		g_hash_table_insert (priv->title_to_group, shortcut_group->title, shortcut_group);
	}

	priv->groups = g_list_reverse (priv->groups);

	xmlFreeDoc (doc);

	return TRUE;
}

static gboolean
save_shortcuts (EShortcuts *shortcuts,
		const char *file_name)
{
	EShortcutsPrivate *priv;
	xmlDoc *doc;
	xmlNode *root;
	GList *p, *q;

	priv = shortcuts->priv;

	doc = xmlNewDoc ((xmlChar *) "1.0");
	root = xmlNewDocNode (doc, NULL, (xmlChar *) "shortcuts", NULL);
	xmlDocSetRootElement (doc, root);

	for (p = priv->groups; p != NULL; p = p->next) {
		ShortcutGroup *group;
		xmlNode *group_node;

		group = (ShortcutGroup *) p->data;
		group_node = xmlNewChild (root, NULL, (xmlChar *) "group", NULL);

		xmlSetProp (group_node, (xmlChar *) "title", group->title);

		for (q = group->shortcuts; q != NULL; q = q->next) {
			const char *shortcut;

			shortcut = (const char *) q->data;
			xmlNewChild (group_node, NULL, (xmlChar *) "item", (xmlChar *) shortcut);
		}
	}

	if (xmlSaveFile (file_name, doc) < 0) {
		xmlFreeDoc (doc);
		return FALSE;
	}

	xmlFreeDoc (doc);
	return TRUE;
}


/* Idle function to update the file on disk.  */

static int
idle_cb (void *data)
{
	EShortcuts *shortcuts;
	EShortcutsPrivate *priv;

	shortcuts = E_SHORTCUTS (data);
	priv = shortcuts->priv;

	if (priv->dirty) {
		g_print ("Saving shortcuts -- %s\n", priv->file_name);
		if (! e_shortcuts_save (shortcuts, NULL))
			g_warning ("Saving of shortcuts failed -- %s", priv->file_name);
		else
			priv->dirty = FALSE;
	}

	priv->save_idle_id = 0;

	return FALSE;
}

static void
schedule_idle (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;

	priv = shortcuts->priv;

	if (priv->save_idle_id != 0)
		return;

	gtk_idle_add (idle_cb, shortcuts);
}

static void
make_dirty (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;

	priv = shortcuts->priv;

	priv->dirty = TRUE;
	schedule_idle (shortcuts);
}

/* Signal handlers for the storage set */
static void
removed_folder_cb (EStorageSet *storage_set,
		   const char  *path,
		   void        *data)
{
	EShortcuts *shortcuts;
	char *tmp;

	shortcuts = E_SHORTCUTS (data);

	tmp = g_strconcat (E_SHELL_URI_PREFIX, path, NULL);
	e_shortcuts_remove_shortcut_by_uri (shortcuts, tmp);
	g_free (tmp);
}


/* Signal handlers for the views.  */

static void
view_destroyed_cb (GtkObject *object,
		   gpointer data)
{
	EShortcuts *shortcuts;
	EShortcutsPrivate *priv;

	shortcuts = E_SHORTCUTS (data);
	priv = shortcuts->priv;

	priv->views = g_list_remove (priv->views, object);
}


/* GtkObject methods.  */

static void
destroy (GtkObject *object)
{
	EShortcuts *shortcuts;
	EShortcutsPrivate *priv;

	shortcuts = E_SHORTCUTS (object);
	priv = shortcuts->priv;

	g_free (priv->file_name);

	if (priv->storage_set != NULL)
		gtk_object_unref (GTK_OBJECT (priv->storage_set));

	if (priv->folder_type_registry != NULL)
		gtk_object_unref (GTK_OBJECT (priv->folder_type_registry));

	unload_shortcuts (shortcuts);

	if (priv->save_idle_id != 0)
		gtk_idle_remove (priv->save_idle_id);

	if (priv->dirty) {
		if (! e_shortcuts_save (shortcuts, NULL))
			g_warning (_("Error saving shortcuts.")); /* FIXME */
	}

	g_hash_table_destroy (priv->title_to_group);

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
class_init (EShortcutsClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) klass;
	object_class->destroy = destroy;

	parent_class = gtk_type_class (gtk_object_get_type ());

	signals[NEW_GROUP]
		= gtk_signal_new ("new_group",
				  GTK_RUN_FIRST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (EShortcutsClass, new_group),
				  gtk_marshal_NONE__INT,
				  GTK_TYPE_NONE, 1,
				  GTK_TYPE_INT);

	signals[REMOVE_GROUP]
		= gtk_signal_new ("remove_group",
				  GTK_RUN_FIRST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (EShortcutsClass, remove_group),
				  gtk_marshal_NONE__INT,
				  GTK_TYPE_NONE, 1,
				  GTK_TYPE_INT);

	signals[NEW_SHORTCUT]
		= gtk_signal_new ("new_shortcut",
				  GTK_RUN_FIRST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (EShortcutsClass, new_shortcut),
				  gtk_marshal_NONE__INT_INT,
				  GTK_TYPE_NONE, 2,
				  GTK_TYPE_INT,
				  GTK_TYPE_INT);

	signals[REMOVE_SHORTCUT]
		= gtk_signal_new ("remove_shortcut",
				  GTK_RUN_FIRST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (EShortcutsClass, remove_shortcut),
				  gtk_marshal_NONE__INT_INT,
				  GTK_TYPE_NONE, 2,
				  GTK_TYPE_INT,
				  GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}


static void
init (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;

	priv = g_new (EShortcutsPrivate, 1);

	priv->file_name      = NULL;
	priv->storage_set    = NULL;
	priv->groups         = NULL;
	priv->views          = NULL;
	priv->title_to_group = g_hash_table_new (g_str_hash, g_str_equal);
	priv->dirty          = 0;
	priv->save_idle_id   = 0;

	shortcuts->priv = priv;
}


void
e_shortcuts_construct (EShortcuts  *shortcuts,
		       EStorageSet *storage_set,
		       EFolderTypeRegistry *folder_type_registry)
{
	EShortcutsPrivate *priv;

	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));
	g_return_if_fail (storage_set != NULL);
	g_return_if_fail (E_IS_STORAGE_SET (storage_set));

	GTK_OBJECT_UNSET_FLAGS (GTK_OBJECT (shortcuts), GTK_FLOATING);

	priv = shortcuts->priv;

	gtk_object_ref (GTK_OBJECT (storage_set));
	priv->storage_set = storage_set;

	gtk_signal_connect (GTK_OBJECT (priv->storage_set), "removed_folder",
			    removed_folder_cb, shortcuts);

	gtk_object_ref (GTK_OBJECT (folder_type_registry));
	priv->folder_type_registry = folder_type_registry;
}

EShortcuts *
e_shortcuts_new (EStorageSet *storage_set,
		 EFolderTypeRegistry *folder_type_registry,
		 const char *file_name)
{
	EShortcuts *new;

	g_return_val_if_fail (storage_set != NULL, NULL);
	g_return_val_if_fail (E_IS_STORAGE_SET (storage_set), NULL);

	new = gtk_type_new (e_shortcuts_get_type ());
	e_shortcuts_construct (new, storage_set, folder_type_registry);

	if (! e_shortcuts_load (new, file_name)) {
		gtk_object_unref (GTK_OBJECT (new));
		return NULL;
	}

	return new;
}


GList *
e_shortcuts_get_group_titles (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;
	GList *list;
	GList *p;

	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);

	priv = shortcuts->priv;

	list = NULL;

	for (p = priv->groups; p != NULL; p = p->next) {
		group = (ShortcutGroup *) p->data;
		list = g_list_prepend (list, g_strdup (group->title));
	}

	return g_list_reverse (list);
}

GList *
e_shortcuts_get_shortcuts_in_group (EShortcuts *shortcuts,
				    const char *group_title)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *shortcut_group;
	GList *list;
	GList *p;

	priv = shortcuts->priv;

	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);
	g_return_val_if_fail (group_title != NULL, NULL);

	shortcut_group = g_hash_table_lookup (priv->title_to_group, group_title);
	if (shortcut_group == NULL)
		return NULL;

	list = NULL;

	for (p = shortcut_group->shortcuts; p != NULL; p = p->next)
		list = g_list_prepend (list, g_strdup ((const char *) p->data));

	return g_list_reverse (list);
}


EStorageSet *
e_shortcuts_get_storage_set (EShortcuts *shortcuts)
{
	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);

	return shortcuts->priv->storage_set;
}


GtkWidget *
e_shortcuts_new_view (EShortcuts *shortcuts)
{
	EShortcutsPrivate *priv;
	GtkWidget *new;

	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);

	priv = shortcuts->priv;

	new = e_shortcuts_view_new (shortcuts);
	priv->views = g_list_prepend (priv->views, new);

	gtk_signal_connect (GTK_OBJECT (new), "destroy", view_destroyed_cb, shortcuts);

	return new;
}


gboolean
e_shortcuts_load (EShortcuts *shortcuts,
		  const char *file_name)
{
	EShortcutsPrivate *priv;
	char *tmp;

	g_return_val_if_fail (shortcuts != NULL, FALSE);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), FALSE);
	g_return_val_if_fail (file_name == NULL || g_path_is_absolute (file_name), FALSE);

	priv = shortcuts->priv;

	if (file_name == NULL) {
		if (priv->file_name == NULL)
			return FALSE;
		file_name = priv->file_name;
	}

	if (! load_shortcuts (shortcuts, file_name))
		return FALSE;

	tmp = g_strdup (file_name);
	g_free (priv->file_name);
	priv->file_name = tmp;

	return TRUE;
}

gboolean
e_shortcuts_save (EShortcuts *shortcuts,
		  const char *file_name)
{
	EShortcutsPrivate *priv;
	char *tmp;

	g_return_val_if_fail (shortcuts != NULL, FALSE);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), FALSE);
	g_return_val_if_fail (file_name == NULL || g_path_is_absolute (file_name), FALSE);

	priv = shortcuts->priv;

	if (file_name == NULL) {
		if (priv->file_name == NULL)
			return FALSE;
		file_name = priv->file_name;
	}

	if (! save_shortcuts (shortcuts, file_name))
		return FALSE;

	tmp = g_strdup (file_name);
	g_free (priv->file_name);
	priv->file_name = tmp;

	return TRUE;
}


const char *
e_shortcuts_get_uri (EShortcuts *shortcuts, int group_num, int num)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;
	GList *shortcut_element;

	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);

	priv = shortcuts->priv;

	group = g_list_nth (priv->groups, group_num)->data;
	if (group == NULL)
		return NULL;

	shortcut_element = g_list_nth (group->shortcuts, num);
	if (shortcut_element == NULL)
		return NULL;

	return shortcut_element->data;
}


void
e_shortcuts_remove_shortcut (EShortcuts *shortcuts,
			     int group_num,
			     int num)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;
	GList *p;
	char *uri;

	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));

	priv = shortcuts->priv;

	p = g_list_nth (priv->groups, group_num);
	g_return_if_fail (p != NULL);

	group = (ShortcutGroup *) p->data;

	p = g_list_nth (group->shortcuts, num);
	g_return_if_fail (p != NULL);

	gtk_signal_emit (GTK_OBJECT (shortcuts), signals[REMOVE_SHORTCUT], group_num, num);

	uri = (char *) p->data;
	g_free (uri);

	group->shortcuts = g_list_remove_link (group->shortcuts, p);

	make_dirty (shortcuts);
}

void
e_shortcuts_add_shortcut (EShortcuts *shortcuts,
			  int group_num,
			  int num,
			  const char *uri)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;
	GList *p;

	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));

	priv = shortcuts->priv;

	p = g_list_nth (priv->groups, group_num);
	g_return_if_fail (p != NULL);

	group = (ShortcutGroup *) p->data;

	if (num == -1)
		num = g_list_length (group->shortcuts);

	group->shortcuts = g_list_insert (group->shortcuts, g_strdup (uri), num);

	gtk_signal_emit (GTK_OBJECT (shortcuts), signals[NEW_SHORTCUT], group_num, num);

	make_dirty (shortcuts);
}

void
e_shortcuts_update_shortcut (EShortcuts *shortcuts,
			     int         group_num,
			     int         num,
			     const char *uri)
{
	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));
		
	/* FIXME: need support in e-shortcut-bar widget (and also
           e-icon-bar) to be able to "update" a shortcut without doing
           this lame remove then add */

	e_shortcuts_remove_shortcut (shortcuts, group_num, num);
	e_shortcuts_add_shortcut (shortcuts, group_num, num, uri);
}


/* The shortcuts_by_uri functions */


typedef struct {
	int group_num;
	int num;
} EShortcutPosition;

static GList *
find_positions_by_uri (EShortcuts  *shortcuts,
		       const char  *uri)
{
	EShortcutsPrivate *priv;
	GList *p = NULL, *q = NULL;
	GList *retval = NULL;
	int group_num = 0, num = 0;

	priv = shortcuts->priv;

	for (p = priv->groups; p != NULL; p = p->next) {
		ShortcutGroup *group;

		group = (ShortcutGroup *) p->data;

		for (q = group->shortcuts; q != NULL; q = q->next) {
			char *listeduri = q->data;

			if (!strcmp (uri, listeduri)) {
				EShortcutPosition *position;

				position = g_new (EShortcutPosition, 1);
				position->group_num = group_num;
				position->num = num;
				
				retval = g_list_append (retval, position);
			}
			num++;
		}

		group_num++;
		num = 0;
	}

	return g_list_first (retval);
}

void
e_shortcuts_remove_shortcut_by_uri (EShortcuts *shortcuts,
				    const char *uri)
{
	GList *items = NULL;

	items = find_positions_by_uri (shortcuts, uri);

	while (items) {
		EShortcutPosition *pos = (EShortcutPosition *) items->data;

		if (pos) {
			e_shortcuts_remove_shortcut (shortcuts, pos->group_num, pos->num);
			g_free (pos);
		}
		items = g_list_next (items);
	}
	g_list_free (items);
}

void
e_shortcuts_update_shortcut_by_uri (EShortcuts *shortcuts,
				    const char *uri)
{
	GList *items = NULL;

	items = find_positions_by_uri (shortcuts, uri);

	while (items) {
		EShortcutPosition *pos = (EShortcutPosition *) items->data;

		if (pos) {
			e_shortcuts_update_shortcut (shortcuts,
						     pos->group_num, pos->num,
						     uri);
			g_free (pos);
		}
		items = g_list_next (items);
	}
	g_list_free (items);
}

void
e_shortcuts_remove_group (EShortcuts *shortcuts,
			  int group_num)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;
	GList *p;

	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));

	priv = shortcuts->priv;

	p = g_list_nth (priv->groups, group_num);
	g_return_if_fail (p != NULL);

	gtk_signal_emit (GTK_OBJECT (shortcuts), signals[REMOVE_GROUP], group_num);

	group = (ShortcutGroup *) p->data;

	e_free_string_list (group->shortcuts);

	priv->groups = g_list_remove_link (priv->groups, p);

	make_dirty (shortcuts);
}

void
e_shortcuts_add_group (EShortcuts *shortcuts,
		       int group_num,
		       const char *group_name)
{
	EShortcutsPrivate *priv;
	ShortcutGroup *group;

	g_return_if_fail (shortcuts != NULL);
	g_return_if_fail (E_IS_SHORTCUTS (shortcuts));

	priv = shortcuts->priv;

	group = g_new (ShortcutGroup, 1);
	group->title     = g_strdup (group_name);
	group->shortcuts = NULL;

	if (group_num == -1)
		group_num = g_list_length (priv->groups);

	priv->groups = g_list_insert (priv->groups, group, group_num);

	gtk_signal_emit (GTK_OBJECT (shortcuts), signals[NEW_GROUP], group_num);

	make_dirty (shortcuts);
}


const char *
e_shortcuts_get_group_title (EShortcuts *shortcuts,
			     int group_num)
{
	EShortcutsPrivate *priv;
	GList *group_element;
	const ShortcutGroup *group;

	g_return_val_if_fail (shortcuts != NULL, NULL);
	g_return_val_if_fail (E_IS_SHORTCUTS (shortcuts), NULL);

	priv = shortcuts->priv;

	group_element = g_list_nth (priv->groups, group_num);
	if (group_element == NULL)
		return NULL;

	group = (ShortcutGroup *) group_element->data;

	return group->title;
}


E_MAKE_TYPE (e_shortcuts, "EShortcuts", EShortcuts, class_init, init, PARENT_TYPE)

