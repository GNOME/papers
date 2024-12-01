/* pps-find-sidebar.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Markus Göllnitz  <camelcasenick@bewares.it>
 * Copyright (C) 2013 Carlos Garcia Campos  <carlosgc@gnome.org>
 * Copyright (C) 2008 Sergey Pushkin  <pushkinsv@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pps-find-sidebar.h"
#include "pps-utils.h"
#include <pps-document.h>
#include <string.h>

typedef struct
{
	PpsSearchContext *context;

	GtkStack *results_stack;
	GtkWidget *search_box;
	GtkWidget *list_view;
	GtkWidget *spinner;
	GtkSingleSelection *selection;
} PpsFindSidebarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsFindSidebar, pps_find_sidebar, ADW_TYPE_BIN)

#define GET_PRIVATE(o) pps_find_sidebar_get_instance_private (o)

static void
selection_changed_cb (GtkSelectionModel *selection,
                      guint position,
                      guint n_items,
                      PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);
	PpsSearchResult *selected_result = PPS_SEARCH_RESULT (gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection)));

	g_return_if_fail (priv->context != NULL);
	g_return_if_fail (selected_result != NULL);

	pps_search_context_select_result (priv->context, selected_result);
}

static void
factory_setup_cb (GtkSignalListItemFactory *self,
                  GObject *object,
                  PpsFindSidebar *sidebar)
{
	GtkListItem *item = GTK_LIST_ITEM (object);
	GtkWidget *widget;

	widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	gtk_list_item_set_child (item, widget);
}

static void
factory_bind_cb (GtkSignalListItemFactory *self,
                 GObject *object,
                 PpsFindSidebar *sidebar)
{
	GtkListItem *item = GTK_LIST_ITEM (object);
	GtkWidget *widget = gtk_list_item_get_child (item);
	PpsSearchResult *result = PPS_SEARCH_RESULT (gtk_list_item_get_item (item));
	GtkLabel *result_label;
	GtkLabel *page_label;

	result_label = GTK_LABEL (gtk_label_new (pps_search_result_get_markup (result)));
	gtk_label_set_use_markup (result_label, TRUE);
	gtk_label_set_ellipsize (result_label, PANGO_ELLIPSIZE_END);
	gtk_widget_set_hexpand (GTK_WIDGET (result_label), TRUE);
	gtk_widget_set_halign (GTK_WIDGET (result_label), GTK_ALIGN_START);

	page_label = GTK_LABEL (gtk_label_new (pps_search_result_get_label (result)));

	gtk_box_append (GTK_BOX (widget), GTK_WIDGET (result_label));
	gtk_box_append (GTK_BOX (widget), GTK_WIDGET (page_label));
}

static void
factory_unbind_cb (GtkSignalListItemFactory *self,
                   GObject *object,
                   PpsFindSidebar *sidebar)
{
	GtkListItem *item = GTK_LIST_ITEM (object);
	GtkWidget *widget = gtk_list_item_get_child (item);

	while (gtk_widget_get_first_child (widget))
		gtk_box_remove (GTK_BOX (widget), gtk_widget_get_first_child (widget));
}

static gboolean
pps_find_sidebar_grab_focus (GtkWidget *widget)
{
	PpsFindSidebar *sidebar = PPS_FIND_SIDEBAR (widget);
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	return gtk_widget_grab_focus (GTK_WIDGET (priv->search_box));
}

static void
pps_find_sidebar_dispose (GObject *object)
{
	PpsFindSidebar *sidebar = PPS_FIND_SIDEBAR (object);
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	g_clear_object (&priv->context);

	G_OBJECT_CLASS (pps_find_sidebar_parent_class)->dispose (object);
}

static void
pps_find_sidebar_class_init (PpsFindSidebarClass *find_sidebar_class)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (find_sidebar_class);

	widget_class->grab_focus = pps_find_sidebar_grab_focus;
	G_OBJECT_CLASS (find_sidebar_class)->dispose = pps_find_sidebar_dispose;

	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/papers/ui/find-sidebar.ui");

	gtk_widget_class_bind_template_child_private (widget_class,
	                                              PpsFindSidebar, results_stack);

	gtk_widget_class_bind_template_child_private (widget_class,
	                                              PpsFindSidebar, search_box);

	gtk_widget_class_bind_template_child_private (widget_class,
	                                              PpsFindSidebar, list_view);

	gtk_widget_class_bind_template_child_private (widget_class,
	                                              PpsFindSidebar, spinner);

	gtk_widget_class_bind_template_child_private (widget_class,
	                                              PpsFindSidebar, selection);

	gtk_widget_class_bind_template_callback (widget_class, factory_setup_cb);
	gtk_widget_class_bind_template_callback (widget_class, factory_bind_cb);
	gtk_widget_class_bind_template_callback (widget_class, factory_unbind_cb);

	gtk_widget_class_bind_template_callback (widget_class, selection_changed_cb);
}

static void
pps_find_sidebar_init (PpsFindSidebar *sidebar)
{
	gtk_widget_init_template (GTK_WIDGET (sidebar));
}

GtkWidget *
pps_find_sidebar_new (void)
{
	return g_object_new (PPS_TYPE_FIND_SIDEBAR,
	                     "orientation", GTK_ORIENTATION_VERTICAL,
	                     NULL);
}

static void
pps_find_sidebar_highlight_first_match_of_page (PpsFindSidebar *sidebar,
                                                gint page)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);
	GtkListView *list_view = GTK_LIST_VIEW (priv->list_view);
	GListModel *result_model = pps_search_context_get_result_model (priv->context);
	PpsSearchResult *result;
	guint current_page, current_index_in_page;
	guint lower_bound = 0;
	guint index = 0;
	guint upper_bound = g_list_model_get_n_items (result_model) - 1;

	/*
	 * Binary search is a fast algorithm, here. However, in the future
	 * the search context should deal with this, and ideally handle this
	 * with a map. Just, that would need to a few more changes on its end.
	 */
	while (lower_bound <= upper_bound) {
		index = (lower_bound + upper_bound) / 2;

		result = g_list_model_get_item (result_model, index);
		current_page = pps_search_result_get_page (result);
		current_index_in_page = pps_search_result_get_index (result);

		if (current_page == page && current_index_in_page == 0) {
			break;
		} else if (current_page < page) {
			lower_bound = index + 1;
		} else {
			upper_bound = index - 1;
		}
	}

	gtk_list_view_scroll_to (list_view, index, GTK_LIST_SCROLL_SELECT, NULL);
}

static void
find_job_finished_cb (PpsSearchContext *search_context,
                      PpsJobFind *job,
                      gint first_match_page,
                      PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	gtk_widget_set_visible (priv->spinner, FALSE);

	if (g_list_model_get_n_items (pps_search_context_get_result_model (search_context)) == 0) {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->results_stack), "no-results");
	}

	if (first_match_page != -1)
		pps_find_sidebar_highlight_first_match_of_page (sidebar, first_match_page);
}

static void
pps_find_sidebar_start (PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	gtk_stack_set_visible_child_name (GTK_STACK (priv->results_stack), "results");
	gtk_widget_set_visible (priv->spinner, TRUE);
}

void
pps_find_sidebar_restart (PpsFindSidebar *sidebar,
                          gint page)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);
	gint first_match_page = -1;
	GListModel *result_model = pps_search_context_get_result_model (priv->context);

	for (guint i = 0; i < g_list_model_get_n_items (result_model); i++) {
		PpsSearchResult *result = g_list_model_get_item (result_model, i);

		if (pps_search_result_get_page (result) >= page) {
			first_match_page = pps_search_result_get_page (result);
			break;
		}
	}

	if (first_match_page != -1)
		pps_find_sidebar_highlight_first_match_of_page (sidebar, first_match_page);
}

void
pps_find_sidebar_clear (PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	gtk_widget_set_visible (priv->spinner, FALSE);
	gtk_stack_set_visible_child_name (GTK_STACK (priv->results_stack), "initial");
}

void
pps_find_sidebar_previous (PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);
	GtkListView *list_view = GTK_LIST_VIEW (priv->list_view);
	guint pos;

	pos = gtk_single_selection_get_selected (priv->selection) - 1;
	gtk_list_view_scroll_to (list_view, pos, GTK_LIST_SCROLL_SELECT, NULL);
}

void
pps_find_sidebar_next (PpsFindSidebar *sidebar)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);
	GtkListView *list_view = GTK_LIST_VIEW (priv->list_view);
	guint pos;

	pos = gtk_single_selection_get_selected (priv->selection) + 1;
	gtk_list_view_scroll_to (list_view, pos, GTK_LIST_SCROLL_SELECT, NULL);
}

void
pps_find_sidebar_set_search_context (PpsFindSidebar *sidebar,
                                     PpsSearchContext *context)
{
	PpsFindSidebarPrivate *priv = GET_PRIVATE (sidebar);

	g_return_if_fail (PPS_IS_SEARCH_CONTEXT (context));

	if (priv->context != NULL) {
		g_signal_handlers_disconnect_by_func (priv->context, pps_find_sidebar_start, sidebar);
		g_signal_handlers_disconnect_by_func (priv->context, pps_find_sidebar_clear, sidebar);
	}

	g_set_object (&priv->context, context);

	gtk_single_selection_set_model (priv->selection, pps_search_context_get_result_model (priv->context));
	g_signal_connect_object (priv->context, "started",
	                         G_CALLBACK (pps_find_sidebar_start),
	                         sidebar, G_CONNECT_SWAPPED);
	g_signal_connect_object (priv->context, "cleared",
	                         G_CALLBACK (pps_find_sidebar_clear),
	                         sidebar, G_CONNECT_SWAPPED);
	g_signal_connect_object (priv->context, "finished",
	                         G_CALLBACK (find_job_finished_cb),
	                         sidebar, G_CONNECT_DEFAULT);

	g_object_set (priv->search_box, "context", priv->context, NULL);
}
