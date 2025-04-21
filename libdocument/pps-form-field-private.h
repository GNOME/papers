// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2020 Germán Poo-Caamaño <gpoo@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

// #include <glib-object.h>

#include "pps-form-field.h"
#include "pps-macros.h"

G_BEGIN_DECLS

/* PpsFormField base class */
PPS_PRIVATE
gchar *pps_form_field_get_alternate_name (PpsFormField *field);
PPS_PRIVATE
void pps_form_field_set_alternate_name (PpsFormField *field,
                                        gchar *alternative_text);

G_END_DECLS
