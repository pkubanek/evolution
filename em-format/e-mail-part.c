/*
 * e-mail-part.c
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
 */

/**
 * EMailPart:
 *
 * The #EMailPart is a wrapper around #CamelMimePart which holds additional
 * information about the mime part, like it's ID, encryption type etc.
 *
 * #EMailPart is not GObject-based, but has a simple reference counting.
 *
 * Each #EMailPart must have a unique ID. The ID is a dot-separated
 * hierarchical description of the location of the part within the email
 * message.
 */

#include "e-mail-part.h"

#include <string.h>

struct _EMailPartPrivate {
	guint ref_cnt;
	gsize instance_size;
	GFreeFunc free_func;
};

static void
mail_part_validity_pair_free (gpointer ptr)
{
	EMailPartValidityPair *pair = ptr;

	if (!pair)
		return;

	camel_cipher_validity_free (pair->validity);
	g_free (pair);
}

static void
mail_part_free (EMailPart *part)
{
	EMailPartValidityPair *pair;

	if (!part)
		return;

	if (part->part) {
		g_object_unref (part->part);
		part->part = NULL;
	}

	if (part->cid) {
		g_free (part->cid);
		part->cid = NULL;
	}

	if (part->mime_type) {
		g_free (part->mime_type);
		part->mime_type = NULL;
	}

	while ((pair = g_queue_pop_head (&part->validities)) != NULL)
		mail_part_validity_pair_free (pair);

	if (part->priv->free_func) {
		part->priv->free_func (part);
		part->priv->free_func = NULL;
	}

	if (part->id) {
		g_free (part->id);
		part->id = NULL;
	}

	g_free (part->priv);
	part->priv = NULL;

	g_free (part);
}

/**
 * e_mail_part_new:
 * @part: (allow-none) a #CamelMimePart or %NULL
 * @id: part ID
 *
 * Creates a new #EMailPart for given mime part.
 *
 * Return value: a new #EMailPart
 */
EMailPart *
e_mail_part_new (CamelMimePart *part,
                 const gchar *id)
{
	return e_mail_part_subclass_new (part, id, sizeof (EMailPart), NULL);
}

/**
 * e_mail_part_new:
 * @part: (allow-none) a #CamelMimePart or %NULL
 * @id: part ID
 * @size: Size of the EMailPart subclass
 *
 * Allocates a @size bytes representing an #EMailPart subclass.
 *
 * Return value: a new #EMailPart-based object
 */
EMailPart *
e_mail_part_subclass_new (CamelMimePart *part,
                          const gchar *id,
                          gsize size,
                          GFreeFunc free_func)
{
	EMailPart *mail_part;

	g_return_val_if_fail (size >= sizeof (EMailPart), NULL);

	mail_part = g_malloc0 (size);
	mail_part->priv = g_new0 (EMailPartPrivate, 1);

	mail_part->priv->ref_cnt = 1;
	mail_part->priv->free_func = free_func;
	mail_part->priv->instance_size = size;

	if (part) {
		mail_part->part = g_object_ref (part);
	}

	if (id) {
		mail_part->id = g_strdup (id);
	}

	return mail_part;
}

EMailPart *
e_mail_part_ref (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, NULL);
	g_return_val_if_fail (part->priv != NULL, NULL);

	g_atomic_int_inc (&part->priv->ref_cnt);

	return part;
}

void
e_mail_part_unref (EMailPart *part)
{
	g_return_if_fail (part != NULL);
	g_return_if_fail (part->priv != NULL);

	if (g_atomic_int_dec_and_test (&part->priv->ref_cnt)) {
		mail_part_free (part);
	}
}

gsize
e_mail_part_get_instance_size (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, 0);

	return part->priv->instance_size;
}

const gchar *
e_mail_part_get_id (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, NULL);

	return part->id;
}

const gchar *
e_mail_part_get_cid (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, NULL);

	return part->cid;
}

void
e_mail_part_set_cid (EMailPart *part,
                     const gchar *cid)
{
	g_return_if_fail (part != NULL);

	g_free (part->cid);
	part->cid = g_strdup (cid);
}

gboolean
e_mail_part_id_has_prefix (EMailPart *part,
                           const gchar *prefix)
{
	g_return_val_if_fail (part != NULL, FALSE);
	g_return_val_if_fail (prefix != NULL, FALSE);

	return g_str_has_prefix (part->id, prefix);
}

gboolean
e_mail_part_id_has_suffix (EMailPart *part,
                           const gchar *suffix)
{
	g_return_val_if_fail (part != NULL, FALSE);
	g_return_val_if_fail (suffix != NULL, FALSE);

	return g_str_has_suffix (part->id, suffix);
}

gboolean
e_mail_part_id_has_substr (EMailPart *part,
                           const gchar *substr)
{
	g_return_val_if_fail (part != NULL, FALSE);
	g_return_val_if_fail (substr != NULL, FALSE);

	return (strstr (part->id, substr) != NULL);
}

CamelMimePart *
e_mail_part_ref_mime_part (EMailPart *part)
{
	CamelMimePart *mime_part = NULL;

	g_return_val_if_fail (part != NULL, NULL);

	if (part->part != NULL)
		mime_part = g_object_ref (part->part);

	return mime_part;
}

const gchar *
e_mail_part_get_mime_type (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, NULL);

	return part->mime_type;
}

void
e_mail_part_set_mime_type (EMailPart *part,
                           const gchar *mime_type)
{
	g_return_if_fail (part != NULL);

	if (g_strcmp0 (mime_type, part->mime_type) == 0)
		return;

	g_free (part->mime_type);
	part->mime_type = g_strdup (mime_type);
}

gboolean
e_mail_part_get_is_attachment (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, FALSE);

	return part->is_attachment;
}

void
e_mail_part_set_is_attachment (EMailPart *part,
                               gboolean is_attachment)
{
	g_return_if_fail (part != NULL);

	part->is_attachment = is_attachment;
}

static EMailPartValidityPair *
mail_part_find_validity_pair (EMailPart *part,
                              guint32 validity_type)
{
	GList *head, *link;

	head = g_queue_peek_head_link (&part->validities);

	for (link = head; link != NULL; link = g_list_next (link)) {
		EMailPartValidityPair *pair = link->data;

		if (pair == NULL)
			continue;

		if ((pair->validity_type & validity_type) == validity_type)
			return pair;
	}

	return NULL;
}

/**
 * e_mail_part_update_validity:
 * @part: An #EMailPart
 * @validity_type: E_MAIL_PART_VALIDITY_* flags
 * @validity: a #CamelCipherValidity
 *
 * Updates validity of the @part. When the part already has some validity
 * set, the new @validity and @validity_type are just appended, preserving
 * the original validity. Validities of the same type (PGP or S/MIME) are
 * merged together.
 */
void
e_mail_part_update_validity (EMailPart *part,
                             CamelCipherValidity *validity,
                             guint32 validity_type)
{
	EMailPartValidityPair *pair;

	g_return_if_fail (part != NULL);

	pair = mail_part_find_validity_pair (part, validity_type & (E_MAIL_PART_VALIDITY_PGP | E_MAIL_PART_VALIDITY_SMIME));
	if (pair != NULL) {
		pair->validity_type |= validity_type;
		camel_cipher_validity_envelope (pair->validity, validity);
	} else {
		pair = g_new0 (EMailPartValidityPair, 1);
		pair->validity_type = validity_type;
		pair->validity = camel_cipher_validity_clone (validity);

		g_queue_push_tail (&part->validities, pair);
	}
}

/**
 * e_mail_part_get_validity:
 * @part: An #EMailPart
 * @validity_type: E_MAIL_PART_VALIDITY_* flags
 *
 * Returns, validity of @part contains any validity with the same bits
 * as @validity_type set. It should contain all bits of it.
 *
 * Returns: a #CamelCipherValidity of the given type, %NULL if not found
 *
 * Since: 3.8
 */
CamelCipherValidity *
e_mail_part_get_validity (EMailPart *part,
                          guint32 validity_type)
{
	EMailPartValidityPair *pair;

	g_return_val_if_fail (part != NULL, NULL);

	pair = mail_part_find_validity_pair (part, validity_type);

	return pair ? pair->validity : NULL;
}

gboolean
e_mail_part_has_validity (EMailPart *part)
{
	g_return_val_if_fail (part != NULL, FALSE);

	return !g_queue_is_empty (&part->validities);
}

EMailPartValidityFlags
e_mail_part_get_validity_flags (EMailPart *part)
{
	EMailPartValidityFlags flags = 0;
	GList *head, *link;

	g_return_val_if_fail (part != NULL, 0);

	head = g_queue_peek_head_link (&part->validities);

	for (link = head; link != NULL; link = g_list_next (link)) {
		EMailPartValidityPair *pair = link->data;

		if (pair != NULL)
			flags |= pair->validity_type;
	}

	return flags;
}

