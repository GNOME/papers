/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2020 Germán Poo-Caamaño <gpoo@gnome.org>
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
