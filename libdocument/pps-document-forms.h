// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-forms.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-document.h"
#include "pps-form-field.h"
#include "pps-macros.h"
#include "pps-mapping-list.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_FORMS (pps_document_forms_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentForms, pps_document_forms, PPS, DOCUMENT_FORMS, GObject)

struct _PpsDocumentFormsInterface {
	GTypeInterface base_iface;

	/* Methods  */
	PpsMappingList *(*get_form_fields) (PpsDocumentForms *document_forms,
	                                    PpsPage *page);
	gboolean (*document_is_modified) (PpsDocumentForms *document_forms);
	gchar *(*form_field_text_get_text) (PpsDocumentForms *document_forms,
	                                    PpsFormField *field);
	void (*form_field_text_set_text) (PpsDocumentForms *document_forms,
	                                  PpsFormField *field,
	                                  const gchar *text);
	gboolean (*form_field_button_get_state) (PpsDocumentForms *document_forms,
	                                         PpsFormField *field);
	void (*form_field_button_set_state) (PpsDocumentForms *document_forms,
	                                     PpsFormField *field,
	                                     gboolean state);
	gchar *(*form_field_choice_get_item) (PpsDocumentForms *document_forms,
	                                      PpsFormField *field,
	                                      gint index);
	gint (*form_field_choice_get_n_items) (PpsDocumentForms *document_forms,
	                                       PpsFormField *field);
	gboolean (*form_field_choice_is_item_selected) (PpsDocumentForms *document_forms,
	                                                PpsFormField *field,
	                                                gint index);
	void (*form_field_choice_select_item) (PpsDocumentForms *document_forms,
	                                       PpsFormField *field,
	                                       gint index);
	void (*form_field_choice_toggle_item) (PpsDocumentForms *document_forms,
	                                       PpsFormField *field,
	                                       gint index);
	void (*form_field_choice_unselect_all) (PpsDocumentForms *document_forms,
	                                        PpsFormField *field);
	void (*form_field_choice_set_text) (PpsDocumentForms *document_forms,
	                                    PpsFormField *field,
	                                    const gchar *text);
	gchar *(*form_field_choice_get_text) (PpsDocumentForms *document_forms,
	                                      PpsFormField *field);
	void (*reset_form) (PpsDocumentForms *document_forms,
	                    PpsLinkAction *action);
};

PPS_PUBLIC
PpsMappingList *pps_document_forms_get_form_fields (PpsDocumentForms *document_forms,
                                                    PpsPage *page);
PPS_PUBLIC
gboolean pps_document_forms_document_is_modified (PpsDocumentForms *document_forms);

PPS_PUBLIC
gchar *pps_document_forms_form_field_text_get_text (PpsDocumentForms *document_forms,
                                                    PpsFormField *field);
PPS_PUBLIC
void pps_document_forms_form_field_text_set_text (PpsDocumentForms *document_forms,
                                                  PpsFormField *field,
                                                  const gchar *text);

PPS_PUBLIC
gboolean pps_document_forms_form_field_button_get_state (PpsDocumentForms *document_forms,
                                                         PpsFormField *field);
PPS_PUBLIC
void pps_document_forms_form_field_button_set_state (PpsDocumentForms *document_forms,
                                                     PpsFormField *field,
                                                     gboolean state);

PPS_PUBLIC
gchar *pps_document_forms_form_field_choice_get_item (PpsDocumentForms *document_forms,
                                                      PpsFormField *field,
                                                      gint index);
PPS_PUBLIC
gint pps_document_forms_form_field_choice_get_n_items (PpsDocumentForms *document_forms,
                                                       PpsFormField *field);
PPS_PUBLIC
gboolean pps_document_forms_form_field_choice_is_item_selected (PpsDocumentForms *document_forms,
                                                                PpsFormField *field,
                                                                gint index);
PPS_PUBLIC
void pps_document_forms_form_field_choice_select_item (PpsDocumentForms *document_forms,
                                                       PpsFormField *field,
                                                       gint index);
PPS_PUBLIC
void pps_document_forms_form_field_choice_toggle_item (PpsDocumentForms *document_forms,
                                                       PpsFormField *field,
                                                       gint index);
PPS_PUBLIC
void pps_document_forms_form_field_choice_unselect_all (PpsDocumentForms *document_forms,
                                                        PpsFormField *field);
PPS_PUBLIC
void pps_document_forms_form_field_choice_set_text (PpsDocumentForms *document_forms,
                                                    PpsFormField *field,
                                                    const gchar *text);
PPS_PUBLIC
gchar *pps_document_forms_form_field_choice_get_text (PpsDocumentForms *document_forms,
                                                      PpsFormField *field);
PPS_PUBLIC
void pps_document_forms_reset_form (PpsDocumentForms *document_forms,
                                    PpsLinkAction *action);

G_END_DECLS
