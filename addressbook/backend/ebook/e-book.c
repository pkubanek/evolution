/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * The Evolution addressbook client object.
 *
 * Author:
 *   Nat Friedman (nat@helixcode.com)
 *
 * Copyright 1999, 2000, Helix Code, Inc.
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <liboaf/liboaf.h>

#include "addressbook.h"
#include "e-card-cursor.h"
#include "e-book-listener.h"
#include "e-book.h"

GtkObjectClass *e_book_parent_class;

#define CARDSERVER_OAF_ID "OAFIID:GNOME_Evolution_Wombat_ServerFactory"

typedef enum {
	URINotLoaded,
	URILoading,
	URILoaded
} EBookLoadState;

struct _EBookPrivate {
	GNOME_Evolution_Addressbook_BookFactory  book_factory;
	EBookListener	      *listener;

	GNOME_Evolution_Addressbook_Book         corba_book;

	EBookLoadState         load_state;

	/*
	 * The operation queue.  New operations are appended to the
	 * end of the queue.  When responses come back from the PAS,
	 * the op structures are popped off the front of the queue.
	 */
	GList                 *pending_ops;

	guint sq_tag;
	GList *sq_pending;
};

enum {
	OPEN_PROGRESS,
	LINK_STATUS,
	LAST_SIGNAL
};

static guint e_book_signals [LAST_SIGNAL];

typedef struct {
	gpointer  cb;
	gpointer  closure;
	EBookViewListener *listener;
} EBookOp;

/*
 * Local response queue management.
 */
static void
e_book_queue_op (EBook    *book,
		 gpointer  cb,
		 gpointer  closure,
		 EBookViewListener *listener)
{
	EBookOp *op;

	op           = g_new0 (EBookOp, 1);
	op->cb       = cb;
	op->closure  = closure;
	op->listener = listener;

	book->priv->pending_ops =
		g_list_append (book->priv->pending_ops, op);
}

static EBookOp *
e_book_pop_op (EBook *book)
{
	GList   *popped;
	EBookOp *op;

	if (book->priv->pending_ops == NULL)
		return NULL;

	op = book->priv->pending_ops->data;

	popped = book->priv->pending_ops;
	book->priv->pending_ops =
		g_list_remove_link (book->priv->pending_ops,
				    book->priv->pending_ops);

	g_list_free_1 (popped);

	return op;
}

static void
e_book_do_response_create_card (EBook                 *book,
				EBookListenerResponse *resp)
{
	EBookOp *op;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_create_card: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	if (op->cb)
		((EBookIdCallback) op->cb) (book, resp->status, resp->id, op->closure);
	g_free (resp->id);
	g_free (op);
}

static void
e_book_do_response_generic (EBook                 *book,
			    EBookListenerResponse *resp)
{
	EBookOp *op;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_generic: Cannot find operation "
			   "in local op queue!\n");
	}

	if (op->cb)
		((EBookCallback) op->cb) (book, resp->status, op->closure);

	g_free (op);
}

static void
e_book_do_response_get_cursor (EBook                 *book,
			       EBookListenerResponse *resp)
{
	CORBA_Environment ev;
	EBookOp *op;
	ECardCursor *cursor;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_get_cursor: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	cursor = e_card_cursor_new(resp->cursor);

	if (op->cb)
		((EBookCursorCallback) op->cb) (book, resp->status, cursor, op->closure);

	/*
	 * Release the remote GNOME_Evolution_Addressbook_Book in the PAS.
	 */
	CORBA_exception_init (&ev);

	Bonobo_Unknown_unref  (resp->cursor, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_cursor: Exception unref'ing "
			   "remote GNOME_Evolution_Addressbook_CardCursor interface!\n");
		CORBA_exception_free (&ev);
		CORBA_exception_init (&ev);
	}
	
	CORBA_Object_release (resp->cursor, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_cursor: Exception releasing "
			   "remote GNOME_Evolution_Addressbook_CardCursor interface!\n");
	}

	CORBA_exception_free (&ev);

	gtk_object_unref(GTK_OBJECT(cursor));
	
	g_free (op);
}



static void
e_book_do_response_get_view (EBook                 *book,
			     EBookListenerResponse *resp)
{
	CORBA_Environment ev;
	EBookOp *op;
	EBookView *book_view;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_get_view: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	book_view = e_book_view_new(resp->book_view, op->listener);
	
	if (op->cb)
		((EBookBookViewCallback) op->cb) (book, resp->status, book_view, op->closure);

	/*
	 * Release the remote GNOME_Evolution_Addressbook_Book in the PAS.
	 */
	CORBA_exception_init (&ev);

	Bonobo_Unknown_unref  (resp->book_view, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_view: Exception unref'ing "
			   "remote GNOME_Evolution_Addressbook_BookView interface!\n");
		CORBA_exception_free (&ev);
		CORBA_exception_init (&ev);
	}
	
	CORBA_Object_release (resp->book_view, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_view: Exception releasing "
			   "remote GNOME_Evolution_Addressbook_BookView interface!\n");
	}

	CORBA_exception_free (&ev);

	gtk_object_unref(GTK_OBJECT(book_view));
	bonobo_object_unref(BONOBO_OBJECT(op->listener));
	
	g_free (op);
}

static void
e_book_do_response_get_changes (EBook                 *book,
				EBookListenerResponse *resp)
{
	CORBA_Environment ev;
	EBookOp *op;
	EBookView *book_view;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_get_changes: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	book_view = e_book_view_new (resp->book_view, op->listener);
	
	if (op->cb)
		((EBookBookViewCallback) op->cb) (book, resp->status, book_view, op->closure);

	/*
	 * Release the remote GNOME_Evolution_Addressbook_BookView in the PAS.
	 */
	CORBA_exception_init (&ev);

	Bonobo_Unknown_unref  (resp->book_view, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_changes: Exception unref'ing "
			   "remote GNOME_Evolution_Addressbook_BookView interface!\n");
		CORBA_exception_free (&ev);
		CORBA_exception_init (&ev);
	}
	
	CORBA_Object_release (resp->book_view, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_do_response_get_changes: Exception releasing "
			   "remote GNOME_Evolution_Addressbook_BookView interface!\n");
	}

	CORBA_exception_free (&ev);

	gtk_object_unref(GTK_OBJECT(book_view));
	bonobo_object_unref(BONOBO_OBJECT(op->listener));
	
	g_free (op);
}

static void
e_book_do_response_open (EBook                 *book,
			 EBookListenerResponse *resp)
{
	EBookOp *op;

	if (resp->status == E_BOOK_STATUS_SUCCESS) {
		book->priv->corba_book  = resp->book;
		book->priv->load_state  = URILoaded;
	}

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_open: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	if (op->cb)
		((EBookCallback) op->cb) (book, resp->status, op->closure);
	g_free (op);
}

static void
e_book_do_progress_event (EBook                 *book,
			  EBookListenerResponse *resp)
{
	gtk_signal_emit (GTK_OBJECT (book), e_book_signals [OPEN_PROGRESS],
			 resp->msg, resp->percent);

	g_free (resp->msg);
}

static void
e_book_do_link_event (EBook                 *book,
		      EBookListenerResponse *resp)
{
	gtk_signal_emit (GTK_OBJECT (book), e_book_signals [LINK_STATUS],
			 resp->connected);
}

static void
e_book_do_response_get_supported_fields (EBook                 *book,
					 EBookListenerResponse *resp)
{
	EBookOp *op;

	op = e_book_pop_op (book);

	if (op == NULL) {
		g_warning ("e_book_do_response_get_supported_fields: Cannot find operation "
			   "in local op queue!\n");
		return;
	}

	if (op->cb)
		((EBookFieldsCallback) op->cb) (book, resp->status, resp->fields, op->closure);
	g_free (op);
}

/*
 * Reading notices out of the EBookListener's queue.
 */
static void
e_book_check_listener_queue (EBookListener *listener, EBook *book)
{
	EBookListenerResponse *resp;

	resp = e_book_listener_pop_response (listener);

	if (resp == NULL)
		return;

	switch (resp->op) {
	case CreateCardResponse:
		e_book_do_response_create_card (book, resp);
		break;
	case RemoveCardResponse:
	case ModifyCardResponse:
	case AuthenticationResponse:
		e_book_do_response_generic (book, resp);
		break;
	case GetCursorResponse:
		e_book_do_response_get_cursor (book, resp);
		break;
	case GetBookViewResponse:
		e_book_do_response_get_view(book, resp);
		break;
	case GetChangesResponse:
		e_book_do_response_get_changes(book, resp);
		break;
	case OpenBookResponse:
		e_book_do_response_open (book, resp);
		break;
	case GetSupportedFieldsResponse:
		e_book_do_response_get_supported_fields (book, resp);
		break;

	case OpenProgressEvent:
		e_book_do_progress_event (book, resp);
		break;
	case LinkStatusEvent:
		e_book_do_link_event (book, resp);
		break;
	default:
		g_error ("EBook: Unknown operation %d in listener queue!\n",
			 resp->op);
	}

	g_free (resp);
}

/**
 * e_book_load_uri:
 */
gboolean
e_book_load_uri (EBook                     *book,
		 const char                *uri,
		 EBookCallback              open_response,
		 gpointer                   closure)
{
	CORBA_Environment ev;

	g_return_val_if_fail (book != NULL,          FALSE);
	g_return_val_if_fail (E_IS_BOOK (book),      FALSE);
	g_return_val_if_fail (uri != NULL,           FALSE);
	g_return_val_if_fail (open_response != NULL, FALSE);

	if (book->priv->load_state != URINotLoaded) {
		g_warning ("e_book_load_uri: Attempted to load a URI "
			   "on a book which already has a URI loaded!\n");
		return FALSE;
	}

	/*
	 * Create our local BookListener interface.
	 */
	book->priv->listener = e_book_listener_new ();
	if (book->priv->listener == NULL) {
		g_warning ("e_book_load_uri: Could not create EBookListener!\n");
		return FALSE;
	}

	gtk_signal_connect (GTK_OBJECT (book->priv->listener), "responses_queued",
			    e_book_check_listener_queue, book);

	/*
	 * Load the addressbook into the PAS.
	 */
	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_BookFactory_openBook (
		book->priv->book_factory, uri,
		bonobo_object_corba_objref (BONOBO_OBJECT (book->priv->listener)),
		&ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_load_uri: CORBA exception while opening addressbook!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);

	book->priv->load_state = URILoading;

	e_book_queue_op (book, open_response, closure, NULL);

	/* Now we play the waiting game. */

	return TRUE;
}

/**
 * e_book_unload_uri:
 */
void
e_book_unload_uri (EBook *book)
{
	CORBA_Environment ev;

	g_return_if_fail (book != NULL);
	g_return_if_fail (E_IS_BOOK (book));

	/*
	 * FIXME: Make sure this works if the URI is still being
	 * loaded.
	 */
	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_unload_uri: No URI is loaded!\n");
		return;
	}

	/*
	 * Release the remote GNOME_Evolution_Addressbook_Book in the PAS.
	 */
	CORBA_exception_init (&ev);

	Bonobo_Unknown_unref  (book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_unload_uri: Exception unref'ing "
			   "remote GNOME_Evolution_Addressbook_Book interface!\n");
		CORBA_exception_free (&ev);
		CORBA_exception_init (&ev);
	}
	
	CORBA_Object_release (book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_unload_uri: Exception releasing "
			   "remote book interface!\n");
	}

	CORBA_exception_free (&ev);

	bonobo_object_unref (BONOBO_OBJECT (book->priv->listener));

	book->priv->listener   = NULL;
	book->priv->load_state = URINotLoaded;
}

gboolean
e_book_load_local_address_book (EBook *book, EBookCallback open_response, gpointer closure)
{
	gchar *filename;
	gchar *uri;
	gboolean rv;

	g_return_val_if_fail (book != NULL,          FALSE);
	g_return_val_if_fail (E_IS_BOOK (book),      FALSE);
	g_return_val_if_fail (open_response != NULL, FALSE);

	filename = gnome_util_prepend_user_home ("evolution/local/Contacts/addressbook.db");
	uri = g_strdup_printf ("file://%s", filename);

	rv = e_book_load_uri (book, uri, open_response, closure);

	g_free (filename);
	g_free (uri);

	return rv;
}

char *
e_book_get_static_capabilities (EBook *book)
{
	CORBA_Environment ev;
	char *temp;
	char *ret_val;

	CORBA_exception_init (&ev);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_unload_uri: No URI is loaded!\n");
		return g_strdup("");
	}

	temp = GNOME_Evolution_Addressbook_Book_getStaticCapabilities(book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_static_capabilities: Exception "
			   "during get_static_capabilities!\n");
		CORBA_exception_free (&ev);
		return NULL;
	}

	ret_val = g_strdup(temp);
	CORBA_free(temp);

	CORBA_exception_free (&ev);

	return ret_val;
}

gboolean
e_book_get_supported_fields (EBook              *book,
			     EBookFieldsCallback cb,
			     gpointer            closure)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_unload_uri: No URI is loaded!\n");
		return FALSE;
	}

	GNOME_Evolution_Addressbook_Book_getSupportedFields(book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_supported_fields: Exception "
			   "during get_supported_fields!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, NULL);

	return TRUE;
}

static gboolean
e_book_construct (EBook *book)
{
	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);

	/*
	 * Connect to the Personal Addressbook Server.
	 */

	book->priv->book_factory = (GNOME_Evolution_Addressbook_BookFactory)
		oaf_activate_from_id (CARDSERVER_OAF_ID, 0, NULL, NULL);
	if (book->priv->book_factory == CORBA_OBJECT_NIL) {
		g_warning ("e_book_construct: Could not obtain a handle "
			   "to the Personal Addressbook Server!\n");
		return FALSE;
	}

	return TRUE;
}

/**
 * e_book_new:
 */
EBook *
e_book_new (void)
{
	EBook *book;

	book = gtk_type_new (E_BOOK_TYPE);

	if (! e_book_construct (book)) {
		gtk_object_unref (GTK_OBJECT (book));
		return NULL;
	}

	return book;
}

/* User authentication. */

void
e_book_authenticate_user (EBook         *book,
			  const char    *user,
			  const char    *passwd,
			  EBookCallback cb,
			  gpointer      closure)
{
	CORBA_Environment  ev;

	g_return_if_fail (book != NULL);
	g_return_if_fail (E_IS_BOOK (book));

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_authenticate_user: No URI loaded!\n");
		return;
	}

	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_Book_authenticateUser (book->priv->corba_book,
							   user,
							   passwd,
							   &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_authenticate_user: Exception authenticating user with the PAS!\n");
		CORBA_exception_free (&ev);
		return;
	}

	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, NULL);
}

/* Fetching cards */

/**
 * e_book_get_card:
 */
ECard *
e_book_get_card (EBook       *book,
		 const char  *id)
{
	char  *vcard;
	ECard *card;

	g_return_val_if_fail (book != NULL,     NULL);
	g_return_val_if_fail (E_IS_BOOK (book), NULL);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_get_card: No URI loaded!\n");
		return NULL;
	}

	vcard = e_book_get_vcard (book, id);

	if (vcard == NULL) {
		g_warning ("e_book_get_card: Got bogus VCard from PAS!\n");
		return NULL;
	}

	card = e_card_new (vcard);
	g_free(vcard);
	
	e_card_set_id(card, id);

	return card;
}

/**
 * e_book_get_vcard:
 */
char *
e_book_get_vcard (EBook       *book,
		  const char  *id)
{
	CORBA_Environment  ev;
	char              *retval;
	char              *vcard;

	g_return_val_if_fail (book != NULL,     NULL);
	g_return_val_if_fail (E_IS_BOOK (book), NULL);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_get_vcard: No URI loaded!\n");
		return NULL;
	}

	CORBA_exception_init (&ev);

	vcard = GNOME_Evolution_Addressbook_Book_getVCard (book->priv->corba_book,
					  (GNOME_Evolution_Addressbook_CardId) id,
					  &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_vcard: Exception getting VCard from PAS!\n");
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	if (vcard == NULL || strlen (vcard) == 0) {
		g_warning ("e_book_get_vcard: Got NULL VCard from PAS!\n");
		return NULL;
	}

	retval = g_strdup (vcard);
	CORBA_free (vcard);

	return retval;
}

/* Deleting cards. */

/**
 * e_book_remove_card:
 */
gboolean
e_book_remove_card (EBook         *book,
		    ECard         *card,
		    EBookCallback  cb,
		    gpointer       closure)
{
	const char *id;

	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (card != NULL,     FALSE);
	g_return_val_if_fail (E_IS_CARD (card), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_remove_card: No URI loaded!\n");
		return FALSE;
	}

	id = e_card_get_id (card);
	g_assert (id != NULL);

	return e_book_remove_card_by_id (book, id, cb, closure);
}

/**
 * e_book_remove_card_by_id:
 */
gboolean
e_book_remove_card_by_id (EBook         *book,
			  const char    *id,
			  EBookCallback  cb,
			  gpointer       closure)

{
	CORBA_Environment ev;

	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (id != NULL,       FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_remove_card_by_id: No URI loaded!\n");
		return FALSE;
	}

	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_Book_removeCard (
		book->priv->corba_book, (const GNOME_Evolution_Addressbook_CardId) id, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_remove_card_by_id: CORBA exception "
			   "talking to PAS!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, NULL);

	return TRUE;
}

/* Adding cards. */

/**
 * e_book_add_card:
 */
gboolean
e_book_add_card (EBook           *book,
		 ECard           *card,
		 EBookIdCallback  cb,
		 gpointer         closure)

{
	char     *vcard;
	gboolean  retval;

	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (card != NULL,     FALSE);
	g_return_val_if_fail (E_IS_CARD (card), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_add_card: No URI loaded!\n");
		return FALSE;
	}

	vcard = e_card_get_vcard (card);

	if (vcard == NULL) {
		g_warning ("e_book_add_card: Cannot convert card to VCard string!\n");
		return FALSE;
	}

	retval = e_book_add_vcard (book, vcard, cb, closure);

	g_free (vcard);

	return retval;
}

/**
 * e_book_add_vcard:
 */
gboolean
e_book_add_vcard (EBook           *book,
		  const char      *vcard,
		  EBookIdCallback  cb,
		  gpointer         closure)
{
	CORBA_Environment ev;

	g_return_val_if_fail (book  != NULL,    FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (vcard != NULL,    FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_add_vcard: No URI loaded!\n");
		return FALSE;
	}

	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_Book_addCard (
		book->priv->corba_book, (const GNOME_Evolution_Addressbook_VCard) vcard, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_add_vcard: Exception adding card to PAS!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);

	e_book_queue_op (book, (EBookCallback) cb, closure, NULL);

	return TRUE;
}

/* Modifying cards. */

/**
 * e_book_commit_card:
 */
gboolean
e_book_commit_card (EBook         *book,
		    ECard         *card,
		    EBookCallback  cb,
		    gpointer       closure)
{
	char     *vcard;
	gboolean  retval;
	
	g_return_val_if_fail (book  != NULL,    FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (card != NULL,     FALSE);
	g_return_val_if_fail (E_IS_CARD (card), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_commit_card: No URI loaded!\n");
		return FALSE;
	}

	vcard = e_card_get_vcard (card);

	if (vcard == NULL) {
		g_warning ("e_book_commit_card: Error "
			   "getting VCard for card!\n");
		return FALSE;
	}

	retval = e_book_commit_vcard (book, vcard, cb, closure);

	g_free (vcard);

	return retval;
}

/**
 * e_book_commit_vcard:
 */
gboolean
e_book_commit_vcard (EBook         *book,
		     const char    *vcard,
		     EBookCallback  cb,
		     gpointer       closure)
{
	CORBA_Environment ev;

	g_return_val_if_fail (book  != NULL,    FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);
	g_return_val_if_fail (vcard != NULL,    FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_commit_vcard: No URI loaded!\n");
		return FALSE;
	}

	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_Book_modifyCard (
		book->priv->corba_book, (const GNOME_Evolution_Addressbook_VCard) vcard, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_commit_vcard: Exception "
			   "modifying card in PAS!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, NULL);

	return TRUE;
}

/**
 * e_book_check_connection:
 */
gboolean
e_book_check_connection (EBook *book)
{
	CORBA_Environment ev;

	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_check_connection: No URI loaded!\n");
		return FALSE;
	}

	CORBA_exception_init (&ev);

	GNOME_Evolution_Addressbook_Book_checkConnection (book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_check_connection: Exception "
			   "querying the PAS!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	CORBA_exception_free (&ev);

	return TRUE;
}

gboolean
e_book_get_cursor       (EBook               *book,
			 gchar               *query,
			 EBookCursorCallback  cb,
			 gpointer             closure)
{
	CORBA_Environment ev;
  
	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_check_connection: No URI loaded!\n");
		return FALSE;
	}
	
	CORBA_exception_init (&ev);
	
	GNOME_Evolution_Addressbook_Book_getCursor (book->priv->corba_book, query, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_all_cards: Exception "
			   "querying list of cards!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, NULL);

	return TRUE;
}

gboolean
e_book_get_book_view       (EBook                 *book,
			    gchar                 *query,
			    EBookBookViewCallback  cb,
			    gpointer               closure)
{
	CORBA_Environment ev;
	EBookViewListener *listener;
  
	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_get_book_view: No URI loaded!\n");
		return FALSE;
	}

	listener = e_book_view_listener_new();
	
	CORBA_exception_init (&ev);
	
	GNOME_Evolution_Addressbook_Book_getBookView (book->priv->corba_book, bonobo_object_corba_objref(BONOBO_OBJECT(listener)), query, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_book_view: Exception "
			   "getting book_view!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, listener);

	return TRUE;
}

gboolean
e_book_get_changes         (EBook                 *book,
			    gchar                 *changeid,
			    EBookBookViewCallback  cb,
			    gpointer               closure)
{
	CORBA_Environment ev;
	EBookViewListener *listener;
  
	g_return_val_if_fail (book != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK (book), FALSE);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_get_changes: No URI loaded!\n");
		return FALSE;
	}

	listener = e_book_view_listener_new();
	
	CORBA_exception_init (&ev);
	
	GNOME_Evolution_Addressbook_Book_getChanges (book->priv->corba_book, bonobo_object_corba_objref(BONOBO_OBJECT(listener)), changeid, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_changes: Exception "
			   "getting changes!\n");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	CORBA_exception_free (&ev);

	e_book_queue_op (book, cb, closure, listener);

	return TRUE;
}

/*
 *
 * Simple Query Stuff
 *
 */

typedef struct _SimpleQueryInfo SimpleQueryInfo;
struct _SimpleQueryInfo {
	guint tag;
	EBook *book;
	gchar *query;
	EBookSimpleQueryCallback cb;
	gpointer closure;
	EBookView *view;
	guint add_tag;
	guint seq_complete_tag;
	GList *cards;
};

static SimpleQueryInfo *
simple_query_new (EBook *book, char *query, EBookSimpleQueryCallback cb, gpointer closure)
{
	SimpleQueryInfo *sq = g_new0 (SimpleQueryInfo, 1);

	sq->tag = ++book->priv->sq_tag;
	sq->book = book;
	gtk_object_ref (GTK_OBJECT (book));
	sq->query = g_strdup_printf (query);
	sq->cb = cb;
	sq->closure = closure;

	/* Automatically add ourselves to the EBook's pending list. */
	book->priv->sq_pending = g_list_prepend (book->priv->sq_pending, sq);

	return sq;
}

static void
simple_query_free (SimpleQueryInfo *sq)
{
	GList *i;
	gboolean found = FALSE;

	/* Find & remove ourselves from the EBook's pending list. */
	for (i = sq->book->priv->sq_pending; i != NULL; i = g_list_next (i)) {
		if (i->data == sq) {
			sq->book->priv->sq_pending = g_list_remove_link (sq->book->priv->sq_pending, i);
			g_list_free_1 (i);
			i = NULL;
			found = TRUE;
		} else
			i = g_list_next (i);
	}

	g_assert (found);
	
	g_free (sq->query);

	if (sq->add_tag)
		gtk_signal_disconnect (GTK_OBJECT (sq->view), sq->add_tag);
	if (sq->seq_complete_tag)
		gtk_signal_disconnect (GTK_OBJECT (sq->view), sq->seq_complete_tag);

	if (sq->view)
		gtk_object_unref (GTK_OBJECT (sq->view));

	if (sq->book)
		gtk_object_unref (GTK_OBJECT (sq->book));

	g_list_foreach (sq->cards, (GFunc) gtk_object_unref, NULL);
	g_list_free (sq->cards);

	g_free (sq);
}

static void
simple_query_card_added_cb (EBookView *view, const GList *cards, gpointer closure)
{
	SimpleQueryInfo *sq = closure;
	
	sq->cards = g_list_concat (sq->cards, g_list_copy ((GList *) cards));
	g_list_foreach ((GList *) cards, (GFunc) gtk_object_ref, NULL);
}

static void
simple_query_sequence_complete_cb (EBookView *view, gpointer closure)
{
	SimpleQueryInfo *sq = closure;

	sq->cb (sq->book, E_BOOK_SIMPLE_QUERY_STATUS_SUCCESS, sq->cards, sq->closure);
	simple_query_free (sq);
}

static void
simple_query_book_view_cb (EBook *book, EBookStatus status, EBookView *book_view, gpointer closure)
{
	SimpleQueryInfo *sq = closure;

	if (status != E_BOOK_STATUS_SUCCESS) {
		sq->cb (sq->book, E_BOOK_SIMPLE_QUERY_STATUS_OTHER_ERROR, NULL, sq->closure);
		simple_query_free (sq);
		return;
	}

	sq->view = book_view;
	gtk_object_ref (GTK_OBJECT (book_view));

	sq->add_tag = gtk_signal_connect (GTK_OBJECT (sq->view),
					  "card_added",
					  GTK_SIGNAL_FUNC (simple_query_card_added_cb),
					  sq);
	sq->seq_complete_tag = gtk_signal_connect (GTK_OBJECT (sq->view),
						   "sequence_complete",
						   GTK_SIGNAL_FUNC (simple_query_sequence_complete_cb),
						   sq);
}

guint
e_book_simple_query (EBook *book, char *query, EBookSimpleQueryCallback cb, gpointer closure)
{
	SimpleQueryInfo *sq;

	g_return_val_if_fail (book && E_IS_BOOK (book), 0);
	g_return_val_if_fail (query, 0);
	g_return_val_if_fail (cb, 0);

	sq = simple_query_new (book, query, cb, closure);
	e_book_get_book_view (book, query, simple_query_book_view_cb, sq);

	return sq->tag;
}

void
e_book_simple_query_cancel (EBook *book, guint tag)
{
	GList *i;

	g_return_if_fail (book && E_IS_BOOK (book));

	for (i=book->priv->sq_pending; i != NULL; i=g_list_next (i)) {
		SimpleQueryInfo *sq = i->data;

		if (sq->tag == tag) {
			sq->cb (sq->book, E_BOOK_SIMPLE_QUERY_STATUS_CANCELLED, NULL, sq->closure);
			simple_query_free (sq);
			return;
		}
	}
	g_warning ("Simple query tag %d is unknown", tag);
}

/**
 * e_book_get_name:
 */
char *
e_book_get_name (EBook *book)
{
	CORBA_Environment  ev;
	char              *retval;
	char              *name;

	g_return_val_if_fail (book != NULL,     NULL);
	g_return_val_if_fail (E_IS_BOOK (book), NULL);

	if (book->priv->load_state != URILoaded) {
		g_warning ("e_book_get_name: No URI loaded!\n");
		return NULL;
	}

	CORBA_exception_init (&ev);

	name = GNOME_Evolution_Addressbook_Book_getName (book->priv->corba_book, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_get_name: Exception getting name from PAS!\n");
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	if (name == NULL) {
		g_warning ("e_book_get_name: Got NULL name from PAS!\n");
		return NULL;
	}

	retval = g_strdup (name);
	CORBA_free (name);

	return retval;
}

static void
e_book_init (EBook *book)
{
	book->priv             = g_new0 (EBookPrivate, 1);
	book->priv->load_state = URINotLoaded;
}

static void
e_book_destroy (GtkObject *object)
{
	EBook             *book = E_BOOK (object);
	CORBA_Environment  ev;

	if (book->priv->load_state == URILoaded)
		e_book_unload_uri (book);

	CORBA_exception_init (&ev);

	CORBA_Object_release (book->priv->book_factory, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("EBook: Exception while releasing BookFactory\n");

		CORBA_exception_free (&ev);
		CORBA_exception_init (&ev);
	}

	g_free (book->priv);

	GTK_OBJECT_CLASS (e_book_parent_class)->destroy (object);
}

static void
e_book_class_init (EBookClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	e_book_parent_class = gtk_type_class (gtk_object_get_type ());

	e_book_signals [LINK_STATUS] =
		gtk_signal_new ("link_status",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookClass, link_status),
				gtk_marshal_NONE__BOOL,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	gtk_object_class_add_signals (object_class, e_book_signals,
				      LAST_SIGNAL);

	object_class->destroy = e_book_destroy;
}

/**
 * e_book_get_type:
 */
GtkType
e_book_get_type (void)
{
	static GtkType type = 0;

	if (! type) {
		GtkTypeInfo info = {
			"EBook",
			sizeof (EBook),
			sizeof (EBookClass),
			(GtkClassInitFunc)  e_book_class_init,
			(GtkObjectInitFunc) e_book_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_object_get_type (), &info);
	}

	return type;
}
