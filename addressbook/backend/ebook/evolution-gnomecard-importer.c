/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <stdio.h>

#include <liboaf/liboaf.h>
#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>

#include <e-book.h>

#include <importer/evolution-importer.h>
#include <importer/GNOME_Evolution_Importer.h>

#define COMPONENT_FACTORY_IID "OAFIID:GNOME_Evolution_Addressbook_GnomeCard_ImporterFactory"

static BonoboGenericFactory *factory = NULL;

typedef struct {
	char *filename;
	GList *cardlist;
	GList *iterator;
	EBook *book;
	gboolean ready;
} GnomeCardImporter;

static void
add_card_cb (EBook *book, EBookStatus status, const gchar *id, gpointer closure)
{
	ECard *card = E_CARD(closure);
	char *vcard = e_card_get_vcard(card);
	g_print ("Saved card: %s\n", vcard);
	g_free(vcard);
	gtk_object_unref(GTK_OBJECT(card));
}

static void
book_open_cb (EBook *book, EBookStatus status, gpointer closure)
{
	GnomeCardImporter *gci = (GnomeCardImporter *) closure;

	gci->cardlist = e_card_load_cards_from_file(gci->filename);
	gci->ready = TRUE;
}

static void
ebook_create (GnomeCardImporter *gci)
{
	gchar *path, *uri;
	
	gci->book = e_book_new ();

	if (!gci->book) {
		printf ("%s: %s(): Couldn't create EBook, bailing.\n",
			__FILE__,
			__FUNCTION__);
		return;
	}

	path = g_concat_dir_and_file (g_get_home_dir (),
				      "evolution/local/Contacts/addressbook.db");
	uri = g_strdup_printf ("file://%s", path);
	g_free (path);

	if (! e_book_load_uri (gci->book, uri, book_open_cb, gci)) {
		printf ("error calling load_uri!\n");
	}
	g_free(uri);
}

/* EvolutionImporter methods */
static void
process_item_fn (EvolutionImporter *importer,
		 CORBA_Object listener,
		 void *closure,
		 CORBA_Environment *ev)
{
	GnomeCardImporter *gci = (GnomeCardImporter *) closure;
	ECard *card;

	if (gci->iterator == NULL)
		gci->iterator = gci->cardlist;

	if (gci->ready == FALSE) {
		GNOME_Evolution_ImporterListener_notifyResult (listener,
							       GNOME_Evolution_ImporterListener_NOT_READY,
							       gci->iterator ? TRUE : FALSE, 
							       ev);
		return;
	}
	
	if (gci->iterator == NULL) {
		GNOME_Evolution_ImporterListener_notifyResult (listener,
							       GNOME_Evolution_ImporterListener_UNSUPPORTED_OPERATION,
							       FALSE, ev);
		return;
	}

	card = gci->iterator->data;
	e_book_add_card (gci->book, card, add_card_cb, card);

	gci->iterator = gci->iterator->next;

	GNOME_Evolution_ImporterListener_notifyResult (listener,
						       GNOME_Evolution_ImporterListener_OK,
						       gci->iterator ? TRUE : FALSE, 
						       ev);
	if (ev->_major != CORBA_NO_EXCEPTION) {
		g_warning ("Error notifying listeners.");
	}
	
	return;
}

static char *supported_extensions[3] = {
	".vcf", ".gcrd", NULL
};

static gboolean
support_format_fn (EvolutionImporter *importer,
		   const char *filename,
		   void *closure)
{
	char *ext;
	int i;

	ext = strrchr (filename, '.');
	for (i = 0; supported_extensions[i] != NULL; i++) {
		if (strcmp (supported_extensions[i], ext) == 0)
			return TRUE;
	}

	return FALSE;
}

static void
importer_destroy_cb (GtkObject *object,
		     GnomeCardImporter *gci)
{
	gtk_main_quit ();
}

static gboolean
load_file_fn (EvolutionImporter *importer,
	      const char *filename,
	      const char *folderpath,
	      void *closure)
{
	GnomeCardImporter *gci;

	gci = (GnomeCardImporter *) closure;
	gci->filename = g_strdup (filename);
	gci->cardlist = NULL;
	gci->iterator = NULL;
	gci->ready = FALSE;
	ebook_create (gci);

	return TRUE;
}
					   
static BonoboObject *
factory_fn (BonoboGenericFactory *_factory,
	    void *closure)
{
	EvolutionImporter *importer;
	GnomeCardImporter *gci;

	gci = g_new (GnomeCardImporter, 1);
	importer = evolution_importer_new (support_format_fn, load_file_fn, 
					   process_item_fn, NULL, gci);
	
	gtk_signal_connect (GTK_OBJECT (importer), "destroy",
			    GTK_SIGNAL_FUNC (importer_destroy_cb), gci);
	
	return BONOBO_OBJECT (importer);
}

static void
importer_init (void)
{
	if (factory != NULL)
		return;

	factory = bonobo_generic_factory_new (COMPONENT_FACTORY_IID, 
					      factory_fn, NULL);

	if (factory == NULL) {
		g_error ("Unable to create factory");
	}
}

int
main (int argc,
      char **argv)
{
	CORBA_ORB orb;
	
	gnome_init_with_popt_table ("Evolution-GnomeCard-Importer",
				    "0.0", argc, argv, oaf_popt_options, 0,
				    NULL);
	orb = oaf_init (argc, argv);
	if (bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL) == FALSE) {
		g_error ("Could not initialize Bonobo.");
	}

	importer_init ();
	bonobo_main ();

	return 0;
}


