// SPDX-FileCopyrightText: 2003, 2004 Marco Pesenti Gritti
// SPDX-FileCopyrightText: 2003, 2004 Christian Persch
// SPDX-FileCopyrightText: 2018       Germán Poo-Caamaño
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "pps-page-selector.h"
#include <papers-document.h>

enum {
	WIDGET_ACTIVATE_LINK,
	WIDGET_N_SIGNALS
};

struct _PpsPageSelector {
	GtkBox parent;

	PpsDocument *document;
	PpsDocumentModel *doc_model;

	GtkWidget *entry;
	GtkWidget *label;
	gulong signal_id;
	gulong notify_document_signal_id;
};

static guint widget_signals[WIDGET_N_SIGNALS] = {
	0,
};

G_DEFINE_TYPE (PpsPageSelector, pps_page_selector, GTK_TYPE_BOX)

static gboolean
show_page_number_in_pages_label (PpsPageSelector *page_selector,
                                 gint page)
{
	gchar *page_label;
	gboolean retval;

	if (!pps_document_has_text_page_labels (page_selector->document))
		return FALSE;

	page_label = g_strdup_printf ("%d", page + 1);
	retval = g_strcmp0 (page_label, gtk_editable_get_text (GTK_EDITABLE (page_selector->entry))) != 0;
	g_free (page_label);

	return retval;
}

static void
update_pages_label (PpsPageSelector *page_selector,
                    gint page)
{
	char *label_text;
	gint n_pages;

	n_pages = pps_document_get_n_pages (page_selector->document);
	if (show_page_number_in_pages_label (page_selector, page))
		label_text = g_strdup_printf (_ ("(%d of %d)"), page + 1, n_pages);
	else
		label_text = g_strdup_printf (_ ("of %d"), n_pages);
	gtk_label_set_text (GTK_LABEL (page_selector->label), label_text);
	g_free (label_text);
}

static void
pps_page_selector_set_current_page (PpsPageSelector *page_selector,
                                    gint page)
{
	if (page >= 0) {
		gchar *page_label;

		page_label = pps_document_get_page_label (page_selector->document, page);
		gtk_editable_set_text (GTK_EDITABLE (page_selector->entry), page_label);
		gtk_editable_set_position (GTK_EDITABLE (page_selector->entry), -1);
		g_free (page_label);
	} else {
		gtk_editable_set_text (GTK_EDITABLE (page_selector->entry), "");
	}

	update_pages_label (page_selector, page);
}

static void
pps_page_selector_update_max_width (PpsPageSelector *page_selector)
{
	gchar *max_label;
	gint n_pages;
	gint max_label_len;
	gchar *max_page_label;
	gchar *max_page_numeric_label;

	n_pages = pps_document_get_n_pages (page_selector->document);

	max_page_label = pps_document_get_page_label (page_selector->document, n_pages - 1);
	max_page_numeric_label = g_strdup_printf ("%d", n_pages);
	if (pps_document_has_text_page_labels (page_selector->document) != 0) {
		max_label = g_strdup_printf (_ ("(%d of %d)"), n_pages, n_pages);
		/* Do not take into account the parentheses for the size computation */
		max_label_len = g_utf8_strlen (max_label, -1) - 2;
	} else {
		max_label = g_strdup_printf (_ ("of %d"), n_pages);
		max_label_len = g_utf8_strlen (max_label, -1);
	}
	g_free (max_page_label);

	gtk_label_set_width_chars (GTK_LABEL (page_selector->label), max_label_len);
	g_free (max_label);

	max_label_len = pps_document_get_max_label_len (page_selector->document);
	gtk_editable_set_width_chars (GTK_EDITABLE (page_selector->entry),
	                              CLAMP (max_label_len, strlen (max_page_numeric_label) + 1, 12));
	g_free (max_page_numeric_label);
}

static void
page_changed_cb (PpsDocumentModel *model,
                 gint old_page,
                 gint new_page,
                 PpsPageSelector *page_selector)
{
	pps_page_selector_set_current_page (page_selector, new_page);
}

static gboolean
page_scroll_cb (GtkEventControllerScroll *self,
                gdouble dx,
                gdouble dy,
                gpointer user_data)
{
	PpsPageSelector *page_selector = PPS_PAGE_SELECTOR (user_data);
	PpsDocumentModel *model = page_selector->doc_model;
	GdkEvent *event = gtk_event_controller_get_current_event (
	    GTK_EVENT_CONTROLLER (self));
	GdkScrollDirection direction = gdk_scroll_event_get_direction (event);
	gint pageno = pps_document_model_get_page (model);

	if ((direction == GDK_SCROLL_DOWN) &&
	    (pageno < pps_document_get_n_pages (page_selector->document) - 1))
		pageno++;
	if ((direction == GDK_SCROLL_UP) && (pageno > 0))
		pageno--;
	pps_document_model_set_page (model, pageno);

	return TRUE;
}

static void
activate_cb (PpsPageSelector *page_selector)
{
	PpsDocumentModel *model;
	const char *text;
	PpsLinkDest *link_dest;
	PpsLinkAction *link_action;
	PpsLink *link;
	gchar *link_text;
	gchar *new_text;
	gint current_page;

	model = page_selector->doc_model;
	current_page = pps_document_model_get_page (model);

	text = gtk_editable_get_text (GTK_EDITABLE (page_selector->entry));

	/* Convert utf8 fullwidth numbers (eg. japanese) to halfwidth - fixes #1518 */
	new_text = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
	gtk_editable_set_text (GTK_EDITABLE (page_selector->entry), new_text);
	text = gtk_editable_get_text (GTK_EDITABLE (page_selector->entry));
	g_free (new_text);

	link_dest = pps_link_dest_new_page_label (text);
	link_action = pps_link_action_new_dest (link_dest);
	link_text = g_strdup_printf (_ ("Page %s"), text);
	link = pps_link_new (link_text, link_action);

	g_signal_emit (page_selector, widget_signals[WIDGET_ACTIVATE_LINK], 0, link);

	g_object_unref (link_dest);
	g_object_unref (link_action);
	g_object_unref (link);
	g_free (link_text);

	if (current_page == pps_document_model_get_page (model))
		pps_page_selector_set_current_page (page_selector, current_page);
}

static gboolean
focus_out_cb (PpsPageSelector *page_selector)
{
	pps_page_selector_set_current_page (page_selector,
	                                    pps_document_model_get_page (page_selector->doc_model));
	g_object_set (page_selector->entry, "xalign", 0.9, NULL);
	pps_page_selector_update_max_width (page_selector);

	return FALSE;
}

static void
pps_page_selector_init (PpsPageSelector *page_selector)
{
	gtk_widget_init_template (GTK_WIDGET (page_selector));
}

static void
pps_page_selector_clear_document (PpsPageSelector *page_selector)
{
	g_clear_object (&page_selector->document);

	// doc_model is weak pointer, so it might be NULL while we have non-NULL
	// handlers. Clearing the signals in such case is an error. We don't
	// really have to worry about setting the ids to 0, since we're already
	// in finalize
	if (page_selector->doc_model != NULL) {
		g_clear_signal_handler (&page_selector->signal_id,
		                        page_selector->doc_model);
	}
}

static void
pps_page_selector_set_document (PpsPageSelector *page_selector,
                                PpsDocument *document)
{
	if (document == NULL)
		return;

	pps_page_selector_clear_document (page_selector);
	page_selector->document = g_object_ref (document);
	gtk_widget_set_sensitive (GTK_WIDGET (page_selector), pps_document_get_n_pages (document) > 0);

	page_selector->signal_id =
	    g_signal_connect (page_selector->doc_model,
	                      "page-changed",
	                      G_CALLBACK (page_changed_cb),
	                      page_selector);

	pps_page_selector_set_current_page (page_selector,
	                                    pps_document_model_get_page (page_selector->doc_model));
	pps_page_selector_update_max_width (page_selector);
}

static void
pps_page_selector_document_changed_cb (PpsDocumentModel *model,
                                       GParamSpec *pspec,
                                       PpsPageSelector *page_selector)
{
	pps_page_selector_set_document (page_selector, pps_document_model_get_document (model));
}

void
pps_page_selector_set_model (PpsPageSelector *page_selector,
                             PpsDocumentModel *model)
{
	g_clear_weak_pointer (&page_selector->doc_model);
	page_selector->doc_model = model;
	g_object_add_weak_pointer (G_OBJECT (model),
	                           (gpointer) &page_selector->doc_model);

	pps_page_selector_set_document (page_selector, pps_document_model_get_document (model));

	page_selector->notify_document_signal_id =
	    g_signal_connect (model, "notify::document",
	                      G_CALLBACK (pps_page_selector_document_changed_cb),
	                      page_selector);
}

static void
pps_page_selector_finalize (GObject *object)
{
	PpsPageSelector *page_selector = PPS_PAGE_SELECTOR (object);

	// doc_model is weak pointer, so it might be NULL while we have non-NULL
	// handlers. Clearing the signals in such case is an error. We don't
	// really have to worry about setting the ids to 0, since we're already
	// in finalize
	if (page_selector->doc_model != NULL) {
		g_clear_signal_handler (&page_selector->notify_document_signal_id,
		                        page_selector->doc_model);
	}
	pps_page_selector_clear_document (page_selector);
	g_clear_weak_pointer (&page_selector->doc_model);

	G_OBJECT_CLASS (pps_page_selector_parent_class)->finalize (object);
}

static gboolean
pps_page_selector_grab_focus (GtkWidget *proxy)
{
	return gtk_widget_grab_focus (PPS_PAGE_SELECTOR (proxy)->entry);
}

static void
pps_page_selector_class_init (PpsPageSelectorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = pps_page_selector_finalize;
	widget_class->grab_focus = pps_page_selector_grab_focus;

	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/papers/previewer/ui/page-selector.ui");
	gtk_widget_class_bind_template_child (widget_class, PpsPageSelector, entry);
	gtk_widget_class_bind_template_child (widget_class, PpsPageSelector, label);

	gtk_widget_class_bind_template_callback (widget_class, page_scroll_cb);
	gtk_widget_class_bind_template_callback (widget_class, activate_cb);
	gtk_widget_class_bind_template_callback (widget_class, focus_out_cb);

	widget_signals[WIDGET_ACTIVATE_LINK] =
	    g_signal_new ("activate_link",
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsPageSelectorClass, activate_link),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1,
	                  G_TYPE_OBJECT);
}
