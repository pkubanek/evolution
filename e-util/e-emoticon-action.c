<<<<<<< HEAD
/*
 * e-emoticon-action.c
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2012 Dan Vrátil <dvratil@redhat.com>
=======
/* e-emoticon-action.c
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "e-emoticon-action.h"

#include "e-emoticon-chooser.h"
#include "e-emoticon-chooser-menu.h"
#include "e-emoticon-tool-button.h"

<<<<<<< HEAD
#define E_EMOTICON_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_EMOTICON_ACTION, EEmoticonActionPrivate))

=======
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
struct _EEmoticonActionPrivate {
	GList *choosers;
	EEmoticonChooser *current_chooser;
};

enum {
	PROP_0,
	PROP_CURRENT_FACE
};

<<<<<<< HEAD
/* Forward Declarations */
static void	e_emoticon_action_interface_init
					(EEmoticonChooserInterface *interface);

G_DEFINE_TYPE_WITH_CODE (
	EEmoticonAction,
	e_emoticon_action,
	GTK_TYPE_ACTION,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_EMOTICON_CHOOSER,
		e_emoticon_action_interface_init))

static void
emoticon_action_proxy_item_activated_cb (EEmoticonAction *action,
                                         EEmoticonChooser *chooser)
=======
static gpointer parent_class;

static void
emoticon_action_proxy_item_activated_cb (EEmoticonAction *action,
					 EEmoticonChooser *chooser)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	action->priv->current_chooser = chooser;

	g_signal_emit_by_name (action, "item-activated");
}

static void
emoticon_action_set_property (GObject *object,
<<<<<<< HEAD
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
=======
			      guint property_id,
			      const GValue *value,
			      GParamSpec *pspec)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	switch (property_id) {
		case PROP_CURRENT_FACE:
			e_emoticon_chooser_set_current_emoticon (
				E_EMOTICON_CHOOSER (object),
				g_value_get_boxed (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
emoticon_action_get_property (GObject *object,
<<<<<<< HEAD
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
=======
			      guint property_id,
			      GValue *value,
			      GParamSpec *pspec)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	switch (property_id) {
		case PROP_CURRENT_FACE:
			g_value_set_boxed (
				value, e_emoticon_chooser_get_current_emoticon (
				E_EMOTICON_CHOOSER (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
emoticon_action_finalize (GObject *object)
{
	EEmoticonActionPrivate *priv;

<<<<<<< HEAD
	priv = E_EMOTICON_ACTION_GET_PRIVATE (object);
=======
	priv = E_EMOTICON_ACTION (object)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	g_list_free (priv->choosers);

	/* Chain up to parent's finalize() method. */
<<<<<<< HEAD
	G_OBJECT_CLASS (e_emoticon_action_parent_class)->finalize (object);
=======
	G_OBJECT_CLASS (parent_class)->finalize (object);
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
}

static void
emoticon_action_activate (GtkAction *action)
{
	EEmoticonActionPrivate *priv;

<<<<<<< HEAD
	priv = E_EMOTICON_ACTION_GET_PRIVATE (action);
=======
	priv = E_EMOTICON_ACTION (action)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	priv->current_chooser = NULL;
}

static GtkWidget *
emoticon_action_create_menu_item (GtkAction *action)
{
	GtkWidget *item;
	GtkWidget *menu;

	item = gtk_image_menu_item_new ();
	menu = gtk_action_create_menu (action);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	gtk_widget_show (menu);

	return item;
}

static GtkWidget *
emoticon_action_create_tool_item (GtkAction *action)
{
	return GTK_WIDGET (e_emoticon_tool_button_new ());
}

static void
emoticon_action_connect_proxy (GtkAction *action,
<<<<<<< HEAD
                               GtkWidget *proxy)
{
	EEmoticonActionPrivate *priv;

	priv = E_EMOTICON_ACTION_GET_PRIVATE (action);
=======
			       GtkWidget *proxy)
{
	EEmoticonActionPrivate *priv;

	priv = E_EMOTICON_ACTION (action)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	if (!E_IS_EMOTICON_CHOOSER (proxy))
		goto chainup;

	if (g_list_find (priv->choosers, proxy) != NULL)
		goto chainup;

	g_signal_connect_swapped (
		proxy, "item-activated",
		G_CALLBACK (emoticon_action_proxy_item_activated_cb), action);

chainup:
	/* Chain up to parent's connect_proxy() method. */
<<<<<<< HEAD
	GTK_ACTION_CLASS (e_emoticon_action_parent_class)->
		connect_proxy (action, proxy);
=======
	GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
}

static void
emoticon_action_disconnect_proxy (GtkAction *action,
<<<<<<< HEAD
                                  GtkWidget *proxy)
{
	EEmoticonActionPrivate *priv;

	priv = E_EMOTICON_ACTION_GET_PRIVATE (action);
=======
				  GtkWidget *proxy)
{
	EEmoticonActionPrivate *priv;

	priv = E_EMOTICON_ACTION (action)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	priv->choosers = g_list_remove (priv->choosers, proxy);

	/* Chain up to parent's disconnect_proxy() method. */
<<<<<<< HEAD
	GTK_ACTION_CLASS (e_emoticon_action_parent_class)->
		disconnect_proxy (action, proxy);
=======
	GTK_ACTION_CLASS (parent_class)->disconnect_proxy (action, proxy);
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
}

static GtkWidget *
emoticon_action_create_menu (GtkAction *action)
{
	EEmoticonActionPrivate *priv;
	GtkWidget *widget;

<<<<<<< HEAD
	priv = E_EMOTICON_ACTION_GET_PRIVATE (action);
=======
	priv = E_EMOTICON_ACTION (action)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	widget = e_emoticon_chooser_menu_new ();

	g_signal_connect_swapped (
		widget, "item-activated",
		G_CALLBACK (emoticon_action_proxy_item_activated_cb), action);

	priv->choosers = g_list_prepend (priv->choosers, widget);

	return widget;
}

static EEmoticon *
emoticon_action_get_current_emoticon (EEmoticonChooser *chooser)
{
	EEmoticonActionPrivate *priv;
	EEmoticon *emoticon = NULL;

<<<<<<< HEAD
	priv = E_EMOTICON_ACTION_GET_PRIVATE (chooser);
=======
	priv = E_EMOTICON_ACTION (chooser)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	if (priv->current_chooser != NULL)
		emoticon = e_emoticon_chooser_get_current_emoticon (
			priv->current_chooser);

	return emoticon;
}

static void
emoticon_action_set_current_emoticon (EEmoticonChooser *chooser,
<<<<<<< HEAD
                                      EEmoticon *emoticon)
=======
				      EEmoticon *emoticon)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	EEmoticonActionPrivate *priv;
	GList *iter;

<<<<<<< HEAD
	priv = E_EMOTICON_ACTION_GET_PRIVATE (chooser);
=======
	priv = E_EMOTICON_ACTION (chooser)->priv;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*

	for (iter = priv->choosers; iter != NULL; iter = iter->next) {
		EEmoticonChooser *proxy_chooser = iter->data;

		e_emoticon_chooser_set_current_emoticon (proxy_chooser, emoticon);
	}
}

static void
<<<<<<< HEAD
e_emoticon_action_class_init (EEmoticonActionClass *class)
=======
emoticon_action_class_init (EEmoticonActionClass *class)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	GObjectClass *object_class;
	GtkActionClass *action_class;

<<<<<<< HEAD
=======
	parent_class = g_type_class_peek_parent (class);
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
	g_type_class_add_private (class, sizeof (EEmoticonAction));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = emoticon_action_set_property;
	object_class->get_property = emoticon_action_get_property;
	object_class->finalize = emoticon_action_finalize;

	action_class = GTK_ACTION_CLASS (class);
	action_class->activate = emoticon_action_activate;
	action_class->create_menu_item = emoticon_action_create_menu_item;
	action_class->create_tool_item = emoticon_action_create_tool_item;
	action_class->connect_proxy = emoticon_action_connect_proxy;
	action_class->disconnect_proxy = emoticon_action_disconnect_proxy;
	action_class->create_menu = emoticon_action_create_menu;

	g_object_class_override_property (
		object_class, PROP_CURRENT_FACE, "current-emoticon");
}

static void
<<<<<<< HEAD
e_emoticon_action_interface_init (EEmoticonChooserInterface *interface)
{
	interface->get_current_emoticon = emoticon_action_get_current_emoticon;
	interface->set_current_emoticon = emoticon_action_set_current_emoticon;
}

static void
e_emoticon_action_init (EEmoticonAction *action)
{
	action->priv = E_EMOTICON_ACTION_GET_PRIVATE (action);
=======
emoticon_action_iface_init (EEmoticonChooserIface *iface)
{
	iface->get_current_emoticon = emoticon_action_get_current_emoticon;
	iface->set_current_emoticon = emoticon_action_set_current_emoticon;
}

static void
emoticon_action_init (EEmoticonAction *action)
{
	action->priv = G_TYPE_INSTANCE_GET_PRIVATE (
		action, E_TYPE_EMOTICON_ACTION, EEmoticonActionPrivate);
}

GType
e_emoticon_action_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo type_info = {
			sizeof (EEmoticonActionClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) emoticon_action_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,  /* class_data */
			sizeof (EEmoticonAction),
			0,     /* n_preallocs */
			(GInstanceInitFunc) emoticon_action_init,
			NULL   /* value_table */
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) emoticon_action_iface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL  /* interemoticon_data */
		};

		type = g_type_register_static (
			GTK_TYPE_ACTION, "EEmoticonAction", &type_info, 0);

		g_type_add_interface_static (
			type, E_TYPE_EMOTICON_CHOOSER, &iface_info);
	}

	return type;
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
}

GtkAction *
e_emoticon_action_new (const gchar *name,
<<<<<<< HEAD
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *stock_id)
=======
		       const gchar *label,
		       const gchar *tooltip,
		       const gchar *stock_id)
>>>>>>> Import GtkhtmlFace* classes as EEmoticon*
{
	g_return_val_if_fail (name != NULL, NULL);

	return g_object_new (
		E_TYPE_EMOTICON_ACTION, "name", name, "label", label,
		"tooltip", tooltip, "stock-id", stock_id, NULL);
}
