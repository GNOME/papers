// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2020 Germán Poo-Caamaño <gpoo@gnome.org>
 *  Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2006 Julien Rebetez
 */

#include "pps-form-field.h"
#include "pps-form-field-private.h"
#include <config.h>

typedef struct
{
	gchar *alt_ui_name;
} PpsFormFieldPrivate;

static void pps_form_field_init (PpsFormField *field);
static void pps_form_field_class_init (PpsFormFieldClass *klass);
static void pps_form_field_text_init (PpsFormFieldText *field_text);
static void pps_form_field_text_class_init (PpsFormFieldTextClass *klass);
static void pps_form_field_button_init (PpsFormFieldButton *field_button);
static void pps_form_field_button_class_init (PpsFormFieldButtonClass *klass);
static void pps_form_field_choice_init (PpsFormFieldChoice *field_choice);
static void pps_form_field_choice_class_init (PpsFormFieldChoiceClass *klass);
static void pps_form_field_signature_init (PpsFormFieldSignature *field_choice);
static void pps_form_field_signature_class_init (PpsFormFieldSignatureClass *klass);

G_DEFINE_TYPE (PpsFormFieldText, pps_form_field_text, PPS_TYPE_FORM_FIELD)
G_DEFINE_TYPE (PpsFormFieldButton, pps_form_field_button, PPS_TYPE_FORM_FIELD)
G_DEFINE_TYPE (PpsFormFieldChoice, pps_form_field_choice, PPS_TYPE_FORM_FIELD)
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PpsFormField, pps_form_field, G_TYPE_OBJECT)
G_DEFINE_TYPE (PpsFormFieldSignature, pps_form_field_signature, PPS_TYPE_FORM_FIELD)

#define GET_FIELD_PRIVATE(o) pps_form_field_get_instance_private (o)
static void
pps_form_field_init (PpsFormField *field)
{
	PpsFormFieldPrivate *priv = GET_FIELD_PRIVATE (field);

	field->page = NULL;
	field->changed = FALSE;
	field->is_read_only = FALSE;
	priv->alt_ui_name = NULL;
}

static void
pps_form_field_finalize (GObject *object)
{
	PpsFormField *field = PPS_FORM_FIELD (object);
	PpsFormFieldPrivate *priv = GET_FIELD_PRIVATE (field);

	g_clear_object (&field->page);
	g_clear_object (&field->activation_link);
	g_clear_pointer (&priv->alt_ui_name, g_free);

	(*G_OBJECT_CLASS (pps_form_field_parent_class)->finalize) (object);
}

static void
pps_form_field_class_init (PpsFormFieldClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pps_form_field_finalize;
}

/**
 * pps_form_field_get_alternate_name
 * @field: a #PpsFormField
 *
 * Gets the alternate ui name of @field. This name is also commonly
 * used by pdf producers/readers to show it as a tooltip when @field area
 * is hovered by a pointing device (eg. mouse).
 *
 * Returns: (transfer full): a string.
 */
gchar *
pps_form_field_get_alternate_name (PpsFormField *field)
{
	PpsFormFieldPrivate *priv;

	g_return_val_if_fail (PPS_IS_FORM_FIELD (field), NULL);

	priv = GET_FIELD_PRIVATE (field);

	return priv->alt_ui_name;
}

/**
 * pps_form_field_set_alternate_name
 * @field: a #PpsFormField
 * @alternative_text: a string with the alternative name of a form field
 *
 * Sets the alternate ui name of @field. This name is also commonly
 * used by pdf producers/readers to show it as a tooltip when @field area
 * is hovered by a pointing device (eg. mouse).
 */
void
pps_form_field_set_alternate_name (PpsFormField *field,
                                   gchar *alternative_text)
{
	PpsFormFieldPrivate *priv;

	g_return_if_fail (PPS_IS_FORM_FIELD (field));

	priv = GET_FIELD_PRIVATE (field);

	if (priv->alt_ui_name)
		g_clear_pointer (&priv->alt_ui_name, g_free);

	priv->alt_ui_name = alternative_text;
}

static void
pps_form_field_text_finalize (GObject *object)
{
	PpsFormFieldText *field_text = PPS_FORM_FIELD_TEXT (object);

	g_clear_pointer (&field_text->text, g_free);

	(*G_OBJECT_CLASS (pps_form_field_text_parent_class)->finalize) (object);
}

static void
pps_form_field_text_init (PpsFormFieldText *field_text)
{
}

static void
pps_form_field_text_class_init (PpsFormFieldTextClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pps_form_field_text_finalize;
}

static void
pps_form_field_button_init (PpsFormFieldButton *field_button)
{
}

static void
pps_form_field_button_class_init (PpsFormFieldButtonClass *klass)
{
}

static void
pps_form_field_choice_finalize (GObject *object)
{
	PpsFormFieldChoice *field_choice = PPS_FORM_FIELD_CHOICE (object);

	g_clear_pointer (&field_choice->selected_items, g_list_free);
	g_clear_pointer (&field_choice->text, g_free);

	(*G_OBJECT_CLASS (pps_form_field_choice_parent_class)->finalize) (object);
}

static void
pps_form_field_choice_init (PpsFormFieldChoice *field_choice)
{
}

static void
pps_form_field_choice_class_init (PpsFormFieldChoiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pps_form_field_choice_finalize;
}

static void
pps_form_field_signature_init (PpsFormFieldSignature *field_signature)
{
}

static void
pps_form_field_signature_class_init (PpsFormFieldSignatureClass *klass)
{
}

PpsFormField *
pps_form_field_text_new (gint id,
                         PpsFormFieldTextType type)
{
	PpsFormField *field;

	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= PPS_FORM_FIELD_TEXT_NORMAL &&
	                          type <= PPS_FORM_FIELD_TEXT_FILE_SELECT,
	                      NULL);

	field = PPS_FORM_FIELD (g_object_new (PPS_TYPE_FORM_FIELD_TEXT, NULL));
	field->id = id;
	PPS_FORM_FIELD_TEXT (field)->type = type;

	return field;
}

PpsFormField *
pps_form_field_button_new (gint id,
                           PpsFormFieldButtonType type)
{
	PpsFormField *field;

	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= PPS_FORM_FIELD_BUTTON_PUSH &&
	                          type <= PPS_FORM_FIELD_BUTTON_RADIO,
	                      NULL);

	field = PPS_FORM_FIELD (g_object_new (PPS_TYPE_FORM_FIELD_BUTTON, NULL));
	field->id = id;
	PPS_FORM_FIELD_BUTTON (field)->type = type;

	return field;
}

PpsFormField *
pps_form_field_choice_new (gint id,
                           PpsFormFieldChoiceType type)
{
	PpsFormField *field;

	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= PPS_FORM_FIELD_CHOICE_COMBO &&
	                          type <= PPS_FORM_FIELD_CHOICE_LIST,
	                      NULL);

	field = PPS_FORM_FIELD (g_object_new (PPS_TYPE_FORM_FIELD_CHOICE, NULL));
	field->id = id;
	PPS_FORM_FIELD_CHOICE (field)->type = type;

	return field;
}

PpsFormField *
pps_form_field_signature_new (gint id)
{
	PpsFormField *field;

	g_return_val_if_fail (id >= 0, NULL);

	field = PPS_FORM_FIELD (g_object_new (PPS_TYPE_FORM_FIELD_SIGNATURE, NULL));
	field->id = id;

	return field;
}
