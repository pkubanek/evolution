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
 *		Shreyas Srinivasan <sshreyas@novell.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#include <gtk/gtk.h>

#define TYPE_PROXY_LOGIN       (proxy_login_get_type ())
#define PROXY_LOGIN(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PROXY_LOGIN, proxyLogin))
#define PROXY_LOGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PROXY_LOGIN, proxyLoginClass))
#define IS_PROXY_LOGIN(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PROXY_LOGIN))
#define IS_PROXY_LOGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PROXY_LOGIN))

typedef struct _proxyLogin		proxyLogin;
typedef struct _proxyLoginClass        proxyLoginClass;
typedef struct _proxyLoginPrivate	proxyLoginPrivate;

struct _proxyLogin{
    GObject object;

    /*Account*/
    EAccount *account;

    /*List of proxies*/
    GList *proxy_list;

    /* Private Dialog Information*/
    proxyLoginPrivate *priv;
};

struct _proxyLoginClass {
	GObjectClass parent_class;
};

GType proxy_login_get_type (void);
proxyLogin * proxy_login_new (void);
static void proxy_login_cb (GtkDialog *dialog, gint state);
static void proxy_login_add_new_store (char *uri, CamelStore *store, void *user_data);
static void proxy_login_setup_tree_view (void);
void org_gnome_proxy_account_login (EPopup *ep, EPopupItem *p, void *data);
proxyLogin* proxy_dialog_new (void);
static void proxy_soap_login (char *email);
char *parse_email_for_name (char *email);
static void proxy_login_update_tree (void);
static void proxy_login_tree_view_changed_cb(GtkDialog *dialog);
void org_gnome_create_proxy_login_option(EPlugin *ep, EMPopupTargetFolder *t);
static int proxy_get_password (EAccount *account, char **user_name, char **password);
