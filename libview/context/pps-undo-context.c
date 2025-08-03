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

#include "pps-undo-context.h"

/**
 * PpsUndoContext:
 *
 * This context singleton manages two stacks of #PpsUndoAction
 * that contain actions that may be undone or redone.
 *
 * These actions may be added by a number of objects that implement
 * PpsUndoHandler (e.g. the annotations context). Each handler provides a pointer
 * to an opaque structure that contains data to undo the action.
 *
 * The switch between the undo/redo stacks is done transparently: actions are added
 * to the redo stack if and only if another action is being undone (i.e.
 * `pps_undo_context_undo` is being called).
 */

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "PpsUndoCtx"

typedef enum {
	PPS_UNDO_CONTEXT_STATE_NORMAL,
	PPS_UNDO_CONTEXT_STATE_UNDOING,
	PPS_UNDO_CONTEXT_STATE_REDOING
} PpsUndoContextState;

struct _PpsUndoContext {
	GObject parent_instance;
	GQueue *undo_stack;
	GQueue *redo_stack;
	/* This tracks whether the user asked to undo/redo an action
	or is doing some other normal thing. This is used to add new undoable actions
	to the correct stack */
	PpsUndoContextState undo_state;
	PpsDocumentModel *document_model;
};

typedef struct {
	PpsUndoHandler *handler;
	gpointer data;
} PpsUndoAction;

enum {
	PROP_0,
	PROP_DOCUMENT_MODEL,
	NUM_PROPERTIES
};
G_DEFINE_TYPE (PpsUndoContext, pps_undo_context, G_TYPE_OBJECT)

static void
pps_undo_action_free (PpsUndoAction *action)
{
	pps_undo_handler_free_action (action->handler, action->data);
	g_object_unref (action->handler);
	g_free (action);
}

static void
pps_undo_context_document_changed (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	PpsUndoContext *context = PPS_UNDO_CONTEXT (user_data);
	g_queue_clear_full (context->undo_stack, (GDestroyNotify) pps_undo_action_free);
	g_queue_clear_full (context->redo_stack, (GDestroyNotify) pps_undo_action_free);
}

static void
pps_undo_context_finalize (GObject *object)
{
	PpsUndoContext *context = PPS_UNDO_CONTEXT (object);
	g_queue_free_full (context->undo_stack, (GDestroyNotify) pps_undo_action_free);
	g_queue_free_full (context->redo_stack, (GDestroyNotify) pps_undo_action_free);
	g_object_unref (context->document_model);
	G_OBJECT_CLASS (pps_undo_context_parent_class)->finalize (object);
}

static void
pps_undo_context_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PpsUndoContext *context = PPS_UNDO_CONTEXT (object);

	switch (property_id) {
	case PROP_DOCUMENT_MODEL:
		context->document_model = g_value_dup_object (value);
		g_signal_connect_object (context->document_model, "notify::document",
		                         G_CALLBACK (pps_undo_context_document_changed),
		                         context, G_CONNECT_DEFAULT);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
pps_undo_context_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	PpsUndoContext *context = PPS_UNDO_CONTEXT (object);

	switch (property_id) {
	case PROP_DOCUMENT_MODEL:
		g_value_set_object (value, context->document_model);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
pps_undo_context_class_init (PpsUndoContextClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pps_undo_context_finalize;
	object_class->set_property = pps_undo_context_set_property;
	object_class->get_property = pps_undo_context_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_DOCUMENT_MODEL,
	                                 g_param_spec_object ("document-model",
	                                                      "Document Model",
	                                                      "The document model associated with this undo context",
	                                                      PPS_TYPE_DOCUMENT_MODEL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
pps_undo_context_init (PpsUndoContext *context)
{
	context->undo_stack = g_queue_new ();
	context->redo_stack = g_queue_new ();
	context->undo_state = PPS_UNDO_CONTEXT_STATE_NORMAL;
}

PpsUndoContext *
pps_undo_context_new (PpsDocumentModel *document_model)
{
	return g_object_new (PPS_TYPE_UNDO_CONTEXT, "document-model", document_model, NULL);
}

/**
 * pps_undo_context_add_action:
 * @context: a #PpsUndoContext
 * @handler: an object implementing #PpsUndoHandler
 * @data: a pointer to a struct that @handler will use to undo an action
 *
 * This method adds a new action to the undo (or redo if it is used while
 * undoing) stack. If it is undone in the future, `pps_undo_handler_undo`
 * will be called on @handler with @data.
 */
void
pps_undo_context_add_action (PpsUndoContext *context, PpsUndoHandler *handler, gpointer data)
{
	g_return_if_fail (PPS_IS_UNDO_CONTEXT (context));
	g_return_if_fail (PPS_IS_UNDO_HANDLER (handler));

	PpsUndoAction *action = g_new (PpsUndoAction, 1);
	GQueue *queue;

	if (context->undo_state == PPS_UNDO_CONTEXT_STATE_UNDOING) {
		queue = context->redo_stack;
	} else {
		queue = context->undo_stack;
	}
	if (context->undo_state == PPS_UNDO_CONTEXT_STATE_NORMAL) {
		g_queue_clear_full (context->redo_stack, (GDestroyNotify) pps_undo_action_free);
		g_debug ("Clearing the redo stack");
	}
	action->handler = g_object_ref (handler);
	action->data = data;

	g_debug ("Adding action to the undo/redo stack");
	g_queue_push_head (queue, action);
}

/**
 * pps_undo_context_undo:
 * @context: a #PpsUndoContext
 *
 * This pops the last action on the undo stack and undoes it by calling the
 * undo interface method of the `PpsUndoHandler` associated to the action.
 * While undoing, the `PpsUndoHandler` should add a new action with
 * `pps_undo_context_add_action` to redo what has just been undone, it will be
 * added to the redo stack by the undo context.
 */
void
pps_undo_context_undo (PpsUndoContext *context)
{
	g_return_if_fail (PPS_IS_UNDO_CONTEXT (context));

	PpsUndoAction *action = g_queue_pop_head (context->undo_stack);
	if (action) {
		context->undo_state = PPS_UNDO_CONTEXT_STATE_UNDOING;
		pps_undo_handler_undo (action->handler, action->data);
		pps_undo_handler_free_action (action->handler, action->data);
		g_free (action);
		context->undo_state = PPS_UNDO_CONTEXT_STATE_NORMAL;
	} else {
		g_debug ("Undo stack empty");
	}
}

/**
 * pps_undo_context_redo:
 * @context: a #PpsUndoContext
 *
 * Similar to the undo method, but taking an action from the redo stack.
 */
void
pps_undo_context_redo (PpsUndoContext *context)
{
	g_return_if_fail (PPS_IS_UNDO_CONTEXT (context));

	PpsUndoAction *action = g_queue_pop_head (context->redo_stack);
	if (action) {
		context->undo_state = PPS_UNDO_CONTEXT_STATE_REDOING;
		pps_undo_handler_undo (action->handler, action->data);
		pps_undo_handler_free_action (action->handler, action->data);
		g_free (action);
		context->undo_state = PPS_UNDO_CONTEXT_STATE_NORMAL;
	} else {
		g_debug ("Redo stack empty");
	}
}

/* The next two functions may be used to "squash" actions on the last
one registered by the context. This is used by the annotations context
but should be avoided in newly written code, i.e. an action should always
represent a complete user action. */

/**
 * pps_undo_context_get_last_handler:
 * @context: #PpsUndoContext instance
 *
 * Returns: (transfer none): The last handler
 */
PpsUndoHandler *
pps_undo_context_get_last_handler (PpsUndoContext *context)
{
	PpsUndoAction *last_action;

	if (g_queue_is_empty (context->undo_stack))
		return NULL;

	last_action = g_queue_peek_head (context->undo_stack);
	return last_action->handler;
}

gpointer
pps_undo_context_get_last_action (PpsUndoContext *context)
{
	PpsUndoAction *last_action;
	GQueue *queue;
	if (context->undo_state == PPS_UNDO_CONTEXT_STATE_UNDOING) {
		queue = context->redo_stack;
	} else {
		queue = context->undo_stack;
	}

	if (g_queue_is_empty (queue))
		return NULL;

	last_action = g_queue_peek_head (queue);
	return last_action->data;
}
