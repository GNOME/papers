/* pps-undo-handler.h
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
#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_UNDO_HANDLER (pps_undo_handler_get_type ())

G_DECLARE_INTERFACE (PpsUndoHandler, pps_undo_handler, PPS, UNDO_HANDLER, GObject)

struct _PpsUndoHandlerInterface {
	GTypeInterface parent_iface;

	/* undoes @data */
	void (*undo) (PpsUndoHandler *self, gpointer data);

	/* frees @data */
	void (*free_action) (gpointer data);
};

PPS_PUBLIC
void
pps_undo_handler_undo (PpsUndoHandler *self, gpointer data);

PPS_PUBLIC
void
pps_undo_handler_free_action (PpsUndoHandler *self, gpointer data);

G_END_DECLS
