/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2002 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "mail-composer-prefs.h"
#include "composer/e-msg-composer.h"

#include <gtk/gtksignal.h>

#include <bonobo/bonobo-generic-factory.h>

#include <gal/widgets/e-gui-utils.h>
#include <gal/widgets/e-unicode.h>

#include "widgets/misc/e-charset-picker.h"

#include "mail-config.h"

#include "art/mark.xpm"


#define d(x)

static void mail_composer_prefs_class_init (MailComposerPrefsClass *class);
static void mail_composer_prefs_init       (MailComposerPrefs *dialog);
static void mail_composer_prefs_destroy    (GtkObject *obj);
static void mail_composer_prefs_finalise   (GObject *obj);

static void sig_event_client (MailConfigSigEvent event, MailConfigSignature *sig, MailComposerPrefs *prefs);

static GtkVBoxClass *parent_class = NULL;


GType
mail_composer_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (MailComposerPrefsClass),
			NULL, NULL,
			(GClassInitFunc) mail_composer_prefs_class_init,
			NULL, NULL,
			sizeof (MailComposerPrefs),
			0,
			(GInstanceInitFunc) mail_composer_prefs_init,
		};
		
		type = g_type_register_static(gtk_vbox_get_type (), "MailComposerPrefs", &info, 0);
	}
	
	return type;
}

static void
mail_composer_prefs_class_init (MailComposerPrefsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref(gtk_vbox_get_type ());
	
	object_class->destroy = mail_composer_prefs_destroy;
	gobject_class->finalize = mail_composer_prefs_finalise;
}

static void
mail_composer_prefs_init (MailComposerPrefs *composer_prefs)
{
	composer_prefs->enabled_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) mark_xpm);
	gdk_pixbuf_render_pixmap_and_mask (composer_prefs->enabled_pixbuf,
					   &composer_prefs->mark_pixmap, &composer_prefs->mark_bitmap, 128);
}

static void
mail_composer_prefs_finalise (GObject *obj)
{
	MailComposerPrefs *prefs = (MailComposerPrefs *) obj;
	
	g_object_unref((prefs->gui));
	g_object_unref((prefs->pman));
	g_object_unref (prefs->enabled_pixbuf);
	gdk_pixmap_unref (prefs->mark_pixmap);
	g_object_unref (prefs->mark_bitmap);
	
        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
mail_composer_prefs_destroy (GtkObject *obj)
{
	MailComposerPrefs *prefs = (MailComposerPrefs *) obj;

	mail_config_signature_unregister_client ((MailConfigSignatureClient) sig_event_client, prefs);
	
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}

static void
attach_style_info (GtkWidget *item, gpointer user_data)
{
	int *style = user_data;
	
	g_object_set_data((GObject *)(item), "style", GINT_TO_POINTER (*style));
	
	(*style)++;
}

static void
toggle_button_toggled (GtkWidget *widget, gpointer user_data)
{
	MailComposerPrefs *prefs = (MailComposerPrefs *) user_data;
	
	if (prefs->control)
		evolution_config_control_changed (prefs->control);
}

static void
menu_changed (GtkWidget *widget, gpointer user_data)
{
	MailComposerPrefs *prefs = (MailComposerPrefs *) user_data;
	
	if (prefs->control)
		evolution_config_control_changed (prefs->control);
}

static void
option_menu_connect (GtkOptionMenu *omenu, gpointer user_data)
{
	GtkWidget *menu, *item;
	GList *items;
	
	menu = gtk_option_menu_get_menu (omenu);
	
	items = GTK_MENU_SHELL (menu)->children;
	while (items) {
		item = items->data;
		g_signal_connect (item, "activate", G_CALLBACK (menu_changed), user_data);
		items = items->next;
	}
}

static void
sig_load_preview (MailComposerPrefs *prefs, MailConfigSignature *sig)
{
	char *str;
	
	if (!sig) {
		gtk_html_load_from_string (GTK_HTML (prefs->sig_preview), " ", 1);
		return;
	}
	
	if (sig->script)
		str = mail_config_signature_run_script (sig->script);
	else
		str = e_msg_composer_get_sig_file_content (sig->filename, sig->html);
	if (!str)
		str = g_strdup (" ");
	
	/* printf ("HTML: %s\n", str); */
	if (sig->html)
		gtk_html_load_from_string (GTK_HTML (prefs->sig_preview), str, strlen (str));
	else {
		GtkHTMLStream *stream;
		int len;
		
		len = strlen (str);
		stream = gtk_html_begin_content (GTK_HTML (prefs->sig_preview), "text/html; charset=utf-8");
		gtk_html_write (GTK_HTML (prefs->sig_preview), stream, "<PRE>", 5);
		if (len)
			gtk_html_write (GTK_HTML (prefs->sig_preview), stream, str, len);
		gtk_html_write (GTK_HTML (prefs->sig_preview), stream, "</PRE>", 6);
		gtk_html_end (GTK_HTML (prefs->sig_preview), stream, GTK_HTML_STREAM_OK);
	}
	
	g_free (str);
}

static MailConfigSignature *
sig_current_sig (MailComposerPrefs *prefs)
{
	return gtk_clist_get_row_data (GTK_CLIST (prefs->sig_clist), prefs->sig_row);
}

static void
sig_edit (GtkWidget *widget, MailComposerPrefs *prefs)
{
	MailConfigSignature *sig = sig_current_sig (prefs);
	
	if (sig->filename && *sig->filename)
		mail_signature_editor (sig);
	else
		e_notice (GTK_WINDOW (prefs), GNOME_MESSAGE_BOX_ERROR,
			  _("Please specify signature filename\nin Advanced section of signature settings."));
}

MailConfigSignature *
mail_composer_prefs_new_signature (MailComposerPrefs *prefs, gboolean html, const gchar *script)
{
	MailConfigSignature *sig;
	char *name[1];
	int row;
	
	sig = mail_config_signature_add (html, script);
	
	if (prefs) {
		if (sig->name)
			name[0] = g_strconcat (sig->name, " ", _("[script]"), NULL);
		else
			name[0] = g_strdup (_("[script]"));
		
		row = gtk_clist_append (prefs->sig_clist, name);
		gtk_clist_set_row_data (prefs->sig_clist, row, sig);
		gtk_clist_select_row (GTK_CLIST (prefs->sig_clist), row, 0);
		g_free (name[0]);
		/*gtk_widget_grab_focus (prefs->sig_name);*/
	}
	
	if (sig->filename && *sig->filename)
		mail_signature_editor (sig);
	
	return sig;
}

static void sig_row_unselect (GtkCList *clist, int row, int col, GdkEvent *event, MailComposerPrefs *prefs);

static void
sig_delete (GtkWidget *widget, MailComposerPrefs *prefs)
{
	MailConfigSignature *sig = sig_current_sig (prefs);
	
	gtk_clist_remove (prefs->sig_clist, prefs->sig_row);
	mail_config_signature_delete (sig);
	if (prefs->sig_row < prefs->sig_clist->rows)
		gtk_clist_select_row (prefs->sig_clist, prefs->sig_row, 0);
	else if (prefs->sig_row)
		gtk_clist_select_row (prefs->sig_clist, prefs->sig_row - 1, 0);
	else
		sig_row_unselect (prefs->sig_clist, prefs->sig_row, 0, NULL, prefs);
}

static void
sig_add (GtkWidget *widget, MailComposerPrefs *prefs)
{
	mail_composer_prefs_new_signature (prefs, mail_config_get_send_html (), NULL);
}

static void
sig_add_script_response (GtkWidget *widget, int button, MailComposerPrefs *prefs)
{
	const char *script, *name;
	GtkWidget *dialog;
	GtkWidget *entry;
	
	if (button == GTK_RESPONSE_ACCEPT) {
		entry = glade_xml_get_widget (prefs->sig_script_gui, "fileentry_add_script_script");
		script = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (entry))));
		
		entry = glade_xml_get_widget (prefs->sig_script_gui, "entry_add_script_name");
		name = gtk_entry_get_text (GTK_ENTRY (entry));
		if (script && *script) {
			struct stat st;
			
			if (!stat (script, &st) && S_ISREG (st.st_mode) && (st.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR))) {
				MailConfigSignature *sig;
				
				sig = mail_composer_prefs_new_signature (prefs, TRUE, script);
				mail_config_signature_set_name (sig, name);
				gtk_widget_hide (prefs->sig_script_dialog);
				
				return;
			}
		}
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (prefs->sig_script_dialog),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 "%s", _("You must specify a valid script name."));
		
		gtk_dialog_run ((GtkDialog *) dialog);
		gtk_widget_destroy (dialog);
	}
	
	gtk_widget_hide (widget);
}

static void
sig_add_script (GtkWidget *widget, MailComposerPrefs *prefs)
{
	GtkWidget *entry;
	
	entry = glade_xml_get_widget (prefs->sig_script_gui, "entry_add_script_name");
	gtk_entry_set_text (GTK_ENTRY (entry), _("Unnamed"));
	
	gtk_widget_show_all (prefs->sig_script_dialog);
	gdk_window_raise (prefs->sig_script_dialog->window);
}

static void
sig_row_select (GtkCList *clist, int row, int col, GdkEvent *event, MailComposerPrefs *prefs)
{
	MailConfigSignature *sig;
	
	d(printf ("sig_row_select\n"));
	sig = gtk_clist_get_row_data (prefs->sig_clist, row);
	prefs->sig_row = row;
	
	gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_delete, TRUE);
	gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_edit, sig->script == NULL);
	
	sig_load_preview (prefs, sig);
}

static void
sig_row_unselect (GtkCList *clist, int row, int col, GdkEvent *event, MailComposerPrefs *prefs)
{
	d(printf ("sig_row_unselect\n"));
	gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_delete, FALSE);
	gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_edit, FALSE);
}

static void
sig_fill_clist (GtkCList *clist)
{
	char *name[1];
	GList *l;
	int row;
	
	gtk_clist_freeze (clist);
	for (l = mail_config_get_signature_list (); l; l = l->next) {
		name[0] = ((MailConfigSignature *) l->data)->name;
		if (((MailConfigSignature *) l->data)->script)
			name[0] = g_strconcat (name[0], " ", _("[script]"), NULL);
		else
			name[0] = g_strdup (name[0]);
		
		row = gtk_clist_append (clist, name);
		gtk_clist_set_row_data (clist, row, l->data);
		g_free (name[0]);
	}
	gtk_clist_thaw (clist);
}

static void
url_requested (GtkHTML *html, const char *url, GtkHTMLStream *handle)
{
	GtkHTMLStreamStatus status;
	char buf[128];
	ssize_t size;
	int fd;
	
	if (!strncmp (url, "file:", 5))
		url += 5;
	
	fd = open (url, O_RDONLY);
	status = GTK_HTML_STREAM_OK;
	if (fd != -1) {
		while ((size = read (fd, buf, sizeof (buf)))) {
			if (size == -1) {
				status = GTK_HTML_STREAM_ERROR;
				break;
			} else
				gtk_html_write (html, handle, buf, size);
		}
	} else
		status = GTK_HTML_STREAM_ERROR;
	
	gtk_html_end (html, handle, status);
}

static void
sig_event_client (MailConfigSigEvent event, MailConfigSignature *sig, MailComposerPrefs *prefs)
{
	char *text;
	
	switch (event) {
	case MAIL_CONFIG_SIG_EVENT_NAME_CHANGED:
		d(printf ("accounts NAME CHANGED\n"));
		if (sig->script)
			text = g_strconcat (sig->name, " ", _("[script]"), NULL);
		else
			text = g_strdup (sig->name);
		
		gtk_clist_set_text (GTK_CLIST (prefs->sig_clist), sig->id, 0, text);
		g_free (text);
		break;
	case MAIL_CONFIG_SIG_EVENT_CONTENT_CHANGED:
		d(printf ("accounts CONTENT CHANGED\n"));
		if (sig == sig_current_sig (prefs))
			sig_load_preview (prefs, sig);
		break;
	default:
		;
	}
}

/*
 *
 * Spell checking cut'n'pasted from gnome-spell/capplet/main.c
 *
 */

#include "Spell.h"

#define GNOME_SPELL_GCONF_DIR "/GNOME/Spell"
#define SPELL_API_VERSION "0.2"

static void
spell_select_lang (MailComposerPrefs *prefs, const gchar *abrev)
{
	int i;
	
	for (i = 0; i < prefs->language_seq->_length; i ++) {
		if (!strcasecmp (abrev, prefs->language_seq->_buffer [i].abrev)) {
			gtk_clist_set_pixmap (GTK_CLIST (prefs->language), i, 0, prefs->mark_pixmap, prefs->mark_bitmap);
		}
	}
}

static void
spell_set_ui_language (MailComposerPrefs *prefs)
{
	char *l, *last, *lang;
	int i;
	
	gtk_clist_freeze (GTK_CLIST (prefs->language));
	gtk_clist_unselect_all (GTK_CLIST (prefs->language));
	
	for (i = 0; i < prefs->language_seq->_length; i ++) {
		gtk_clist_set_pixmap (GTK_CLIST (prefs->language), i, 0, NULL, NULL);
	}
	
	last = prefs->language_str;
	while ((l = strchr (last, ' '))) {
		if (l != last) {
			lang = g_strndup (last, l - last);
			spell_select_lang (prefs, lang);
			g_free (lang);
		}
		
		last = l + 1;
	}
	if (last)
		spell_select_lang (prefs, last);
	gtk_clist_thaw (GTK_CLIST (prefs->language));
}

static void
spell_set_ui (MailComposerPrefs *prefs)
{
	prefs->spell_active = FALSE;
	
	spell_set_ui_language (prefs);
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->colour),
				    prefs->spell_error_color.red,
				    prefs->spell_error_color.green,
				    prefs->spell_error_color.blue, 0xffff);
	
	prefs->spell_active = TRUE;
}

static gchar *
spell_get_language_str (MailComposerPrefs *prefs)
{
	GString *str = g_string_new (NULL);
	char *rv;
	int i;
	
	for (i = 0; i < GTK_CLIST (prefs->language)->rows; i ++) {
		GdkPixmap *pmap = NULL;
		GdkBitmap *bmap;
		
		gtk_clist_get_pixmap (GTK_CLIST (prefs->language), i, 0, &pmap, &bmap);
		if (pmap) {
			if (str->len)
				g_string_append_c (str, ' ');
			g_string_append (str, gtk_clist_get_row_data (GTK_CLIST (prefs->language), i));
		}
	}
	
	rv = str->str;
	g_string_free (str, FALSE);
	
	return rv ? rv : g_strdup ("");
}

static void
spell_get_ui (MailComposerPrefs *prefs)
{
	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->colour),
				    &prefs->spell_error_color.red,
				    &prefs->spell_error_color.green,
				    &prefs->spell_error_color.blue, NULL);
	g_free (prefs->language_str);
	prefs->language_str = spell_get_language_str (prefs);
}

#define GET(t,x,prop,f,c) \
        val = gconf_client_get_without_default (prefs->gconf, GNOME_SPELL_GCONF_DIR x, NULL); \
        if (val) { f; prop = c (gconf_value_get_ ## t (val)); \
        gconf_value_free (val); }

static void
spell_save_orig (MailComposerPrefs *prefs)
{
	g_free (prefs->language_str_orig);
	prefs->language_str_orig = g_strdup (prefs->language_str ? prefs->language_str : "");
	prefs->spell_error_color_orig = prefs->spell_error_color;
}

/* static void
spell_load_orig (MailComposerPrefs *prefs)
{
	g_free (prefs->language_str);
	prefs->language_str = g_strdup (prefs->language_str_orig);
	prefs->spell_error_color = prefs->spell_error_color_orig;
} */

static void
spell_load_values (MailComposerPrefs *prefs)
{
	GConfValue *val;
	char *def_lang;
	
	def_lang = g_strdup ("en");
	g_free (prefs->language_str);
	prefs->language_str = g_strdup (def_lang);
	prefs->spell_error_color.red   = 0xffff;
	prefs->spell_error_color.green = 0;
	prefs->spell_error_color.blue  = 0;
	
	GET (int, "/spell_error_color_red",   prefs->spell_error_color.red, (void)0, (int));
	GET (int, "/spell_error_color_green", prefs->spell_error_color.green, (void)0, (int));
	GET (int, "/spell_error_color_blue",  prefs->spell_error_color.blue, (void)0, (int));
	GET (string, "/language", prefs->language_str, g_free (prefs->language_str), g_strdup);
	
	if (prefs->language_str == NULL)
		prefs->language_str = g_strdup (def_lang);
	
	spell_save_orig (prefs);
	
	g_free (def_lang);
}

#define SET(t,x,prop) \
        gconf_client_set_ ## t (prefs->gconf, GNOME_SPELL_GCONF_DIR x, prop, NULL);

#define STR_EQUAL(str1, str2) ((str1 == NULL && str2 == NULL) || (str1 && str2 && !strcmp (str1, str2)))

static void
spell_save_values (MailComposerPrefs *prefs, gboolean force)
{
	if (force || !gdk_color_equal (&prefs->spell_error_color, &prefs->spell_error_color_orig)) {
		SET (int, "/spell_error_color_red",   prefs->spell_error_color.red);
		SET (int, "/spell_error_color_green", prefs->spell_error_color.green);
		SET (int, "/spell_error_color_blue",  prefs->spell_error_color.blue);
	}
	
	if (force || !STR_EQUAL (prefs->language_str, prefs->language_str_orig)) {
		SET (string, "/language", prefs->language_str ? prefs->language_str : "");
	}
	
	gconf_client_suggest_sync (prefs->gconf, NULL);
}

static void
spell_apply (MailComposerPrefs *prefs)
{
	spell_get_ui (prefs);
	spell_save_values (prefs, FALSE);
}

/* static void
spell_revert (MailComposerPrefs *prefs)
{
	spell_load_orig (prefs);
	spell_set_ui (prefs);
	spell_save_values (prefs, TRUE);
} */

static void
spell_changed (gpointer user_data)
{
	MailComposerPrefs *prefs = (MailComposerPrefs *) user_data;
	
	if (prefs->control)
		evolution_config_control_changed (prefs->control);
}

static void
spell_color_set (GtkWidget *widget, guint r, guint g, guint b, guint a, gpointer user_data)
{
	spell_changed (user_data);
}

static void
spell_language_select_row (GtkWidget *widget, gint row, gint column, GdkEvent *event, MailComposerPrefs *prefs)
{
	GList *sel = GTK_CLIST (prefs->language)->selection;
	
	if (sel) {
		GdkPixmap *pmap = NULL;
		GdkBitmap *bmap;
		int row = GPOINTER_TO_INT (sel->data);
		
		gtk_clist_get_pixmap (GTK_CLIST (prefs->language), row, 0, &pmap, &bmap);
		if (pmap)
			gtk_label_set_text (GTK_LABEL (GTK_BIN (prefs->spell_able_button)->child), _("Disable"));
		else
			gtk_label_set_text (GTK_LABEL (GTK_BIN (prefs->spell_able_button)->child), _("Enable"));
	}
	
	gtk_widget_set_sensitive (prefs->spell_able_button, TRUE);
}

static void
spell_language_unselect_row (GtkWidget *widget, gint row, gint column, GdkEvent *event, MailComposerPrefs *prefs)
{
	gtk_widget_set_sensitive (prefs->spell_able_button, FALSE);
}

static void
spell_language_enable (GtkWidget *widget, MailComposerPrefs *prefs)
{
	GList *sel = GTK_CLIST (prefs->language)->selection;
	
	if (sel) {
		int row = GPOINTER_TO_INT (sel->data);
		GdkPixmap *pmap = NULL;
		GdkBitmap *bmap;
		
		gtk_clist_get_pixmap (GTK_CLIST (prefs->language), row, 0, &pmap, &bmap);
		if (pmap) {
			gtk_clist_set_pixmap (GTK_CLIST (prefs->language), row, 0, NULL, NULL);
			gtk_label_set_text (GTK_LABEL (GTK_BIN (prefs->spell_able_button)->child), _("Enable"));
		} else {
			gtk_label_set_text (GTK_LABEL (GTK_BIN (prefs->spell_able_button)->child), _("Disable"));
			gtk_clist_set_pixmap (GTK_CLIST (prefs->language), row, 0, prefs->mark_pixmap, prefs->mark_bitmap);
		}
		
		spell_changed (prefs);
	}
}

static void
spell_language_button_press (GtkWidget *widget, GdkEventButton *event, MailComposerPrefs *prefs)
{
	int row, col;
	
	if (gtk_clist_get_selection_info (prefs->language, event->x, event->y, &row, &col)) {
		if (col == 0) {
			GList *sel = GTK_CLIST (prefs->language)->selection;
			GdkPixmap *pmap = NULL;
			GdkBitmap *bmap;
			
			g_signal_stop_emission_by_name (widget, "button_press_event");
			
			gtk_clist_get_pixmap (GTK_CLIST (prefs->language), row, 0, &pmap, &bmap);
			if (pmap)
				gtk_clist_set_pixmap (GTK_CLIST (prefs->language), row, 0, NULL, NULL);
			else
				gtk_clist_set_pixmap (GTK_CLIST (prefs->language), row, 0,
						      prefs->mark_pixmap, prefs->mark_bitmap);
			
			if (sel && GPOINTER_TO_INT (sel->data) == row)
				gtk_label_set_text (GTK_LABEL (GTK_BIN (prefs->spell_able_button)->child),
						    pmap ? _("Enable") : _("Disable"));
			
			spell_changed (prefs);
		}
	}
}

static void
spell_setup (MailComposerPrefs *prefs)
{
	int i;
	
	gtk_clist_freeze (GTK_CLIST (prefs->language));
	if (prefs->language_seq) {
		for (i = 0; i < prefs->language_seq->_length; i++) {
			char *texts[2];
			
			texts[0] = NULL;
			texts[1] = _(prefs->language_seq->_buffer[i].name);
			gtk_clist_append (GTK_CLIST (prefs->language), texts);
			gtk_clist_set_row_data (GTK_CLIST (prefs->language), i, prefs->language_seq->_buffer[i].abrev);
		}
	}
	gtk_clist_thaw (GTK_CLIST (prefs->language));
	
	spell_load_values (prefs);
	spell_set_ui (prefs);
	
	glade_xml_signal_connect_data (prefs->gui, "spellColorSet", G_CALLBACK (spell_color_set), prefs);
	glade_xml_signal_connect_data (prefs->gui, "spellLanguageSelectRow",
				       G_CALLBACK (spell_language_select_row), prefs);
	glade_xml_signal_connect_data (prefs->gui, "spellLanguageUnselectRow",
				       G_CALLBACK (spell_language_unselect_row), prefs);
	glade_xml_signal_connect_data (prefs->gui, "spellLanguageEnable", G_CALLBACK (spell_language_enable), prefs);
	
	g_signal_connect (prefs->language, "button_press_event", G_CALLBACK (spell_language_button_press), prefs);
}

static gboolean
spell_setup_check_options (MailComposerPrefs *prefs)
{
	GNOME_Spell_Dictionary dict;
	CORBA_Environment ev;
	char *dictionary_id;

	dictionary_id = "OAFIID:GNOME_Spell_Dictionary:" SPELL_API_VERSION;
	dict = bonobo_activation_activate_from_id(dictionary_id, 0, NULL, NULL);
	if (dict == CORBA_OBJECT_NIL) {
		g_warning ("Cannot activate %s", dictionary_id);
		
		return FALSE;
	}
	
	CORBA_exception_init (&ev);
	prefs->language_seq = GNOME_Spell_Dictionary_getLanguages (dict, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		prefs->language_seq = NULL;
	CORBA_exception_free (&ev);
	
	if (prefs->language_seq == NULL)
		return FALSE;

	gconf_client_add_dir (prefs->gconf, GNOME_SPELL_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
	
        spell_setup (prefs);
	
	return TRUE;
}

/*
 * End of Spell checking
 */

static void
mail_composer_prefs_construct (MailComposerPrefs *prefs)
{
	GtkWidget *toplevel, *widget, *menu, *info_pixmap;
	GtkDialog *dialog;
	GladeXML *gui;
	int style;
	char *names[][2] = {
		{ "live_spell_check", "chkEnableSpellChecking" },
		{ "magic_smileys_check", "chkAutoSmileys" },
		{ "gtk_html_prop_keymap_option", "omenuShortcutsType" },
		{ NULL, NULL }
	};
	
	prefs->gconf = gconf_client_get_default ();
	
	gui = glade_xml_new (EVOLUTION_GLADEDIR "/mail-config.glade", "composer_tab", NULL);
	prefs->gui = gui;
	prefs->sig_script_gui = glade_xml_new (EVOLUTION_GLADEDIR "/mail-config.glade", "vbox_add_script_signature", NULL);
	
	/* get our toplevel widget */
	toplevel = glade_xml_get_widget (gui, "toplevel");
	
	/* reparent */
	gtk_widget_ref (toplevel);
	gtk_container_remove (GTK_CONTAINER (toplevel->parent), toplevel);
	gtk_container_add (GTK_CONTAINER (prefs), toplevel);
	gtk_widget_unref (toplevel);
	
	/* General tab */
	
	/* Default Behavior */
	prefs->send_html = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui, "chkSendHTML"));
	gtk_toggle_button_set_active (prefs->send_html, mail_config_get_send_html ());
	g_signal_connect (prefs->send_html, "toggled", G_CALLBACK(toggle_button_toggled), prefs);
	
	prefs->auto_smileys = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui, "chkAutoSmileys"));
	g_signal_connect (prefs->auto_smileys, "toggled", G_CALLBACK(toggle_button_toggled), prefs);
	
	prefs->prompt_empty_subject = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui, "chkPromptEmptySubject"));
	gtk_toggle_button_set_active (prefs->prompt_empty_subject, mail_config_get_prompt_empty_subject ());
	g_signal_connect (prefs->prompt_empty_subject, "toggled", G_CALLBACK(toggle_button_toggled), prefs);
	
	prefs->prompt_bcc_only = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui, "chkPromptBccOnly"));
	gtk_toggle_button_set_active (prefs->prompt_bcc_only, mail_config_get_prompt_only_bcc ());
	g_signal_connect (prefs->prompt_bcc_only, "toggled", G_CALLBACK(toggle_button_toggled), prefs);
	
	prefs->charset = GTK_OPTION_MENU (glade_xml_get_widget (gui, "omenuCharset"));
	menu = e_charset_picker_new (mail_config_get_default_charset ());
	gtk_option_menu_set_menu (prefs->charset, GTK_WIDGET (menu));
	option_menu_connect (prefs->charset, prefs);

#warning "gtkhtml prop manager"
#if 0	
	/* Spell Checking: GtkHTML part */
	prefs->pman = GTK_HTML_PROPMANAGER (gtk_html_propmanager_new (NULL));
	g_signal_connect (prefs->pman, "changed", G_CALLBACK(toggle_button_toggled), prefs);
	g_object_ref((prefs->pman));
	
	gtk_html_propmanager_set_names (prefs->pman, names);
	gtk_html_propmanager_set_gui (prefs->pman, gui, NULL);
#endif

	/* Spell Checking: GNOME Spell part */
	prefs->colour = GNOME_COLOR_PICKER (glade_xml_get_widget (gui, "colorpickerSpellCheckColor"));
	prefs->language = GTK_CLIST (glade_xml_get_widget (gui, "clistSpellCheckLanguage"));
	prefs->spell_able_button = glade_xml_get_widget (gui, "buttonSpellCheckEnable");
	info_pixmap = glade_xml_get_widget (gui, "pixmapSpellInfo");
	gtk_clist_set_column_justification (prefs->language, 0, GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_auto_resize (prefs->language, 0, TRUE);
	gtk_image_set_from_file(GTK_IMAGE (info_pixmap), EVOLUTION_IMAGES "/info-bulb.png");
	
	if (!spell_setup_check_options (prefs)) {
		gtk_widget_hide (GTK_WIDGET (prefs->colour));
		gtk_widget_hide (GTK_WIDGET (prefs->language));
	}
	
	/* Forwards and Replies */
	prefs->forward_style = GTK_OPTION_MENU (glade_xml_get_widget (gui, "omenuForwardStyle"));
	gtk_option_menu_set_history (prefs->forward_style, mail_config_get_default_forward_style ());
	style = 0;
	gtk_container_foreach (GTK_CONTAINER (gtk_option_menu_get_menu (prefs->forward_style)),
			       attach_style_info, &style);
	option_menu_connect (prefs->forward_style, prefs);
	
	prefs->reply_style = GTK_OPTION_MENU (glade_xml_get_widget (gui, "omenuReplyStyle"));
	gtk_option_menu_set_history (prefs->reply_style, mail_config_get_default_reply_style ());
	style = 0;
	gtk_container_foreach (GTK_CONTAINER (gtk_option_menu_get_menu (prefs->reply_style)),
			       attach_style_info, &style);
	option_menu_connect (prefs->reply_style, prefs);
	
	/* Signatures */
	dialog = (GtkDialog *) gtk_dialog_new ();
	prefs->sig_script_dialog = (GtkWidget *) dialog;
	gtk_dialog_add_buttons (dialog, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	gtk_window_set_title ((GtkWindow *) dialog, _("Add script signature"));
	g_signal_connect (dialog, "response", G_CALLBACK (sig_add_script_response), prefs);
	widget = glade_xml_get_widget (prefs->sig_script_gui, "vbox_add_script_signature");
	gtk_box_pack_start_defaults ((GtkBox *) dialog->vbox, widget);
	
	prefs->sig_add = GTK_BUTTON (glade_xml_get_widget (gui, "cmdSignatureAdd"));
	g_signal_connect (prefs->sig_add, "clicked", G_CALLBACK (sig_add), prefs);
	
	glade_xml_signal_connect_data (gui, "cmdSignatureAddScriptClicked", G_CALLBACK(sig_add_script), prefs);
	
	prefs->sig_edit = GTK_BUTTON (glade_xml_get_widget (gui, "cmdSignatureEdit"));
	g_signal_connect (prefs->sig_edit, "clicked", G_CALLBACK (sig_edit), prefs);
	
	prefs->sig_delete = GTK_BUTTON (glade_xml_get_widget (gui, "cmdSignatureDelete"));
	g_signal_connect (prefs->sig_delete, "clicked", G_CALLBACK (sig_delete), prefs);
	
	prefs->sig_clist = GTK_CLIST (glade_xml_get_widget (gui, "clistSignatures"));
	sig_fill_clist (prefs->sig_clist);
	g_signal_connect (prefs->sig_clist, "select_row", G_CALLBACK (sig_row_select), prefs);
	g_signal_connect (prefs->sig_clist, "unselect_row", G_CALLBACK (sig_row_unselect), prefs);
	if (mail_config_get_signature_list () == NULL) {
		gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_delete, FALSE);
		gtk_widget_set_sensitive ((GtkWidget *) prefs->sig_edit, FALSE);
	}
	
	/* preview GtkHTML widget */
	widget = glade_xml_get_widget (gui, "scrolled-sig");
	prefs->sig_preview = (GtkHTML *) gtk_html_new ();
	g_signal_connect (prefs->sig_preview, "url_requested", G_CALLBACK (url_requested), NULL);
	gtk_widget_show (GTK_WIDGET (prefs->sig_preview));
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (prefs->sig_preview));
	
	if (GTK_CLIST (prefs->sig_clist)->rows)
		gtk_clist_select_row (GTK_CLIST (prefs->sig_clist), 0, 0);
	
	mail_config_signature_register_client ((MailConfigSignatureClient) sig_event_client, prefs);
}


GtkWidget *
mail_composer_prefs_new (void)
{
	MailComposerPrefs *new;
	
	new = (MailComposerPrefs *) g_object_new (mail_composer_prefs_get_type (), NULL);
	mail_composer_prefs_construct (new);
	
	return (GtkWidget *) new;
}


void
mail_composer_prefs_apply (MailComposerPrefs *prefs)
{
	GtkWidget *menu, *item;
	char *string;
	int val;
	
	/* General tab */
	
	/* Default Behavior */
	mail_config_set_send_html (gtk_toggle_button_get_active (prefs->send_html));
	mail_config_set_prompt_empty_subject (gtk_toggle_button_get_active (prefs->prompt_empty_subject));
	mail_config_set_prompt_only_bcc (gtk_toggle_button_get_active (prefs->prompt_bcc_only));
	
	menu = gtk_option_menu_get_menu (prefs->charset);
	string = e_charset_picker_get_charset (menu);
	if (string) {
		mail_config_set_default_charset (string);
		g_free (string);
	}
	
	/* Spell Checking */
#warning "gtkhtml propmanager"
#if 0
	gtk_html_propmanager_apply (prefs->pman);
#endif
	spell_apply (prefs);
	
	/* Forwards and Replies */
	menu = gtk_option_menu_get_menu (prefs->forward_style);
	item = gtk_menu_get_active (GTK_MENU (menu));
	val = GPOINTER_TO_INT (g_object_get_data((GObject *)item, "style"));
	mail_config_set_default_forward_style (val);
	
	menu = gtk_option_menu_get_menu (prefs->reply_style);
	item = gtk_menu_get_active (GTK_MENU (menu));
	val = GPOINTER_TO_INT (g_object_get_data((GObject *)item, "style"));
	mail_config_set_default_reply_style (val);
	
	/* Keyboard Shortcuts */
	/* FIXME: implement me */
	
	/* Signatures */
	/* FIXME: implement me */
}
