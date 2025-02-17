/* pps-undo-context.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Lucas Baudin
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

#include "pps-document-model.h"
#include "pps-undo-handler.h"
#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_UNDO_CONTEXT (pps_undo_context_get_type ())
G_DECLARE_FINAL_TYPE (PpsUndoContext, pps_undo_context, PPS, UNDO_CONTEXT, GObject)

PPS_PUBLIC
PpsUndoContext *pps_undo_context_new (PpsDocumentModel *document_model);

PPS_PUBLIC
void pps_undo_context_add_action (PpsUndoContext *context, PpsUndoHandler *handler, gpointer data);

PPS_PUBLIC
void pps_undo_context_undo (PpsUndoContext *context);

PPS_PUBLIC
void pps_undo_context_redo (PpsUndoContext *context);

PPS_PUBLIC
PpsUndoHandler *
pps_undo_context_get_last_handler (PpsUndoContext *context);

PPS_PUBLIC
gpointer
pps_undo_context_get_last_action (PpsUndoContext *context);

G_END_DECLS
