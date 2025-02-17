/* pps-undo-handler.c
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

#include "pps-undo-handler.h"

/**
 * PpsUndoHandler:
 *
 * This interface is implemented by objects (e.g. the annotation context) that
 * aim to register actions that may be undone. Such actions must be registered
 * into the undo context so as they are added to the undo stack with
 * `pps_undo_context_add_action`.
 * Then, when an action must be undone (e.g. the user pressed Ctrl+Z), the
 * `pps_undo_handler_undo` interface method is called on the object.
 * An action is represented by an arbitrary pointer. The object must free
 * such pointers when the `pps_undo_handler_free_action` interface method is called.
 *
 * For instance, the annotation context implements `PpsUndoHandler`. When an
 * annotation is added, the annotation context creates a custom struct that represents
 * this action, it contains (among other things) a pointer to the added `PpsAnnotation`.
 * This action is added to the undo stack of the undo context. Then, if the user uses
 * Ctrl+Z, the `pps_undo_context_undo` method is called, the addition action is taken
 * from the undo stack and `pps_undo_handler_undo` is called on the annotation context
 * and the addition action. Then, the implementation of this interface in the
 * annotation context removes the annotation from the document. This removal entails that the annotation
 * context adds a new action to the undo context (a struct that represents the removal
 * of the annotation), and the undo context adds this action to the redo stack since this
 * happens while undoing.
 *
 */

G_DEFINE_INTERFACE (PpsUndoHandler, pps_undo_handler, G_TYPE_OBJECT)

static void
pps_undo_handler_default_init (PpsUndoHandlerInterface *iface)
{
	iface->undo = NULL;
}

void
pps_undo_handler_undo (PpsUndoHandler *self, gpointer data)
{
	g_return_if_fail (PPS_IS_UNDO_HANDLER (self));
	PPS_UNDO_HANDLER_GET_IFACE (self)->undo (self, data);
}

void
pps_undo_handler_free_action (PpsUndoHandler *self, gpointer data)
{
	g_return_if_fail (PPS_IS_UNDO_HANDLER (self));
	PPS_UNDO_HANDLER_GET_IFACE (self)->free_action (data);
}
