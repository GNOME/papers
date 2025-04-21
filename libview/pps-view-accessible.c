// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>

#include "pps-page-accessible.h"
#include "pps-page-cache.h"
#include "pps-selection.h"
#include "pps-view-accessible.h"
#include "pps-view-private.h"

static void pps_view_accessible_action_iface_init (AtkActionIface *iface);
static void pps_view_accessible_document_iface_init (AtkDocumentIface *iface);

enum {
	ACTION_SCROLL_UP,
	ACTION_SCROLL_DOWN,
	LAST_ACTION
};

static const gchar *const pps_view_accessible_action_names[] = {
	N_ ("Scroll Up"),
	N_ ("Scroll Down"),
	NULL
};

static const gchar *const pps_view_accessible_action_descriptions[] = {
	N_ ("Scroll View Up"),
	N_ ("Scroll View Down"),
	NULL
};

struct _PpsViewAccessiblePrivate {
	PpsDocumentModel *model;

	/* AtkAction */
	gchar *action_descriptions[LAST_ACTION];
	guint action_idle_handler;
	GtkScrollType idle_scroll;

	gint previous_cursor_page;
	gint start_page;
	gint end_page;
	AtkObject *focused_element;

	GPtrArray *children;
};

G_DEFINE_TYPE_WITH_CODE (PpsViewAccessible, pps_view_accessible, GTK_TYPE_CONTAINER_ACCESSIBLE, G_ADD_PRIVATE (PpsViewAccessible) G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, pps_view_accessible_action_iface_init) G_IMPLEMENT_INTERFACE (ATK_TYPE_DOCUMENT, pps_view_accessible_document_iface_init))

static gint
get_relevant_page (PpsView *view)
{
	return pps_view_is_caret_navigation_enabled (view) ? view->cursor_page : view->current_page;
}

static void
clear_children (PpsViewAccessible *self)
{
	gint i;
	AtkObject *child;

	if (self->priv->children == NULL)
		return;

	for (i = 0; i < self->priv->children->len; i++) {
		child = g_ptr_array_index (self->priv->children, i);
		atk_object_notify_state_change (child, ATK_STATE_DEFUNCT, TRUE);
	}

	g_clear_pointer (&self->priv->children, g_ptr_array_unref);
}

static void
pps_view_accessible_finalize (GObject *object)
{
	PpsViewAccessiblePrivate *priv = PPS_VIEW_ACCESSIBLE (object)->priv;
	int i;

	if (priv->model) {
		g_signal_handlers_disconnect_by_data (priv->model, object);
		g_clear_object (&priv->model);
	}

	g_clear_handle_id (&priv->action_idle_handler, g_source_remove);

	for (i = 0; i < LAST_ACTION; i++)
		g_free (priv->action_descriptions[i]);

	clear_children (PPS_VIEW_ACCESSIBLE (object));

	G_OBJECT_CLASS (pps_view_accessible_parent_class)->finalize (object);
}

static void
pps_view_accessible_initialize (AtkObject *obj,
                                gpointer data)
{
	PpsViewAccessiblePrivate *priv;

	if (ATK_OBJECT_CLASS (pps_view_accessible_parent_class)->initialize != NULL)
		ATK_OBJECT_CLASS (pps_view_accessible_parent_class)->initialize (obj, data);

	gtk_accessible_set_widget (GTK_ACCESSIBLE (obj), GTK_WIDGET (data));

	atk_object_set_name (obj, _ ("Document View"));
	atk_object_set_role (obj, ATK_ROLE_DOCUMENT_FRAME);

	priv = PPS_VIEW_ACCESSIBLE (obj)->priv;
	priv->previous_cursor_page = -1;
	priv->start_page = 0;
	priv->end_page = -1;
}

gint
pps_view_accessible_get_n_pages (PpsViewAccessible *self)
{
	return self->priv->children == NULL ? 0 : self->priv->children->len;
}

static AtkObject *
pps_view_accessible_ref_child (AtkObject *obj,
                               gint i)
{
	PpsViewAccessible *self;
	PpsView *view;

	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (obj), NULL);
	self = PPS_VIEW_ACCESSIBLE (obj);
	g_return_val_if_fail (i >= 0 || i < pps_view_accessible_get_n_pages (self), NULL);

	view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (obj)));
	if (view == NULL)
		return NULL;

	/* If a given page is requested, we assume that the text would
	 * be requested soon, so we need to be sure that is cached.*/
	if (view->page_cache)
		pps_page_cache_ensure_page (view->page_cache, i);

	return g_object_ref (g_ptr_array_index (self->priv->children, i));
}

static gint
pps_view_accessible_get_n_children (AtkObject *obj)
{
	return pps_view_accessible_get_n_pages (PPS_VIEW_ACCESSIBLE (obj));
}

static void
pps_view_accessible_class_init (PpsViewAccessibleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	object_class->finalize = pps_view_accessible_finalize;
	atk_class->initialize = pps_view_accessible_initialize;
	atk_class->get_n_children = pps_view_accessible_get_n_children;
	atk_class->ref_child = pps_view_accessible_ref_child;
}

static void
pps_view_accessible_init (PpsViewAccessible *accessible)
{
	accessible->priv = pps_view_accessible_get_instance_private (accessible);
}

#if ATK_CHECK_VERSION(2, 11, 3)
static gint
pps_view_accessible_get_page_count (AtkDocument *atk_document)
{
	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (atk_document), -1);

	return pps_view_accessible_get_n_pages (PPS_VIEW_ACCESSIBLE (atk_document));
}

static gint
pps_view_accessible_get_current_page_number (AtkDocument *atk_document)
{
	GtkWidget *widget;

	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (atk_document), -1);

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (atk_document));
	if (widget == NULL)
		return -1;

	/* +1 as user starts to count on 1, but papers starts on 0 */
	return get_relevant_page (PPS_VIEW (widget)) + 1;
}
#endif

static void
pps_view_accessible_document_iface_init (AtkDocumentIface *iface)
{
#if ATK_CHECK_VERSION(2, 11, 3)
	iface->get_current_page_number = pps_view_accessible_get_current_page_number;
	iface->get_page_count = pps_view_accessible_get_page_count;
#endif
}

static gboolean
pps_view_accessible_idle_do_action (gpointer data)
{
	PpsViewAccessiblePrivate *priv = PPS_VIEW_ACCESSIBLE (data)->priv;
	PpsView *view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (data)));

	g_signal_emit_by_name (view, "scroll", priv->idle_scroll, GTK_ORIENTATION_VERTICAL);
	priv->action_idle_handler = 0;
	return FALSE;
}

static gboolean
pps_view_accessible_action_do_action (AtkAction *action,
                                      gint i)
{
	PpsViewAccessiblePrivate *priv = PPS_VIEW_ACCESSIBLE (action)->priv;

	if (gtk_accessible_get_widget (GTK_ACCESSIBLE (action)) == NULL)
		return FALSE;

	if (priv->action_idle_handler)
		return FALSE;

	switch (i) {
	case ACTION_SCROLL_UP:
		priv->idle_scroll = GTK_SCROLL_PAGE_BACKWARD;
		break;
	case ACTION_SCROLL_DOWN:
		priv->idle_scroll = GTK_SCROLL_PAGE_FORWARD;
		break;
	default:
		return FALSE;
	}
	priv->action_idle_handler = g_idle_add (pps_view_accessible_idle_do_action,
	                                        action);
	return TRUE;
}

static gint
pps_view_accessible_action_get_n_actions (AtkAction *action)
{
	return LAST_ACTION;
}

static const gchar *
pps_view_accessible_action_get_description (AtkAction *action,
                                            gint i)
{
	PpsViewAccessiblePrivate *priv = PPS_VIEW_ACCESSIBLE (action)->priv;

	if (i < 0 || i >= LAST_ACTION)
		return NULL;

	if (priv->action_descriptions[i])
		return priv->action_descriptions[i];
	else
		return pps_view_accessible_action_descriptions[i];
}

static const gchar *
pps_view_accessible_action_get_name (AtkAction *action,
                                     gint i)
{
	if (i < 0 || i >= LAST_ACTION)
		return NULL;

	return pps_view_accessible_action_names[i];
}

static gboolean
pps_view_accessible_action_set_description (AtkAction *action,
                                            gint i,
                                            const gchar *description)
{
	PpsViewAccessiblePrivate *priv = PPS_VIEW_ACCESSIBLE (action)->priv;
	gchar *old_description;

	if (i < 0 || i >= LAST_ACTION)
		return FALSE;

	old_description = priv->action_descriptions[i];
	priv->action_descriptions[i] = g_strdup (description);
	g_free (old_description);

	return TRUE;
}

static void
pps_view_accessible_action_iface_init (AtkActionIface *iface)
{
	iface->do_action = pps_view_accessible_action_do_action;
	iface->get_n_actions = pps_view_accessible_action_get_n_actions;
	iface->get_description = pps_view_accessible_action_get_description;
	iface->get_name = pps_view_accessible_action_get_name;
	iface->set_description = pps_view_accessible_action_set_description;
}

static void
pps_view_accessible_cursor_moved (PpsView *view,
                                  gint page,
                                  gint offset,
                                  PpsViewAccessible *accessible)
{
	PpsViewAccessiblePrivate *priv = accessible->priv;
	PpsPageAccessible *page_accessible = NULL;

	if (priv->previous_cursor_page != page) {
		AtkObject *current_page = NULL;

		if (priv->previous_cursor_page >= 0) {
			AtkObject *previous_page = NULL;
			previous_page = g_ptr_array_index (priv->children,
			                                   priv->previous_cursor_page);
			atk_object_notify_state_change (previous_page, ATK_STATE_FOCUSED, FALSE);
		}

		priv->previous_cursor_page = page;
		current_page = g_ptr_array_index (priv->children, page);
		atk_object_notify_state_change (current_page, ATK_STATE_FOCUSED, TRUE);

#if ATK_CHECK_VERSION(2, 11, 2)
		/* +1 as user start to count on 1, but papers starts on 0 */
		g_signal_emit_by_name (accessible, "page-changed", page + 1);
#endif
	}

	page_accessible = g_ptr_array_index (priv->children, page);
	g_signal_emit_by_name (page_accessible, "text-caret-moved", offset);
}

static void
pps_view_accessible_selection_changed (PpsView *view,
                                       PpsViewAccessible *view_accessible)
{
	AtkObject *page_accessible;

	page_accessible = g_ptr_array_index (view_accessible->priv->children,
	                                     get_relevant_page (view));
	g_signal_emit_by_name (page_accessible, "text-selection-changed");
}

static void
page_changed_cb (PpsDocumentModel *model,
                 gint old_page,
                 gint new_page,
                 PpsViewAccessible *accessible)
{
#if ATK_CHECK_VERSION(2, 11, 2)
	PpsView *view;

	view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible)));
	if (!pps_view_is_caret_navigation_enabled (view))
		g_signal_emit_by_name (accessible, "page-changed", new_page + 1);
#endif
}

static void
initialize_children (PpsViewAccessible *self)
{
	gint i;
	PpsPageAccessible *child;
	gint n_pages;
	PpsDocument *pps_document;

	pps_document = pps_document_model_get_document (self->priv->model);
	n_pages = pps_document_get_n_pages (pps_document);

	self->priv->children = g_ptr_array_new_full (n_pages, (GDestroyNotify) g_object_unref);
	for (i = 0; i < n_pages; i++) {
		child = pps_page_accessible_new (self, i);
		g_ptr_array_add (self->priv->children, child);
	}

	/* When a document is reloaded, it may have less pages.
	 * We need to update the end page accordingly to avoid
	 * invalid access to self->priv->children
	 * See https://bugzilla.gnome.org/show_bug.cgi?id=735744
	 */
	if (self->priv->end_page >= n_pages)
		self->priv->end_page = n_pages - 1;
}

static void
document_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsViewAccessible *accessible)
{
	PpsDocument *document = pps_document_model_get_document (model);

	clear_children (accessible);

	if (document == NULL)
		return;

	initialize_children (accessible);

	/* Inside this callback the document is already loaded. We
	 * don't have here an "just before" and "just after"
	 * signal. We emit both in a row, as usual ATs uses reload to
	 * know that current content has changed, and load-complete to
	 * know that the content is already available.
	 */
	g_signal_emit_by_name (accessible, "reload");
	g_signal_emit_by_name (accessible, "load-complete");
}

void
pps_view_accessible_set_model (PpsViewAccessible *accessible,
                               PpsDocumentModel *model)
{
	PpsViewAccessiblePrivate *priv = accessible->priv;

	if (priv->model == model)
		return;

	if (priv->model) {
		g_signal_handlers_disconnect_by_data (priv->model, accessible);
		g_object_unref (priv->model);
	}

	priv->model = g_object_ref (model);

	document_changed_cb (model, NULL, accessible);
	g_signal_connect (priv->model, "page-changed",
	                  G_CALLBACK (page_changed_cb),
	                  accessible);
	g_signal_connect (priv->model, "notify::document",
	                  G_CALLBACK (document_changed_cb),
	                  accessible);
}

static gboolean
pps_view_accessible_focus_changed (GtkWidget *widget,
                                   GdkEventFocus *event,
                                   PpsViewAccessible *self)
{
	AtkObject *page_accessible;

	g_return_val_if_fail (PPS_IS_VIEW (widget), FALSE);
	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (self), FALSE);

	if (self->priv->children == NULL || self->priv->children->len == 0)
		return FALSE;

	page_accessible = g_ptr_array_index (self->priv->children,
	                                     get_relevant_page (PPS_VIEW (widget)));
	atk_object_notify_state_change (page_accessible,
	                                ATK_STATE_FOCUSED, event->in);

	return FALSE;
}

AtkObject *
pps_view_accessible_new (GtkWidget *widget)
{
	AtkObject *accessible;
	PpsView *view;

	g_return_val_if_fail (PPS_IS_VIEW (widget), NULL);

	accessible = g_object_new (PPS_TYPE_VIEW_ACCESSIBLE, NULL);
	atk_object_initialize (accessible, widget);

	g_signal_connect (widget, "cursor-moved",
	                  G_CALLBACK (pps_view_accessible_cursor_moved),
	                  accessible);
	g_signal_connect (widget, "selection-changed",
	                  G_CALLBACK (pps_view_accessible_selection_changed),
	                  accessible);
	g_signal_connect (widget, "focus-in-event",
	                  G_CALLBACK (pps_view_accessible_focus_changed),
	                  accessible);
	g_signal_connect (widget, "focus-out-event",
	                  G_CALLBACK (pps_view_accessible_focus_changed),
	                  accessible);

	view = PPS_VIEW (widget);
	if (view->model)
		pps_view_accessible_set_model (PPS_VIEW_ACCESSIBLE (accessible),
		                               view->model);

	return accessible;
}

gint
pps_view_accessible_get_relevant_page (PpsViewAccessible *accessible)
{
	PpsView *view;

	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (accessible), -1);

	view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible)));

	return get_relevant_page (view);
}

void
_transform_doc_rect_to_atk_rect (PpsViewAccessible *accessible,
                                 gint page,
                                 PpsRectangle *doc_rect,
                                 PpsRectangle *atk_rect,
                                 AtkCoordType coord_type)
{
	PpsView *view;
	GdkRectangle view_rect;
	GtkWidget *widget, *toplevel;
	gint x_widget, y_widget;

	view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible)));
	_pps_view_transform_doc_rect_to_view_rect (view, page, doc_rect, &view_rect);
	view_rect.x -= view->scroll_x;
	view_rect.y -= view->scroll_y;

	widget = GTK_WIDGET (view);
	toplevel = gtk_widget_get_toplevel (widget);
	gtk_widget_translate_coordinates (widget, toplevel, 0, 0, &x_widget, &y_widget);
	view_rect.x += x_widget;
	view_rect.y += y_widget;

	if (coord_type == ATK_XY_SCREEN) {
		gint x_window, y_window;
		gdk_window_get_origin (gtk_widget_get_window (toplevel), &x_window, &y_window);
		view_rect.x += x_window;
		view_rect.y += y_window;
	}

	atk_rect->x1 = view_rect.x;
	atk_rect->y1 = view_rect.y;
	atk_rect->x2 = view_rect.x + view_rect.width;
	atk_rect->y2 = view_rect.y + view_rect.height;
}

gboolean
pps_view_accessible_is_doc_rect_showing (PpsViewAccessible *accessible,
                                         gint page,
                                         PpsRectangle *doc_rect)
{
	PpsView *view;
	GdkRectangle view_rect;
	GtkAllocation allocation;
	gint x, y;
	gboolean hidden;

	view = PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible)));
	if (page < view->start_page || page > view->end_page)
		return FALSE;

	gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
	x = gtk_adjustment_get_value (view->hadjustment);
	y = gtk_adjustment_get_value (view->vadjustment);

	_pps_view_transform_doc_rect_to_view_rect (view, page, doc_rect, &view_rect);
	hidden = view_rect.x + view_rect.width < x || view_rect.x > x + allocation.width ||
	         view_rect.y + view_rect.height < y || view_rect.y > y + allocation.height;

	return !hidden;
}

void
pps_view_accessible_set_page_range (PpsViewAccessible *accessible,
                                    gint start,
                                    gint end)
{
	gint i;
	AtkObject *page;

	g_return_if_fail (PPS_IS_VIEW_ACCESSIBLE (accessible));

	for (i = accessible->priv->start_page; i <= accessible->priv->end_page; i++) {
		if (i < start || i > end) {
			page = g_ptr_array_index (accessible->priv->children, i);
			atk_object_notify_state_change (page, ATK_STATE_SHOWING, FALSE);
		}
	}

	for (i = start; i <= end; i++) {
		if (i < accessible->priv->start_page || i > accessible->priv->end_page) {
			page = g_ptr_array_index (accessible->priv->children, i);
			atk_object_notify_state_change (page, ATK_STATE_SHOWING, TRUE);
		}
	}

	accessible->priv->start_page = start;
	accessible->priv->end_page = end;
}

void
pps_view_accessible_set_focused_element (PpsViewAccessible *accessible,
                                         PpsMapping *new_focus,
                                         gint new_focus_page)
{
	PpsPageAccessible *page;

	if (accessible->priv->focused_element) {
		atk_object_notify_state_change (accessible->priv->focused_element, ATK_STATE_FOCUSED, FALSE);
		accessible->priv->focused_element = NULL;
	}

	if (!new_focus || new_focus_page == -1)
		return;

	page = g_ptr_array_index (accessible->priv->children, new_focus_page);
	accessible->priv->focused_element = pps_page_accessible_get_accessible_for_mapping (page, new_focus);
	if (accessible->priv->focused_element)
		atk_object_notify_state_change (accessible->priv->focused_element, ATK_STATE_FOCUSED, TRUE);
}

void
pps_view_accessible_update_element_state (PpsViewAccessible *accessible,
                                          PpsMapping *element,
                                          gint element_page)
{
	PpsPageAccessible *page;

	page = g_ptr_array_index (accessible->priv->children, element_page);
	pps_page_accessible_update_element_state (page, element);
}
