/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2009 Juanjo Marín <juanj.marin@juntadeandalucia.es>
 *  Copyright (C) 2008 Carlos Garcia Campos
 *  Copyright (C) 2004 Martin Kretzschmar
 *  Copyright (C) 2004 Red Hat, Inc.
 *  Copyright (C) 2000, 2001, 2002, 2003, 2004 Marco Pesenti Gritti
 *  Copyright © 2003, 2004, 2005, 2009, 2012 Christian Persch
 *
 *  Author:
 *    Martin Kretzschmar <martink@gnome.org>
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <adwaita.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "pps-document-signatures.h"
#include "pps-document-view.h"
#include "pps-find-sidebar.h"
#include "pps-message-area.h"
#include "pps-progress-message-area.h"
#include "pps-sidebar-bookmarks.h"
#include "pps-signature-details.h"
#include "pps-utils.h"
#include "pps-view-presentation.h"

#define MOUSE_BACK_BUTTON 8
#define MOUSE_FORWARD_BUTTON 9

typedef struct
{
	/* UI */
	GtkWidget *stack;
	GtkWidget *document_toolbar_view;
	GtkWidget *scrolled_window;
	GtkWidget *view;
	GtkWidget *message_area;
	GtkWidget *banner;
	GtkWidget *sidebar_stack;
	GtkWidget *sidebar;
	GtkWidget *sidebar_links;
	GtkWidget *sidebar_layers;
	GtkWidget *sidebar_annots;
	GtkWidget *sidebar_bookmarks;
	GtkWidget *find_sidebar;
	GtkWidget *page_selector;
	GtkWidget *header_bar;
	GtkWidget *signature_banner;

	GtkRevealer *zoom_fit_best_revealer;

	AdwToastOverlay *toast_overlay;
	AdwAlertDialog *error_alert;
	AdwAlertDialog *print_cancel_alert;
	AdwAlertDialog *rect_small_alert;
	AdwOverlaySplitView *split_view;

	/* Settings */
	GSettings *settings;
	GSettings *default_settings;
	GSettings *lockdown_settings;

	/* Progress Messages */
	GCancellable *progress_cancellable;

	/* Menu button */
	GtkWidget *action_menu_button;

	/* Popup view */
	GtkWidget *view_popup;
	PpsLink *link;
	PpsImage *image;
	PpsAnnotation *annot;

	/* Popup attachment */
	GMenuModel *attachment_popup_menu;
	GtkWidget *attachment_popup;
	GListModel *attachments;

	/* Digital signing */
	GtkWidget *certificate_listbox;
	guint signature_page;
	PpsRectangle *signature_bounding_box;
	PpsCertificateInfo *signature_certificate_info;
	PpsSignature *signature;

	/* Document */
	PpsDocumentModel *model;
	char *uri;
	gint64 uri_mtime;
	char *local_uri;
	char *display_name;
	char *edit_name;

	PpsDocument *document;
	PpsHistory *history;
	PpsMetadata *metadata;
	PpsAttachmentContext *attachment_context;
	PpsBookmarks *bookmarks;
	PpsSearchContext *search_context;
	GdkRGBA annot_color;

	GMenu *annot_menu;
	GtkWidget *annot_menu_child;

	GSimpleActionGroup *document_action_group;

	/* Has the document been modified? */
	gboolean is_modified;
	gulong modified_handler_id;

	PpsJob *save_job;
	gboolean close_after_save;

	/* Printing */
	GQueue *print_queue;
	GtkPrintSettings *print_settings;
	GtkPageSetup *print_page_setup;
	gboolean close_after_print;

	/* Misc Runtime State */
	gboolean sidebar_was_open_before_find;
	gboolean sidebar_was_open_before_collapsed;
} PpsDocumentViewPrivate;

#define GET_PRIVATE(o) pps_document_view_get_instance_private (o)

#define GS_LOCKDOWN_SCHEMA_NAME "org.gnome.desktop.lockdown"
#define GS_LOCKDOWN_SAVE "disable-save-to-disk"
#define GS_LOCKDOWN_PRINT "disable-printing"
#define GS_LOCKDOWN_PRINT_SETUP "disable-print-setup"

#define GS_SCHEMA_NAME "org.gnome.Papers"
#define GS_OVERRIDE_RESTRICTIONS "override-restrictions"
#define GS_PAGE_CACHE_SIZE "page-cache-size"
#define GS_LAST_DOCUMENT_DIRECTORY "document-directory"
#define GS_LAST_PICTURES_DIRECTORY "pictures-directory"
#define GS_ALLOW_LINKS_CHANGE_ZOOM "allow-links-change-zoom"

#define PPS_PRINT_SETTINGS_FILE "print-settings"
#define PPS_PRINT_SETTINGS_GROUP "Print Settings"
#define PPS_PAGE_SETUP_GROUP "Page Setup"

static const gchar *document_print_settings[] = {
	GTK_PRINT_SETTINGS_COLLATE,
	GTK_PRINT_SETTINGS_REVERSE,
	GTK_PRINT_SETTINGS_NUMBER_UP,
	GTK_PRINT_SETTINGS_SCALE,
	GTK_PRINT_SETTINGS_PRINT_PAGES,
	GTK_PRINT_SETTINGS_PAGE_RANGES,
	GTK_PRINT_SETTINGS_PAGE_SET,
	GTK_PRINT_SETTINGS_OUTPUT_URI
};

static void pps_document_view_document_modified_cb (PpsDocument *document,
                                                    GParamSpec *pspec,
                                                    PpsDocumentView *pps_doc_view);
static void pps_document_view_save_job_cb (PpsJob *save,
                                           PpsDocumentView *window);
static void pps_document_view_popup_cmd_open_link (GSimpleAction *action,
                                                   GVariant *parameter,
                                                   gpointer user_data);
static void pps_document_view_popup_cmd_open_link_new_window (GSimpleAction *action,
                                                              GVariant *parameter,
                                                              gpointer user_data);
static void pps_document_view_popup_cmd_copy_link_address (GSimpleAction *action,
                                                           GVariant *parameter,
                                                           gpointer user_data);
static void pps_document_view_popup_cmd_save_image_as (GSimpleAction *action,
                                                       GVariant *parameter,
                                                       gpointer user_data);
static void pps_document_view_popup_cmd_copy_image (GSimpleAction *action,
                                                    GVariant *parameter,
                                                    gpointer user_data);
static void pps_document_view_popup_cmd_annot_properties (GSimpleAction *action,
                                                          GVariant *parameter,
                                                          gpointer user_data);
static void pps_document_view_popup_cmd_remove_annotation (GSimpleAction *action,
                                                           GVariant *parameter,
                                                           gpointer user_data);
static void pps_document_view_popup_cmd_open_attachment (GSimpleAction *action,
                                                         GVariant *parameter,
                                                         gpointer user_data);
static void pps_document_view_popup_cmd_save_attachment_as (GSimpleAction *action,
                                                            GVariant *parameter,
                                                            gpointer user_data);
static void view_handle_link_cb (PpsView *view,
                                 PpsLink *link,
                                 PpsLink *backlink,
                                 PpsDocumentView *window);
static void bookmark_activated_cb (PpsSidebarBookmarks *sidebar_bookmarks,
                                   gint old_page,
                                   gint page,
                                   PpsDocumentView *window);
static void scroll_history_cb (PpsView *view,
                               GtkScrollType scroll,
                               gboolean horizontal,
                               PpsDocumentView *window);
static void scroll_child_history_cb (GtkScrolledWindow *scrolled_window,
                                     GtkScrollType scroll,
                                     gboolean horizontal,
                                     PpsDocumentView *window);
static void activate_link_cb (GObject *object,
                              PpsLink *link,
                              PpsDocumentView *window);
static void view_external_link_cb (PpsDocumentView *window,
                                   PpsLinkAction *action);

static void pps_document_view_show_find_bar (PpsDocumentView *pps_doc_view);
static void pps_document_view_close_find_bar (PpsDocumentView *pps_doc_view);

static char *pps_document_view_signature_password_callback (const char *text, gpointer user_data);
static void doc_restrictions_changed (PpsDocumentView *self, gchar *key);

G_DEFINE_TYPE_WITH_PRIVATE (PpsDocumentView, pps_document_view, ADW_TYPE_BREAKPOINT_BIN)

static void
pps_document_view_set_action_enabled (PpsDocumentView *pps_doc_view,
                                      const char *name,
                                      gboolean enabled)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GAction *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
pps_document_view_set_default_actions (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsDocument *document = priv->document;
	PpsView *view = PPS_VIEW (priv->view);
	g_autofree PpsDocumentInfo *info = pps_document_get_info (document);
	gboolean dual_mode = pps_document_model_get_page_layout (priv->model) == PPS_PAGE_LAYOUT_DUAL;

	if (!info || info->fields_mask == 0) {
		pps_document_view_set_action_enabled (pps_doc_view, "show-properties",
		                                      FALSE);
	}

	if (!PPS_IS_SELECTION (document)) {
		pps_document_view_set_action_enabled (pps_doc_view, "select-all",
		                                      FALSE);
	}

	if (!PPS_IS_DOCUMENT_FIND (document)) {
		pps_document_view_set_action_enabled (pps_doc_view, "find",
		                                      FALSE);
		pps_document_view_set_action_enabled (pps_doc_view, "toggle-find",
		                                      FALSE);
	}

	if (PPS_IS_DOCUMENT_ANNOTATIONS (document) &&
	    pps_document_annotations_can_add_annotation (PPS_DOCUMENT_ANNOTATIONS (document))) {
		g_autoptr (GMenuItem) item = g_menu_item_new (NULL, NULL);
		g_menu_item_set_attribute (item, "custom", "s", "palette");
		g_menu_insert_item (priv->annot_menu, 0, item);

		gtk_popover_menu_add_child (GTK_POPOVER_MENU (priv->view_popup),
		                            priv->annot_menu_child, "palette");
	} else {
		pps_document_view_set_action_enabled (pps_doc_view,
		                                      "add-text-annotation",
		                                      FALSE);
	}

	if (PPS_IS_DOCUMENT_SIGNATURES (document)) {
		pps_document_view_set_action_enabled (pps_doc_view, "digital-signing",
		                                      pps_document_signatures_can_sign (PPS_DOCUMENT_SIGNATURES (document)));
	}

	/* View menu */
	pps_document_view_set_action_enabled (pps_doc_view, "enable-spellchecking",
	                                      FALSE);

	/* Bookmarks menu */
	pps_document_view_set_action_enabled (pps_doc_view, "add-bookmark",
	                                      !!priv->bookmarks);

	pps_document_view_set_action_enabled (pps_doc_view, "dual-odd-left", dual_mode);

	pps_document_view_set_action_enabled (pps_doc_view, "zoom-in",
	                                      pps_view_can_zoom_in (view));
	pps_document_view_set_action_enabled (pps_doc_view, "zoom-out",
	                                      pps_view_can_zoom_out (view));

	pps_document_view_set_action_enabled (pps_doc_view, "go-back-history",
	                                      !pps_history_is_frozen (priv->history) &&
	                                          pps_history_can_go_back (priv->history));
	pps_document_view_set_action_enabled (pps_doc_view, "go-forward-history",
	                                      !pps_history_is_frozen (priv->history) &&
	                                          pps_history_can_go_forward (priv->history));

	pps_document_view_set_action_enabled (pps_doc_view, "caret-navigation",
	                                      pps_view_supports_caret_navigation (view));

	doc_restrictions_changed (pps_doc_view, NULL);
}

static void
update_sizing_buttons (PpsDocumentView *window)
{
	GAction *action;
	const gchar *mode = NULL;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "sizing-mode");

	switch (pps_document_model_get_sizing_mode (priv->model)) {
	case PPS_SIZING_FIT_PAGE:
		mode = "fit-page";
		break;
	case PPS_SIZING_FIT_WIDTH:
		mode = "fit-width";
		break;
	case PPS_SIZING_AUTOMATIC:
		mode = "automatic";
		break;
	case PPS_SIZING_FREE:
		mode = "free";
		break;
	}

	gtk_revealer_set_reveal_child (priv->zoom_fit_best_revealer,
	                               pps_document_model_get_sizing_mode (priv->model) != PPS_SIZING_AUTOMATIC);

	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (mode));
}

/**
 * pps_document_view_is_empty:
 * @pps_doc_view: The instance of the #PpsDocumentView.
 *
 * It does look if there is any document loaded or if there is any job to load
 * a document.
 *
 * Returns: %TRUE if there isn't any document loaded or any any documente to be
 *          loaded, %FALSE in other case.
 */
gboolean
pps_document_view_is_empty (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_return_val_if_fail (PPS_IS_DOCUMENT_VIEW (pps_doc_view), FALSE);

	return priv->document == NULL;
}

static void
pps_document_view_set_message_area (PpsDocumentView *window,
                                    GtkWidget *area)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (priv->message_area == area)
		return;

	if (priv->message_area)
		adw_toolbar_view_remove (ADW_TOOLBAR_VIEW (priv->document_toolbar_view), priv->message_area);
	priv->message_area = area;

	if (!area)
		return;

	adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (priv->document_toolbar_view), priv->message_area);
}

static void
error_message_detail_cb (AdwToast *toast, PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	adw_dialog_present (ADW_DIALOG (priv->error_alert), GTK_WIDGET (window));
}

G_GNUC_PRINTF (3, 4)
static void
pps_document_view_error_message (PpsDocumentView *window,
                                 GError *error,
                                 const gchar *format,
                                 ...)
{
	AdwToast *toast;
	va_list args;
	g_autofree gchar *msg = NULL;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	toast = adw_toast_new (msg);
	adw_toast_set_timeout (toast, 20);

	if (error) {
		adw_toast_set_button_label (toast, _ ("View Details"));
		g_signal_connect (toast, "button-clicked",
		                  (GCallback) error_message_detail_cb, window);

		adw_alert_dialog_set_heading (priv->error_alert, msg);
		adw_alert_dialog_set_body (priv->error_alert, error->message);
	}

	adw_toast_overlay_add_toast (priv->toast_overlay, toast);
}

G_GNUC_PRINTF (2, 3)
static void
pps_document_view_warning_message (PpsDocumentView *window,
                                   const gchar *format,
                                   ...)
{
	AdwToast *toast;
	va_list args;
	gchar *msg = NULL;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	toast = adw_toast_new (msg);
	adw_toast_set_timeout (toast, 20);

	g_free (msg);

	adw_toast_overlay_add_toast (priv->toast_overlay, toast);
}

static const gchar *
find_link_in_outlines (PpsOutlines *outlines, PpsLink *link)
{
	PpsLink *outlines_link;
	PpsLinkAction *a, *b;
	const gchar *link_title = NULL;
	GListModel *children;

	outlines_link = pps_outlines_get_link (outlines);

	if (outlines_link) {
		a = pps_link_get_action (link);
		b = pps_link_get_action (outlines_link);

		if (a && b && pps_link_action_equal (a, b)) {
			link_title = pps_link_get_title (link);
		}
	}

	children = pps_outlines_get_children (outlines);

	if (children) {
		int n = 0;
		PpsOutlines *child;

		while ((child = g_list_model_get_item (children, n++)) != NULL) {
			link_title = find_link_in_outlines (child, link);

			if (link_title)
				break;
		}
	}

	return link_title;
}

static const gchar *
pps_document_view_find_title_for_link (PpsDocumentView *window,
                                       PpsLink *link)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (PPS_IS_DOCUMENT_LINKS (priv->document) &&
	    pps_document_links_has_document_links (PPS_DOCUMENT_LINKS (priv->document))) {
		GListModel *model;
		const gchar *link_title = NULL;

		g_object_get (G_OBJECT (priv->sidebar_links),
		              "model", &model,
		              NULL);
		if (model) {
			int n = g_list_model_get_n_items (model);

			for (int i = 0; i < n; i++) {
				link_title = find_link_in_outlines (g_list_model_get_item (model, i), link);

				if (link_title)
					break;
			}

			g_object_unref (model);
		}

		return link_title;
	}

	return NULL;
}

static void
view_handle_link_cb (PpsView *view, PpsLink *link, PpsLink *backlink, PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsLink *new_link = NULL;

	if (!pps_link_get_title (link)) {
		const gchar *link_title;

		link_title = pps_document_view_find_title_for_link (window, link);
		if (link_title) {
			new_link = pps_link_new (link_title, pps_link_get_action (link));
		} else {
			PpsLinkAction *action;
			PpsLinkDest *dest;
			gchar *page_label;
			gchar *title;

			action = pps_link_get_action (link);
			dest = pps_link_action_get_dest (action);
			page_label = pps_document_links_get_dest_page_label (PPS_DOCUMENT_LINKS (priv->document), dest);
			if (!page_label)
				return;

			title = g_strdup_printf (_ ("Page %s"), page_label);
			g_free (page_label);

			new_link = pps_link_new (title, action);
			g_free (title);
		}
	}

	pps_history_add_link (priv->history, backlink);

	pps_history_add_link (priv->history, new_link ? new_link : link);
	if (new_link)
		g_object_unref (new_link);
}

static void
bookmark_activated_cb (PpsSidebarBookmarks *sidebar_bookmarks,
                       gint old_page,
                       gint page,
                       PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = pps_document_view_get_instance_private (window);

	pps_history_add_page (priv->history, old_page);
	pps_history_add_page (priv->history, page);
}

static void
scroll_history_cb (PpsView *view,
                   GtkScrollType scroll,
                   gboolean horizontal,
                   PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = pps_document_view_get_instance_private (window);
	gint old_page = -1;
	gint new_page = -1;

	if (!priv->document)
		return;

	switch (scroll) {
	case GTK_SCROLL_START:
		old_page = pps_document_model_get_page (priv->model);
		new_page = 0;
		break;
	case GTK_SCROLL_END:
		old_page = pps_document_model_get_page (priv->model);
		new_page = pps_document_get_n_pages (priv->document) - 1;
		break;
	default:
		break;
	}

	if (old_page >= 0 && new_page >= 0) {
		pps_history_add_page (priv->history, old_page);
		pps_history_add_page (priv->history, new_page);
	}
}

static void
scroll_child_history_cb (GtkScrolledWindow *scrolled_window,
                         GtkScrollType scroll,
                         gboolean horizontal,
                         PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = pps_document_view_get_instance_private (window);

	if (!priv->document)
		return;

	if (pps_document_model_get_continuous (priv->model) && !horizontal) {
		gint old_page = -1;
		gint new_page = -1;
		switch (scroll) {
		case GTK_SCROLL_START:
			old_page = pps_document_model_get_page (priv->model);
			new_page = 0;
			break;
		case GTK_SCROLL_END:
			old_page = pps_document_model_get_page (priv->model);
			new_page = pps_document_get_n_pages (priv->document) - 1;
			break;
		default:
			break;
		}
		if (old_page >= 0 && new_page >= 0) {
			pps_history_add_page (priv->history, old_page);
			pps_history_add_page (priv->history, new_page);
		}
	}
}

static void
view_selection_changed_cb (PpsView *view,
                           PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsDocument *document = priv->document;
	gboolean has_selection = pps_view_has_selection (view);
	gboolean can_annotate;

	pps_document_view_set_action_enabled (window, "copy", has_selection);

	can_annotate = PPS_IS_DOCUMENT_ANNOTATIONS (document) &&
	               pps_document_annotations_can_add_annotation (PPS_DOCUMENT_ANNOTATIONS (document));

	pps_document_view_set_action_enabled (window, "add-highlight-annotation",
	                                      can_annotate && has_selection);
}

static void
view_layers_changed_cb (PpsView *view,
                        PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	/* FIXME: We use a indirect way to call a method to rust object.
	 *        We should use a method after rust port of PpsDocumentView.
	 */
	g_signal_emit_by_name (priv->sidebar_layers, "update-visibility", NULL);
}

static void
view_caret_cursor_moved_cb (PpsView *view,
                            guint page,
                            guint offset,
                            PpsDocumentView *window)
{
	GVariant *position;
	gchar *caret_position;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (!priv->metadata)
		return;

	position = g_variant_new ("(uu)", page, offset);
	caret_position = g_variant_print (position, FALSE);
	g_variant_unref (position);

	pps_metadata_set_string (priv->metadata, "caret-position", caret_position);
	g_free (caret_position);
}

static void
page_changed_cb (PpsDocumentView *pps_doc_view,
                 gint old_page,
                 gint new_page,
                 PpsDocumentModel *model)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (priv->metadata && !pps_document_view_is_empty (pps_doc_view))
		pps_metadata_set_int (priv->metadata, "page", new_page);
}

static void
setup_model_from_metadata (PpsDocumentView *window)
{
	gint page;
	const gchar *sizing_mode;
	gdouble zoom;
	gint rotation;
	gboolean inverted_colors = FALSE;
	gboolean continuous = FALSE;
	gboolean dual_page = FALSE;
	gboolean dual_page_odd_left = FALSE;
	gboolean rtl = FALSE;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (!priv->metadata)
		return;

	/* Current page */
	if (pps_metadata_get_int (priv->metadata, "page", &page)) {
		pps_document_model_set_page (priv->model, page);
	}

	/* Sizing mode */
	if (pps_metadata_get_string (priv->metadata, "sizing-mode", &sizing_mode)) {
		GEnumValue *enum_value;

		enum_value = g_enum_get_value_by_nick (g_type_class_peek (PPS_TYPE_SIZING_MODE), sizing_mode);
		pps_document_model_set_sizing_mode (priv->model, enum_value->value);
	}

	/* Zoom */
	if (pps_document_model_get_sizing_mode (priv->model) == PPS_SIZING_FREE) {
		if (pps_metadata_get_double (priv->metadata, "zoom", &zoom)) {
			zoom *= pps_document_misc_get_widget_dpi (GTK_WIDGET (window)) / 72.0;
			pps_document_model_set_scale (priv->model, zoom);
		}
	}

	/* Rotation */
	if (pps_metadata_get_int (priv->metadata, "rotation", &rotation)) {
		switch (rotation) {
		case 90:
			rotation = 90;
			break;
		case 180:
			rotation = 180;
			break;
		case 270:
			rotation = 270;
			break;
		default:
			rotation = 0;
			break;
		}
		pps_document_model_set_rotation (priv->model, rotation);
	}

	/* Inverted Colors */
	if (pps_metadata_get_boolean (priv->metadata, "inverted-colors", &inverted_colors)) {
		pps_document_model_set_inverted_colors (priv->model, inverted_colors);
	}

	/* Continuous */
	if (pps_metadata_get_boolean (priv->metadata, "continuous", &continuous)) {
		pps_document_model_set_continuous (priv->model, continuous);
	}

	/* Dual page */
	if (pps_metadata_get_boolean (priv->metadata, "dual-page", &dual_page)) {
		pps_document_model_set_page_layout (priv->model,
		                                    dual_page ? PPS_PAGE_LAYOUT_DUAL : PPS_PAGE_LAYOUT_SINGLE);
	}

	/* Dual page odd pages left */
	if (pps_metadata_get_boolean (priv->metadata, "dual-page-odd-left", &dual_page_odd_left)) {
		pps_document_model_set_dual_page_odd_pages_left (priv->model, dual_page_odd_left);
	}

	/* Right to left document */
	if (pps_metadata_get_boolean (priv->metadata, "rtl", &rtl)) {
		pps_document_model_set_rtl (priv->model, rtl);
	}
}

static void
setup_document (PpsDocumentView *window)
{
	gint page, n_pages;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	/* Make sure to not open a document on the last page,
	 * since closing it on the last page most likely means the
	 * user was finished reading the document. In that case, reopening should
	 * show the first page. */
	page = pps_document_model_get_page (priv->model);
	n_pages = pps_document_get_n_pages (priv->document);
	if (page == n_pages - 1)
		pps_document_model_set_page (priv->model, 0);

	if (n_pages == 1)
		pps_document_model_set_page_layout (priv->model, PPS_PAGE_LAYOUT_SINGLE);
	else if (n_pages == 2)
		pps_document_model_set_dual_page_odd_pages_left (priv->model, TRUE);
}

static void
setup_view_from_metadata (PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (!priv->metadata)
		return;

	/* Caret navigation mode */
	if (pps_view_supports_caret_navigation (PPS_VIEW (priv->view))) {
		gboolean caret_navigation;
		const gchar *caret_position;

		if (pps_metadata_get_string (priv->metadata, "caret-position", &caret_position)) {
			GVariant *position;

			position = g_variant_parse (G_VARIANT_TYPE ("(uu)"), caret_position, NULL, NULL, NULL);
			if (position) {
				guint page, offset;

				g_variant_get (position, "(uu)", &page, &offset);
				g_variant_unref (position);

				pps_view_set_caret_cursor_position (PPS_VIEW (priv->view),
				                                    page, offset);
			}
		}

		if (pps_metadata_get_boolean (priv->metadata, "caret-navigation", &caret_navigation))
			pps_view_set_caret_navigation_enabled (PPS_VIEW (priv->view), caret_navigation);
	}
}

static void
page_cache_size_changed (GSettings *settings,
                         gchar *key,
                         PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	guint page_cache_mb;

	page_cache_mb = g_settings_get_uint (settings, GS_PAGE_CACHE_SIZE);
	pps_view_set_page_cache_size (PPS_VIEW (priv->view),
	                              (gsize) page_cache_mb * 1024 * 1024);
}

static void
allow_links_change_zoom_changed (GSettings *settings,
                                 gchar *key,
                                 PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean allow_links_change_zoom = g_settings_get_boolean (settings, GS_ALLOW_LINKS_CHANGE_ZOOM);

	pps_view_set_allow_links_change_zoom (PPS_VIEW (priv->view), allow_links_change_zoom);
}

static void
pps_document_view_setup_default (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsDocumentModel *model = priv->model;
	GSettings *settings = priv->default_settings;
	gboolean show_sidebar;
	g_autofree gchar *annot_color = NULL;

	/* Sidebar */
	show_sidebar = g_settings_get_boolean (settings, "show-sidebar");
	if (adw_overlay_split_view_get_collapsed (priv->split_view)) {
		priv->sidebar_was_open_before_collapsed = show_sidebar;
		adw_overlay_split_view_set_show_sidebar (priv->split_view, FALSE);
	} else {
		adw_overlay_split_view_set_show_sidebar (priv->split_view, show_sidebar);
	}

	/* Annotation color */
	annot_color = g_settings_get_string (settings, "annot-color");
	g_action_group_change_action_state (G_ACTION_GROUP (priv->document_action_group), "annot-color", g_variant_new_string (annot_color));

	/* Document model */
	pps_document_model_set_continuous (model, g_settings_get_boolean (settings, "continuous"));
	pps_document_model_set_page_layout (model, g_settings_get_boolean (settings, "dual-page") ? PPS_PAGE_LAYOUT_DUAL : PPS_PAGE_LAYOUT_SINGLE);
	pps_document_model_set_dual_page_odd_pages_left (model, g_settings_get_boolean (settings, "dual-page-odd-left"));
	pps_document_model_set_rtl (model, gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL ? TRUE : FALSE);
	pps_document_model_set_inverted_colors (model, g_settings_get_boolean (settings, "inverted-colors"));
	pps_document_model_set_sizing_mode (model, g_settings_get_enum (settings, "sizing-mode"));
	if (pps_document_model_get_sizing_mode (model) == PPS_SIZING_FREE)
		pps_document_model_set_scale (model, g_settings_get_double (settings, "zoom"));

	g_simple_action_set_state (
	    G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group),
	                                                 "enable-spellchecking")),
	    g_variant_new_boolean (FALSE));
	pps_view_set_enable_spellchecking (PPS_VIEW (priv->view),
	                                   g_settings_get_boolean (settings, "enable-spellchecking"));
}

static void
doc_restrictions_changed (PpsDocumentView *self,
                          gchar *key)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (self);
	gboolean override_restrictions = TRUE;
	g_autofree PpsDocumentInfo *info = NULL;
	gboolean ok_to_print = TRUE;
	gboolean ok_to_copy = TRUE;

	if (!priv->document) {
		pps_document_view_set_action_enabled (self, "save-as", FALSE);
		pps_document_view_set_action_enabled (self, "print", FALSE);
		return;
	}

	info = pps_document_get_info (priv->document);
	override_restrictions = g_settings_get_boolean (priv->settings,
	                                                GS_OVERRIDE_RESTRICTIONS);

	if (!override_restrictions && info && info->fields_mask & PPS_DOCUMENT_INFO_PERMISSIONS) {
		ok_to_print = (info->permissions & PPS_DOCUMENT_PERMISSIONS_OK_TO_PRINT);
		ok_to_copy = (info->permissions & PPS_DOCUMENT_PERMISSIONS_OK_TO_COPY);
	}

	if (!pps_print_operation_exists_for_document (priv->document))
		ok_to_print = FALSE;

	if (g_settings_get_boolean (priv->lockdown_settings, GS_LOCKDOWN_SAVE)) {
		ok_to_copy = FALSE;
	}

	if (priv->lockdown_settings &&
	    g_settings_get_boolean (priv->lockdown_settings, GS_LOCKDOWN_PRINT)) {
		ok_to_print = FALSE;
	}

	pps_document_view_set_action_enabled (self, "save-as", ok_to_copy);
	pps_document_view_set_action_enabled (self, "print", ok_to_print);
}

/* This function detects the schema dynamically, since not only
 * linux installations have the schemas available
 */
static void
pps_document_view_setup_lockdown (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GSettingsSchemaSource *source;
	g_autoptr (GSettingsSchema) schema = NULL;

	if (priv->lockdown_settings)
		return;

	source = g_settings_schema_source_get_default ();
	schema = g_settings_schema_source_lookup (source, GS_LOCKDOWN_SCHEMA_NAME, TRUE);

	if (!schema)
		return;

	priv->lockdown_settings = g_settings_new_full (schema, NULL, NULL);

	g_signal_connect_swapped (priv->lockdown_settings, "changed",
	                          G_CALLBACK (doc_restrictions_changed),
	                          pps_doc_view);
}

static void
pps_document_view_set_document_metadata (PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	g_autofree PpsDocumentInfo *info = NULL;

	if (!priv->metadata)
		return;

	info = pps_document_get_info (priv->document);
	if (info->fields_mask & PPS_DOCUMENT_INFO_TITLE && info->title && info->title[0] != '\0')
		pps_metadata_set_string (priv->metadata, "title", info->title);
	else
		pps_metadata_set_string (priv->metadata, "title", "");

	if (info->fields_mask & PPS_DOCUMENT_INFO_AUTHOR && info->author && info->author[0] != '\0')
		pps_metadata_set_string (priv->metadata, "author", info->author);
	else
		pps_metadata_set_string (priv->metadata, "author", "");
}

static void
pps_document_view_setup_sidebar (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GSettings *settings = priv->default_settings;

	// Use BIND_GET_NO_CHANGES so that when we have several DocumentViews
	// One instance sidebar does not get updated when the other one applies
	// the settings
	g_settings_bind (settings, "sidebar-page",
	                 priv->sidebar, "visible-child-name",
	                 G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_GET_NO_CHANGES);
}

/* Known backends (for bad extensions fix) */
#define PPS_BACKEND_PS "PSDocument"
#define PPS_BACKEND_PDF "PdfDocument"

typedef struct
{
	const gchar *backend;
	const gchar *text;
} BadTitleEntry;

/* Some docs report titles with confusing extensions (ex. .doc for pdf).
           Erase the confusing extension of the title */
static void
pps_document_view_sanitize_title (PpsDocumentView *pps_doc_view, char **title)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	const gchar *backend;
	int i;

	static const BadTitleEntry bad_extensions[] = {
		{ PPS_BACKEND_PS, ".dvi" },
		{ PPS_BACKEND_PDF, ".doc" },
		{ PPS_BACKEND_PDF, ".dvi" },
		{ PPS_BACKEND_PDF, ".indd" },
		{ PPS_BACKEND_PDF, ".rtf" }
	};

	static const BadTitleEntry bad_prefixes[] = {
		{ PPS_BACKEND_PDF, "Microsoft Word - " },
		{ PPS_BACKEND_PDF, "Microsoft PowerPoint - " }
	};

	backend = G_OBJECT_TYPE_NAME (priv->document);

	for (i = 0; i < G_N_ELEMENTS (bad_extensions); i++) {
		if (g_ascii_strcasecmp (bad_extensions[i].backend, backend) == 0 &&
		    g_str_has_suffix (*title, bad_extensions[i].text)) {
			char *new_title;

			new_title = g_strndup (*title, strlen (*title) - strlen (bad_extensions[i].text));
			g_free (*title);
			*title = new_title;
		}
	}

	for (i = 0; i < G_N_ELEMENTS (bad_prefixes); i++) {
		if (g_ascii_strcasecmp (bad_prefixes[i].backend, backend) == 0 &&
		    g_str_has_prefix (*title, bad_prefixes[i].text)) {
			char *new_title;
			int len = strlen (bad_prefixes[i].text);

			new_title = g_strdup_printf ("%s", (*title) + len);
			g_free (*title);
			*title = new_title;
		}
	}
}

static void
pps_document_view_update_title (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	AdwHeaderBar *header_bar = ADW_HEADER_BAR (priv->header_bar);
	AdwWindowTitle *title_widget = ADW_WINDOW_TITLE (adw_header_bar_get_title_widget (header_bar));
	GtkNative *native;
	char *title = NULL, *p;
	char *subtitle = NULL, *title_header = NULL;
	gchar *doc_title;
	gboolean ltr;

	if (!priv->document)
		return;

	doc_title = g_strdup (pps_document_get_title (priv->document));

	/* Make sure we get a valid title back */
	if (doc_title != NULL) {
		doc_title = g_strstrip (doc_title);

		if (doc_title[0] != '\0' &&
		    g_utf8_validate (doc_title, -1, NULL)) {
			pps_document_view_sanitize_title (pps_doc_view, &doc_title);
		} else {
			g_clear_pointer (&doc_title, g_free);
		}
	}

	ltr = gtk_widget_get_direction (GTK_WIDGET (pps_doc_view)) == GTK_TEXT_DIR_LTR;

	if (doc_title && priv->display_name) {
		title_header = doc_title;
		subtitle = priv->display_name;

		if (ltr)
			title = g_strdup_printf ("%s — %s", subtitle, title_header);
		else
			title = g_strdup_printf ("%s — %s", title_header, subtitle);

		for (p = title; *p; ++p) {
			/* an '\n' byte is always ASCII, no need for UTF-8 special casing */
			if (*p == '\n')
				*p = ' ';
		}
	} else if (priv->display_name) {
		title = g_strdup (priv->display_name);
	} else {
		title = g_strdup (_ ("Papers"));
	}

	native = gtk_widget_get_native (GTK_WIDGET (pps_doc_view));

	if (native)
		gtk_window_set_title (GTK_WINDOW (native), title);

	if (title_header && subtitle) {
		adw_window_title_set_title (title_widget, title_header);
		adw_window_title_set_subtitle (title_widget, subtitle);
	} else if (title) {
		adw_window_title_set_title (title_widget, title);
	}

	g_free (title);
}

static void
signatures_job_finished_callback (PpsJobSignatures *job,
                                  PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean is_invalid = FALSE;
	GList *signature_list = pps_job_signatures_get_signatures (job);

	if (g_list_length (signature_list) == 0)
		return;

	for (GList *iter = signature_list; iter && iter->data; iter = iter->next) {
		PpsSignature *signature = PPS_SIGNATURE (iter->data);

		if (!pps_signature_is_valid (signature)) {
			is_invalid = TRUE;
			break;
		}
	}

	if (is_invalid) {
		gtk_widget_add_css_class (priv->signature_banner, "error");
		adw_banner_set_title (ADW_BANNER (priv->signature_banner), _ ("Digital Signature is invalid!"));
	} else {
		gtk_widget_remove_css_class (priv->signature_banner, "error");
		adw_banner_set_title (ADW_BANNER (priv->signature_banner), _ ("Document has been digitally signed."));
	}
	adw_banner_set_revealed (ADW_BANNER (priv->signature_banner), TRUE);
}

static void
pps_document_view_set_document (PpsDocumentView *pps_doc_view, PpsDocument *document)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (priv->document == document)
		return;

	g_set_object (&priv->document, document);
	pps_document_model_set_document (priv->model, document);

	pps_document_view_set_message_area (pps_doc_view, NULL);

	pps_document_view_set_document_metadata (pps_doc_view);

	/* if the document contains signatures show the top info message
	   so the user can quickly see this information */
	if (PPS_IS_DOCUMENT_SIGNATURES (document)) {
		PpsJob *job = pps_job_signatures_new (document);

		g_signal_connect (job, "finished", G_CALLBACK (signatures_job_finished_callback), pps_doc_view);
		pps_job_scheduler_push_job (job, PPS_JOB_PRIORITY_NONE);
	}

	if (pps_document_get_n_pages (document) <= 0) {
		pps_document_view_warning_message (pps_doc_view, "%s",
		                                   _ ("The document contains no pages"));
	} else if (!pps_document_check_dimensions (document)) {
		pps_document_view_warning_message (pps_doc_view, "%s",
		                                   _ ("The document contains only empty pages"));
	}

	pps_document_view_set_default_actions (pps_doc_view);

	priv->is_modified = FALSE;
	priv->modified_handler_id = g_signal_connect (document, "notify::modified", G_CALLBACK (pps_document_view_document_modified_cb), pps_doc_view);

	// This cannot be done in pps_document_view_setup_default because before
	// having a document, we don't know which sidebars are supported
	pps_document_view_setup_sidebar (pps_doc_view);

	/* Set password callback */
	if (PPS_IS_DOCUMENT_SIGNATURES (priv->document)) {
		pps_document_signatures_set_password_callback (PPS_DOCUMENT_SIGNATURES (priv->document), pps_document_view_signature_password_callback, NULL);
	}

	gtk_widget_grab_focus (priv->view);

	pps_document_view_update_title (pps_doc_view);
}

static void
pps_document_view_clear_local_uri (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (priv->local_uri) {
		pps_tmp_uri_unlink (priv->local_uri);
		g_clear_pointer (&priv->local_uri, g_free);
	}
}

static void
pps_document_view_handle_link (PpsDocumentView *pps_doc_view,
                               PpsLinkDest *dest)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (dest) {
		PpsLink *link;
		PpsLinkAction *link_action;

		link_action = pps_link_action_new_dest (dest);
		link = pps_link_new (NULL, link_action);
		pps_view_handle_link (PPS_VIEW (priv->view), link);
		g_object_unref (link_action);
		g_object_unref (link);
	}
}

/**
 * pps_document_view_get_uri:
 * @pps_doc_view: The instance of the #PpsDocumentView.
 *
 * It returns the uri of the document showed in the #PpsDocumentView.
 *
 * Returns: the uri of the document showed in the #PpsDocumentView.
 */
const char *
pps_document_view_get_uri (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	return priv->uri;
}

static void
pps_document_view_reset_progress_cancellable (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (priv->progress_cancellable)
		g_cancellable_reset (priv->progress_cancellable);
	else
		priv->progress_cancellable = g_cancellable_new ();
}

/**
 * pps_document_view_open_document:
 * @pps_doc_view: A #PpsDocumentView
 * @document: A #PpsDocument to open in the document view
 * @metadata: (nullable): A #PpsMetadata if the document supports it
 * @dest: (nullable): A #PpsLinkDest
 *
 */
void
pps_document_view_open_document (PpsDocumentView *pps_doc_view,
                                 PpsDocument *document,
                                 PpsMetadata *metadata,
                                 PpsLinkDest *dest)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (document == priv->document)
		return;

	g_set_object (&priv->metadata, metadata);

	pps_document_view_clear_local_uri (pps_doc_view);

	g_clear_pointer (&priv->uri, g_free);
	priv->uri = g_strdup (pps_document_get_uri (document));

	setup_model_from_metadata (pps_doc_view);

	pps_document_view_set_document (pps_doc_view, document);

	if (priv->metadata) {
		priv->bookmarks = pps_bookmarks_new (priv->metadata);
		pps_sidebar_bookmarks_set_bookmarks (PPS_SIDEBAR_BOOKMARKS (priv->sidebar_bookmarks), priv->bookmarks);
	}

	setup_document (pps_doc_view);
	setup_view_from_metadata (pps_doc_view);

	pps_document_view_handle_link (pps_doc_view, dest);
}

static const gchar *
get_settings_key_for_directory (GUserDirectory directory)
{
	switch (directory) {
	case G_USER_DIRECTORY_PICTURES:
		return GS_LAST_PICTURES_DIRECTORY;
	case G_USER_DIRECTORY_DOCUMENTS:
	default:
		return GS_LAST_DOCUMENT_DIRECTORY;
	}
}

static void
pps_document_view_file_dialog_restore_folder (PpsDocumentView *window,
                                              GtkFileDialog *dialog,
                                              GUserDirectory directory)
{
	const gchar *dir;
	gchar *folder_path;
	GFile *folder;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	g_settings_get (priv->settings,
	                get_settings_key_for_directory (directory),
	                "ms", &folder_path);

	dir = g_get_user_special_dir (directory);
	folder_path = folder_path ?: g_strdup (dir) ?
	                                            : g_strdup (g_get_home_dir ());

	folder = g_file_new_for_path (folder_path);
	gtk_file_dialog_set_initial_folder (dialog, folder);

	g_free (folder_path);
	g_object_unref (folder);
}

static void
pps_document_view_file_dialog_save_folder (PpsDocumentView *window,
                                           GFile *file,
                                           GUserDirectory directory)
{
	gchar *path = NULL;
	GFile *folder = NULL;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (file)
		folder = g_file_get_parent (file);

	/* Store 'nothing' if the folder is the default one */
	if (folder && g_strcmp0 (g_file_get_path (folder), g_get_user_special_dir (directory)) != 0)
		path = g_file_get_path (folder);

	g_clear_object (&folder);

	g_settings_set (priv->settings,
	                get_settings_key_for_directory (directory),
	                "ms", path);
	g_free (path);
}

static void
pps_document_view_open_copy_at_dest (PpsDocumentView *window,
                                     PpsLinkDest *dest)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	GtkNative *native = gtk_widget_get_native (GTK_WIDGET (window));

	g_signal_emit_by_name (native, "open-copy", priv->metadata, dest,
	                       priv->display_name, priv->edit_name, NULL);
}

static void
pps_document_view_cmd_file_open_copy (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer user_data)
{
	PpsDocumentView *window = user_data;

	pps_document_view_open_copy_at_dest (window, NULL);
}

static void
pps_document_view_cmd_file_open_with (GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer user_data)
{
	PpsDocumentView *window = user_data;
	const char *uri;
	g_autoptr (GFile) file;
	g_autoptr (GtkFileLauncher) launcher;

	uri = pps_document_view_get_uri (window);
	file = g_file_new_for_uri (uri);
	launcher = gtk_file_launcher_new (file);
	gtk_file_launcher_launch (launcher,
	                          GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (window))),
	                          NULL, NULL, NULL);
}

static void
window_save_file_copy_ready_cb (GFile *src,
                                GAsyncResult *async_result,
                                GFile *dst)
{
	PpsDocumentView *pps_doc_view;
	GError *error = NULL;

	pps_doc_view = PPS_DOCUMENT_VIEW (g_object_get_data (G_OBJECT (dst), "pps-window"));

	if (g_file_copy_finish (src, async_result, &error)) {
		pps_document_view_warning_message (pps_doc_view, _ ("File Saved"));
		pps_tmp_file_unlink (src);
		return;
	}

	if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		gchar *name;

		name = g_file_get_basename (dst);
		pps_document_view_error_message (pps_doc_view, error,
		                                 _ ("The file could not be saved as “%s”."),
		                                 name);
		g_free (name);
	}
	pps_tmp_file_unlink (src);
	g_error_free (error);
}

static void
pps_document_view_save_remote (PpsDocumentView *pps_doc_view,
                               GFile *src,
                               GFile *dst)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_document_view_reset_progress_cancellable (pps_doc_view);
	g_object_set_data (G_OBJECT (dst), "pps-window", pps_doc_view);
	g_file_copy_async (src, dst,
	                   G_FILE_COPY_OVERWRITE,
	                   G_PRIORITY_DEFAULT,
	                   priv->progress_cancellable,
	                   NULL,
	                   dst,
	                   (GAsyncReadyCallback) window_save_file_copy_ready_cb,
	                   dst);
}

static void
pps_document_view_clear_save_job (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (priv->save_job != NULL) {
		if (!pps_job_is_finished (priv->save_job))
			pps_job_cancel (priv->save_job);

		g_signal_handlers_disconnect_by_func (priv->save_job,
		                                      pps_document_view_save_job_cb,
		                                      pps_doc_view);
		g_clear_object (&priv->save_job);
	}
}

static void
pps_document_view_save_job_cb (PpsJob *job,
                               PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	g_autoptr (GError) error = NULL;

	if (!pps_job_is_succeeded (job, &error)) {
		priv->close_after_save = FALSE;
		pps_document_view_error_message (window, error,
		                                 _ ("The file could not be saved as “%s”."),
		                                 pps_job_save_get_uri (PPS_JOB_SAVE (job)));
	} else {
		gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
		                             pps_job_save_get_uri (PPS_JOB_SAVE (job)));
	}

	pps_document_view_clear_save_job (window);

	if (priv->close_after_save)
		g_idle_add_once ((GSourceOnceFunc) gtk_window_destroy, gtk_widget_get_native (GTK_WIDGET (window)));
}

static void
file_save_dialog_response_cb (GtkFileDialog *dialog,
                              GAsyncResult *result,
                              PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GFile *file;
	gchar *uri;

	file = gtk_file_dialog_save_finish (dialog, result, NULL);

	if (!file) {
		priv->close_after_save = FALSE;
		return;
	}

	pps_document_view_file_dialog_save_folder (pps_doc_view, file,
	                                           G_USER_DIRECTORY_DOCUMENTS);

	uri = g_file_get_uri (file);
	g_clear_object (&file);

	/* FIXME: remote copy should be done here rather than in the save job,
	 * so that we can track progress and cancel the operation
	 */

	pps_document_view_clear_save_job (pps_doc_view);
	priv->save_job = pps_job_save_new (priv->document,
	                                   uri, priv->uri);
	g_signal_connect (priv->save_job, "finished",
	                  G_CALLBACK (pps_document_view_save_job_cb),
	                  pps_doc_view);
	/* The priority doesn't matter for this job */
	pps_job_scheduler_push_job (priv->save_job, PPS_JOB_PRIORITY_NONE);

	g_free (uri);
}

static void
pps_document_view_save_as (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GtkFileDialog *dialog;
	gchar *base_name, *dir_name, *var_tmp_dir, *tmp_dir;
	GFile *file, *parent, *dest_file;
	const gchar *default_dir, *dest_dir, *documents_dir;

	dialog = gtk_file_dialog_new ();

	gtk_file_dialog_set_title (dialog, _ ("Save As…"));
	pps_document_factory_add_filters (dialog, priv->document);

	file = g_file_new_for_uri (priv->uri);
	base_name = priv->edit_name;
	parent = g_file_get_parent (file);
	dir_name = g_file_get_path (parent);
	g_object_unref (parent);

	gtk_file_dialog_set_initial_name (dialog, base_name);

	documents_dir = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
	default_dir = g_file_test (documents_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) ? documents_dir : g_get_home_dir ();

	tmp_dir = g_build_filename ("tmp", NULL);
	var_tmp_dir = g_build_filename ("var", "tmp", NULL);
	dest_dir = dir_name && !g_str_has_prefix (dir_name, g_get_tmp_dir ()) &&
	                   !g_str_has_prefix (dir_name, tmp_dir) &&
	                   !g_str_has_prefix (dir_name, var_tmp_dir)
	               ? dir_name
	               : default_dir;

	dest_file = g_file_new_for_path (dest_dir);
	gtk_file_dialog_set_initial_folder (dialog, dest_file);

	g_object_unref (file);
	g_object_unref (dest_file);
	g_free (tmp_dir);
	g_free (var_tmp_dir);
	g_free (dir_name);

	gtk_file_dialog_set_modal (dialog, TRUE);

	gtk_file_dialog_save (dialog, GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pps_doc_view))), NULL,
	                      (GAsyncReadyCallback) file_save_dialog_response_cb, pps_doc_view);
}

static void
pps_document_view_cmd_save_as (GSimpleAction *action,
                               GVariant *parameter,
                               gpointer user_data)
{
	PpsDocumentView *window = user_data;

	pps_document_view_save_as (window);
}

static gchar *
print_settings_filename (gboolean create)
{
	if (create) {
		g_autofree gchar *dot_dir = g_build_filename (g_get_user_config_dir (), "papers", NULL);
		g_mkdir_with_parents (dot_dir, 0700);
	}

	return g_build_filename (g_get_user_config_dir (), "papers", PPS_PRINT_SETTINGS_FILE, NULL);
}

static GKeyFile *
get_print_settings_file (void)
{
	GKeyFile *print_settings_file;
	gchar *filename;
	GError *error = NULL;

	print_settings_file = g_key_file_new ();

	filename = print_settings_filename (FALSE);
	if (!g_key_file_load_from_file (print_settings_file,
	                                filename,
	                                G_KEY_FILE_KEEP_COMMENTS |
	                                    G_KEY_FILE_KEEP_TRANSLATIONS,
	                                &error)) {

		/* Don't warn if the file simply doesn't exist */
		if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			g_warning ("%s", error->message);

		g_error_free (error);
	}

	g_free (filename);

	return print_settings_file;
}

static void
save_print_setting_file (GKeyFile *key_file)
{
	gchar *filename;
	GError *error = NULL;

	filename = print_settings_filename (TRUE);
	g_key_file_save_to_file (key_file, filename, &error);
	if (error) {
		g_warning ("Failed to save print settings: %s", error->message);
		g_error_free (error);
	}
	g_free (filename);
}

static void
pps_document_view_save_print_settings (PpsDocumentView *window,
                                       GtkPrintSettings *print_settings)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	GKeyFile *key_file;
	gint i;

	key_file = get_print_settings_file ();
	gtk_print_settings_to_key_file (print_settings, key_file, PPS_PRINT_SETTINGS_GROUP);

	/* Always Remove n_copies from global settings */
	g_key_file_remove_key (key_file, PPS_PRINT_SETTINGS_GROUP, GTK_PRINT_SETTINGS_N_COPIES, NULL);

	/* Save print settings that are specific to the document */
	for (i = 0; i < G_N_ELEMENTS (document_print_settings); i++) {
		/* Remove it from global settings */
		g_key_file_remove_key (key_file, PPS_PRINT_SETTINGS_GROUP,
		                       document_print_settings[i], NULL);

		if (priv->metadata) {
			const gchar *value;

			value = gtk_print_settings_get (print_settings,
			                                document_print_settings[i]);
			pps_metadata_set_string (priv->metadata,
			                         document_print_settings[i], value);
		}
	}

	save_print_setting_file (key_file);
	g_key_file_free (key_file);
}

static void
pps_document_view_save_print_page_setup (PpsDocumentView *window,
                                         GtkPageSetup *page_setup)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	GKeyFile *key_file;

	key_file = get_print_settings_file ();
	gtk_page_setup_to_key_file (page_setup, key_file, PPS_PAGE_SETUP_GROUP);

	/* Do not save document settings in global file */
	g_key_file_remove_key (key_file, PPS_PAGE_SETUP_GROUP,
	                       "page-setup-orientation", NULL);
	g_key_file_remove_key (key_file, PPS_PAGE_SETUP_GROUP,
	                       "page-setup-margin-top", NULL);
	g_key_file_remove_key (key_file, PPS_PAGE_SETUP_GROUP,
	                       "page-setup-margin-bottom", NULL);
	g_key_file_remove_key (key_file, PPS_PAGE_SETUP_GROUP,
	                       "page-setup-margin-left", NULL);
	g_key_file_remove_key (key_file, PPS_PAGE_SETUP_GROUP,
	                       "page-setup-margin-right", NULL);

	save_print_setting_file (key_file);
	g_key_file_free (key_file);

	if (!priv->metadata)
		return;

	/* Save page setup options that are specific to the document */
	pps_metadata_set_int (priv->metadata, "page-setup-orientation",
	                      gtk_page_setup_get_orientation (page_setup));
	pps_metadata_set_double (priv->metadata, "page-setup-margin-top",
	                         gtk_page_setup_get_top_margin (page_setup, GTK_UNIT_MM));
	pps_metadata_set_double (priv->metadata, "page-setup-margin-bottom",
	                         gtk_page_setup_get_bottom_margin (page_setup, GTK_UNIT_MM));
	pps_metadata_set_double (priv->metadata, "page-setup-margin-left",
	                         gtk_page_setup_get_left_margin (page_setup, GTK_UNIT_MM));
	pps_metadata_set_double (priv->metadata, "page-setup-margin-right",
	                         gtk_page_setup_get_right_margin (page_setup, GTK_UNIT_MM));
}

static void
pps_document_view_load_print_settings_from_metadata (PpsDocumentView *window,
                                                     GtkPrintSettings *print_settings)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gint i;

	if (!priv->metadata)
		return;

	/* Load print setting that are specific to the document */
	for (i = 0; i < G_N_ELEMENTS (document_print_settings); i++) {
		const gchar *value = NULL;

		pps_metadata_get_string (priv->metadata,
		                         document_print_settings[i], &value);
		gtk_print_settings_set (print_settings,
		                        document_print_settings[i], value);
	}
}

static void
pps_document_view_load_print_page_setup_from_metadata (PpsDocumentView *window,
                                                       GtkPageSetup *page_setup)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gint int_value;
	gdouble double_value;
	GtkPaperSize *paper_size = gtk_page_setup_get_paper_size (page_setup);

	/* Load page setup options that are specific to the document */
	if (priv->metadata &&
	    pps_metadata_get_int (priv->metadata, "page-setup-orientation", &int_value)) {
		gtk_page_setup_set_orientation (page_setup, int_value);
	} else {
		gtk_page_setup_set_orientation (page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
	}

	if (priv->metadata &&
	    pps_metadata_get_double (priv->metadata, "page-setup-margin-top", &double_value)) {
		gtk_page_setup_set_top_margin (page_setup, double_value, GTK_UNIT_MM);
	} else {
		gtk_page_setup_set_top_margin (page_setup,
		                               gtk_paper_size_get_default_top_margin (paper_size, GTK_UNIT_MM),
		                               GTK_UNIT_MM);
	}

	if (priv->metadata &&
	    pps_metadata_get_double (priv->metadata, "page-setup-margin-bottom", &double_value)) {
		gtk_page_setup_set_bottom_margin (page_setup, double_value, GTK_UNIT_MM);
	} else {
		gtk_page_setup_set_bottom_margin (page_setup,
		                                  gtk_paper_size_get_default_bottom_margin (paper_size, GTK_UNIT_MM),
		                                  GTK_UNIT_MM);
	}

	if (priv->metadata &&
	    pps_metadata_get_double (priv->metadata, "page-setup-margin-left", &double_value)) {
		gtk_page_setup_set_left_margin (page_setup, double_value, GTK_UNIT_MM);
	} else {
		gtk_page_setup_set_left_margin (page_setup,
		                                gtk_paper_size_get_default_left_margin (paper_size, GTK_UNIT_MM),
		                                GTK_UNIT_MM);
	}

	if (priv->metadata &&
	    pps_metadata_get_double (priv->metadata, "page-setup-margin-right", &double_value)) {
		gtk_page_setup_set_right_margin (page_setup, double_value, GTK_UNIT_MM);
	} else {
		gtk_page_setup_set_right_margin (page_setup,
		                                 gtk_paper_size_get_default_right_margin (paper_size, GTK_UNIT_MM),
		                                 GTK_UNIT_MM);
	}
}

static GtkPrintSettings *
get_print_settings (GKeyFile *key_file)
{
	GtkPrintSettings *print_settings;

	print_settings = g_key_file_has_group (key_file, PPS_PRINT_SETTINGS_GROUP) ? gtk_print_settings_new_from_key_file (key_file, PPS_PRINT_SETTINGS_GROUP, NULL) : gtk_print_settings_new ();

	return print_settings ? print_settings : gtk_print_settings_new ();
}

static GtkPageSetup *
get_print_page_setup (GKeyFile *key_file)
{
	GtkPageSetup *page_setup;

	page_setup = g_key_file_has_group (key_file, PPS_PAGE_SETUP_GROUP) ? gtk_page_setup_new_from_key_file (key_file, PPS_PAGE_SETUP_GROUP, NULL) : gtk_page_setup_new ();

	return page_setup ? page_setup : gtk_page_setup_new ();
}

static void
pps_document_view_print_cancel (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsPrintOperation *op;

	if (!priv->print_queue)
		return;

	while ((op = g_queue_peek_tail (priv->print_queue))) {
		pps_print_operation_cancel (op);
	}
}

static void
pps_document_view_print_update_pending_jobs_message (PpsDocumentView *pps_doc_view,
                                                     gint n_jobs)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gchar *text = NULL;

	if (!PPS_IS_PROGRESS_MESSAGE_AREA (priv->message_area) ||
	    !priv->print_queue)
		return;

	if (n_jobs == 0) {
		pps_document_view_set_message_area (pps_doc_view, NULL);
		return;
	}

	if (n_jobs > 1) {
		text = g_strdup_printf (ngettext ("%d pending job in queue",
		                                  "%d pending jobs in queue",
		                                  n_jobs - 1),
		                        n_jobs - 1);
	}

	pps_message_area_set_secondary_text (PPS_MESSAGE_AREA (priv->message_area),
	                                     text);
	g_free (text);
}

static void
pps_document_view_print_operation_done (PpsPrintOperation *op,
                                        GtkPrintOperationResult result,
                                        PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gint n_jobs;

	switch (result) {
	case GTK_PRINT_OPERATION_RESULT_APPLY: {
		GtkPrintSettings *print_settings;

		print_settings = pps_print_operation_get_print_settings (op);
		pps_document_view_save_print_settings (pps_doc_view, print_settings);

		if (pps_print_operation_get_embed_page_setup (op)) {
			GtkPageSetup *page_setup;

			page_setup = pps_print_operation_get_default_page_setup (op);
			pps_document_view_save_print_page_setup (pps_doc_view, page_setup);
		}
	}

	break;
	case GTK_PRINT_OPERATION_RESULT_ERROR: {
		GError *error = NULL;

		pps_print_operation_get_error (op, &error);

		pps_document_view_error_message (pps_doc_view, error, _ ("Failed to Print Document"));

		g_error_free (error);
	} break;
	case GTK_PRINT_OPERATION_RESULT_CANCEL:
	default:
		break;
	}

	g_queue_remove (priv->print_queue, op);
	g_object_unref (op);
	n_jobs = g_queue_get_length (priv->print_queue);
	pps_document_view_print_update_pending_jobs_message (pps_doc_view, n_jobs);

	if (n_jobs == 0 && priv->close_after_print)
		g_idle_add_once ((GSourceOnceFunc) gtk_window_destroy,
		                 gtk_widget_get_native (GTK_WIDGET (pps_doc_view)));
}

static void
pps_document_view_print_progress_response_cb (PpsProgressMessageArea *area,
                                              gint response,
                                              PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (response == GTK_RESPONSE_CANCEL) {
		PpsPrintOperation *op;

		op = g_queue_peek_tail (priv->print_queue);
		pps_print_operation_cancel (op);
	} else {
		gtk_widget_set_visible (GTK_WIDGET (area), FALSE);
	}
}

static void
pps_document_view_print_operation_status_changed (PpsPrintOperation *op,
                                                  PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	const gchar *status;
	gdouble fraction;

	status = pps_print_operation_get_status (op);
	fraction = pps_print_operation_get_progress (op);

	if (!priv->message_area) {
		GtkWidget *area;
		const gchar *job_name;
		gchar *text;

		job_name = pps_print_operation_get_job_name (op);
		text = g_strdup_printf (_ ("Printing job “%s”"), job_name);

		area = pps_progress_message_area_new ("document-print-symbolic", text);
		pps_message_area_add_button (PPS_MESSAGE_AREA (area), _ ("C_ancel"), GTK_RESPONSE_CANCEL);
		pps_document_view_print_update_pending_jobs_message (pps_doc_view, 1);
		g_signal_connect (pps_message_area_get_info_bar (PPS_MESSAGE_AREA (area)), "response",
		                  G_CALLBACK (pps_document_view_print_progress_response_cb),
		                  pps_doc_view);

		pps_document_view_set_message_area (pps_doc_view, area);
		g_free (text);
	}

	pps_progress_message_area_set_status (PPS_PROGRESS_MESSAGE_AREA (priv->message_area),
	                                      status);
	pps_progress_message_area_set_fraction (PPS_PROGRESS_MESSAGE_AREA (priv->message_area),
	                                        fraction);
}

static void
pps_document_view_print_operation_begin_print (PpsPrintOperation *op,
                                               PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (!priv->print_queue)
		priv->print_queue = g_queue_new ();

	g_queue_push_head (priv->print_queue, op);
	pps_document_view_print_update_pending_jobs_message (pps_doc_view,
	                                                     g_queue_get_length (priv->print_queue));
}

void
pps_document_view_print_range (PpsDocumentView *pps_doc_view,
                               gint first_page,
                               gint last_page)
{
	PpsPrintOperation *op;
	GKeyFile *print_settings_file;
	GtkPrintSettings *print_settings;
	GtkPageSetup *print_page_setup;
	gint current_page;
	gint document_last_page;
	gboolean embed_page_setup;
	gchar *output_basename;
	gchar *dot;
	PpsDocumentViewPrivate *priv;
	GtkNative *native;

	g_return_if_fail (PPS_IS_DOCUMENT_VIEW (pps_doc_view));
	priv = GET_PRIVATE (pps_doc_view);
	g_return_if_fail (priv->document != NULL);

	if (!priv->print_queue)
		priv->print_queue = g_queue_new ();

	op = pps_print_operation_new (priv->document);
	if (!op) {
		g_warning ("%s", "Printing is not supported for document\n");
		return;
	}

	g_signal_connect (op, "begin_print",
	                  G_CALLBACK (pps_document_view_print_operation_begin_print),
	                  (gpointer) pps_doc_view);
	g_signal_connect (op, "status_changed",
	                  G_CALLBACK (pps_document_view_print_operation_status_changed),
	                  (gpointer) pps_doc_view);
	g_signal_connect (op, "done",
	                  G_CALLBACK (pps_document_view_print_operation_done),
	                  (gpointer) pps_doc_view);

	current_page = pps_document_model_get_page (priv->model);
	document_last_page = pps_document_get_n_pages (priv->document);

	print_settings_file = get_print_settings_file ();

	print_settings = get_print_settings (print_settings_file);
	pps_document_view_load_print_settings_from_metadata (pps_doc_view, print_settings);

	print_page_setup = get_print_page_setup (print_settings_file);
	pps_document_view_load_print_page_setup_from_metadata (pps_doc_view, print_page_setup);

	if (first_page != 1 || last_page != document_last_page) {
		GtkPageRange range;

		/* Ranges in GtkPrint are 0 - N */
		range.start = first_page - 1;
		range.end = last_page - 1;

		gtk_print_settings_set_print_pages (print_settings,
		                                    GTK_PRINT_PAGES_RANGES);
		gtk_print_settings_set_page_ranges (print_settings,
		                                    &range, 1);
	}

	output_basename = g_strdup (priv->edit_name);
	dot = g_strrstr (output_basename, ".");
	if (dot)
		dot[0] = '\0';

	/* Set output basename for printing to file */
	gtk_print_settings_set (print_settings,
	                        GTK_PRINT_SETTINGS_OUTPUT_BASENAME,
	                        output_basename);
	g_free (output_basename);

	native = gtk_widget_get_native (GTK_WIDGET (pps_doc_view));

	pps_print_operation_set_job_name (op, gtk_window_get_title (GTK_WINDOW (native)));
	pps_print_operation_set_current_page (op, current_page);
	pps_print_operation_set_print_settings (op, print_settings);
	pps_print_operation_set_default_page_setup (op, print_page_setup);
	embed_page_setup = priv->lockdown_settings ? !g_settings_get_boolean (priv->lockdown_settings,
	                                                                      GS_LOCKDOWN_PRINT_SETUP)
	                                           : TRUE;
	pps_print_operation_set_embed_page_setup (op, embed_page_setup);

	g_object_unref (print_settings);
	g_object_unref (print_page_setup);
	g_key_file_free (print_settings_file);

	pps_print_operation_run (op, GTK_WINDOW (native));
}

static void
pps_document_view_cmd_file_print (GSimpleAction *action,
                                  GVariant *state,
                                  gpointer user_data)
{
	PpsDocumentView *self = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (self);

	gtk_menu_button_popdown (GTK_MENU_BUTTON (priv->action_menu_button));
	pps_document_view_print_range (self, 1,
	                               pps_document_get_n_pages (priv->document));
}

static void
pps_document_view_cmd_file_properties (GSimpleAction *action,
                                       GVariant *state,
                                       gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GtkWidget *properties;

	properties = g_object_new (g_type_from_name ("PpsPropertiesWindow"),
	                           "document", priv->document, NULL);
	adw_dialog_present (ADW_DIALOG (properties), GTK_WIDGET (pps_doc_view));
}

static void
document_modified_confirmation_dialog_response (AdwAlertDialog *dialog,
                                                gchar *response,
                                                PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (g_str_equal (response, "yes")) {
		priv->close_after_save = TRUE;
		pps_document_view_save_as (pps_doc_view);
	} else if (g_str_equal (response, "no")) {
		gtk_window_destroy (GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pps_doc_view))));
	} else if (g_str_equal (response, "cancel")) {
		priv->close_after_save = FALSE;
	}
}

static gboolean
pps_document_view_check_document_modified (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsDocument *document = priv->document;
	AdwAlertDialog *dialog;
	const gchar *text, *secondary_text, *secondary_text_command;

	if (!document)
		return FALSE;

	if (PPS_IS_DOCUMENT_FORMS (document) &&
	    pps_document_forms_document_is_modified (PPS_DOCUMENT_FORMS (document))) {
		secondary_text = _ ("Document contains form fields that have been filled out.");
	} else if (PPS_IS_DOCUMENT_ANNOTATIONS (document) &&
	           pps_document_annotations_document_is_modified (PPS_DOCUMENT_ANNOTATIONS (document))) {
		secondary_text = _ ("Document contains new or modified annotations.");
	} else {
		return FALSE;
	}

	dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (NULL, NULL));

	text = _ ("Save Changes to a Copy?");
	secondary_text_command = _ ("If you don’t save a copy, changes will be permanently lost.");

	adw_alert_dialog_add_responses (dialog,
	                                "no", _ ("Close _Without Saving"),
	                                "cancel", _ ("_Cancel"),
	                                "yes", _ ("_Save a Copy"),
	                                NULL);
	adw_alert_dialog_set_response_appearance (dialog, "no", ADW_RESPONSE_DESTRUCTIVE);
	adw_alert_dialog_set_response_appearance (dialog, "yes", ADW_RESPONSE_SUGGESTED);

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (document_modified_confirmation_dialog_response),
	                  pps_doc_view);

	adw_alert_dialog_set_heading (dialog, text);
	adw_alert_dialog_format_body (dialog, "%s %s", secondary_text, secondary_text_command);
	adw_alert_dialog_set_default_response (dialog, "yes");

	adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (pps_doc_view));

	return TRUE;
}

static void
print_jobs_confirmation_dialog_response (AdwAlertDialog *dialog,
                                         gchar *response,
                                         PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (g_str_equal (response, "close-later")) {
		if (!priv->print_queue ||
		    g_queue_is_empty (priv->print_queue))
			gtk_window_destroy (GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pps_doc_view))));
		else
			priv->close_after_print = TRUE;
	} else if (g_str_equal (response, "force-close")) {
		priv->close_after_print = TRUE;
		if (priv->print_queue &&
		    !g_queue_is_empty (priv->print_queue)) {
			gtk_widget_set_sensitive (GTK_WIDGET (pps_doc_view), FALSE);
			pps_document_view_print_cancel (pps_doc_view);
		} else {
			gtk_window_destroy (GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pps_doc_view))));
		}
	} else {
		priv->close_after_print = FALSE;
	}
}

static gboolean
pps_document_view_check_print_queue (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_autofree gchar *text = NULL;
	gint n_print_jobs;

	n_print_jobs = priv->print_queue ? g_queue_get_length (priv->print_queue) : 0;

	if (n_print_jobs == 0)
		return FALSE;

	adw_dialog_present (ADW_DIALOG (priv->print_cancel_alert), GTK_WIDGET (pps_doc_view));

	return TRUE;
}

static void
pps_document_view_save_settings (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsView *pps_view = PPS_VIEW (priv->view);
	PpsDocumentModel *model = priv->model;
	GSettings *settings = priv->default_settings;
	PpsSizingMode sizing_mode;
	g_autoptr (GVariant) annot_color = NULL;

	g_settings_set_boolean (settings, "continuous",
	                        pps_document_model_get_continuous (model));
	g_settings_set_boolean (settings, "dual-page",
	                        pps_document_model_get_page_layout (model) == PPS_PAGE_LAYOUT_DUAL);
	g_settings_set_boolean (settings, "dual-page-odd-left",
	                        pps_document_model_get_dual_page_odd_pages_left (model));
	g_settings_set_boolean (settings, "inverted-colors",
	                        pps_document_model_get_inverted_colors (model));
	sizing_mode = pps_document_model_get_sizing_mode (model);
	g_settings_set_enum (settings, "sizing-mode", sizing_mode);
	if (sizing_mode == PPS_SIZING_FREE) {
		gdouble zoom = pps_document_model_get_scale (model);

		zoom *= 72.0 / pps_document_misc_get_widget_dpi (GTK_WIDGET (pps_doc_view));
		g_settings_set_double (settings, "zoom", zoom);
	}
	g_settings_set_boolean (settings, "show-sidebar",
	                        adw_overlay_split_view_get_show_sidebar (ADW_OVERLAY_SPLIT_VIEW (priv->split_view)));

	annot_color = g_action_group_get_action_state (G_ACTION_GROUP (priv->document_action_group), "annot-color");

	g_settings_set_string (settings, "annot-color", g_variant_get_string (annot_color, NULL));

	g_settings_set_boolean (settings, "enable-spellchecking",
	                        pps_view_get_enable_spellchecking (pps_view));
	g_settings_apply (settings);
}

static void
pps_document_view_cmd_select_page (GSimpleAction *action,
                                   GVariant *parameter,
                                   gpointer user_data)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (PPS_DOCUMENT_VIEW (user_data));

	gtk_widget_grab_focus (priv->page_selector);
}

static void
pps_document_view_cmd_continuous (GSimpleAction *action,
                                  GVariant *state,
                                  gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_document_model_set_continuous (priv->model, g_variant_get_boolean (state));
	g_simple_action_set_state (action, state);
}

static void
pps_document_view_cmd_dual (GSimpleAction *action,
                            GVariant *state,
                            gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsDocument *document = priv->document;
	gboolean has_pages = FALSE;
	gboolean dual_page;

	dual_page = g_variant_get_boolean (state);

	pps_document_model_set_page_layout (priv->model,
	                                    dual_page ? PPS_PAGE_LAYOUT_DUAL : PPS_PAGE_LAYOUT_SINGLE);
	g_simple_action_set_state (action, state);

	if (document)
		has_pages = pps_document_get_n_pages (priv->document) > 0;

	pps_document_view_set_action_enabled (window, "dual-odd-left", dual_page && has_pages);
}

static void
pps_document_view_cmd_dual_odd_pages_left (GSimpleAction *action,
                                           GVariant *state,
                                           gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_document_model_set_dual_page_odd_pages_left (priv->model,
	                                                 g_variant_get_boolean (state));
	g_simple_action_set_state (action, state);
}

static void
pps_document_view_cmd_rtl (GSimpleAction *action,
                           GVariant *state,
                           gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_document_model_set_rtl (priv->model,
	                            g_variant_get_boolean (state));
	g_simple_action_set_state (action, state);
}

static void
pps_document_view_change_sizing_mode_action_state (GSimpleAction *action,
                                                   GVariant *state,
                                                   gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	const gchar *mode;

	mode = g_variant_get_string (state, NULL);

	if (g_str_equal (mode, "fit-page"))
		pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FIT_PAGE);
	else if (g_str_equal (mode, "fit-width"))
		pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FIT_WIDTH);
	else if (g_str_equal (mode, "automatic"))
		pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_AUTOMATIC);
	else if (g_str_equal (mode, "free"))
		pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FREE);
	else
		g_assert_not_reached ();

	g_simple_action_set_state (action, state);
}

static void
pps_document_view_cmd_annotation_color_action_state (GSimpleAction *action,
                                                     GVariant *state,
                                                     gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	const gchar *color;

	color = g_variant_get_string (state, NULL);

	if (g_str_equal (color, "yellow"))
		gdk_rgba_parse (&priv->annot_color, "#f5c211");
	else if (g_str_equal (color, "orange"))
		gdk_rgba_parse (&priv->annot_color, "#ff7800");
	else if (g_str_equal (color, "red"))
		gdk_rgba_parse (&priv->annot_color, "#ed333b");
	else if (g_str_equal (color, "purple"))
		gdk_rgba_parse (&priv->annot_color, "#c061cb");
	else if (g_str_equal (color, "blue"))
		gdk_rgba_parse (&priv->annot_color, "#3584e4");
	else if (g_str_equal (color, "green"))
		gdk_rgba_parse (&priv->annot_color, "#33d17a");
	else
		g_assert_not_reached ();

	if (priv->view)
		pps_view_set_annotation_color (PPS_VIEW (priv->view), &priv->annot_color);

	g_simple_action_set_state (action, state);
}

static void
pps_document_view_cmd_view_zoom (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gdouble zoom = g_variant_get_double (parameter);

	pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FREE);
	pps_document_model_set_scale (priv->model,
	                              zoom * pps_document_misc_get_widget_dpi (GTK_WIDGET (pps_doc_view)) / 72.0);
}

static void
pps_document_view_cmd_edit_select_all (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_view_select_all (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_toggle_find (GSimpleAction *action,
                                   GVariant *state,
                                   gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean show = g_variant_get_boolean (state);

	if (show)
		pps_document_view_show_find_bar (pps_doc_view);
	else
		pps_document_view_close_find_bar (pps_doc_view);
	pps_view_find_set_highlight_search (PPS_VIEW (priv->view), show);

	g_simple_action_set_state (action, state);
}

static void
pps_document_view_find_restart (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gint page;

	page = pps_document_model_get_page (priv->model);
	pps_view_find_restart (PPS_VIEW (priv->view), page);
}

static void
pps_document_view_cmd_find (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer user_data)
{
	PpsView *view;
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_autofree gchar *selected_text = NULL;

	view = PPS_VIEW (priv->view);
	selected_text = pps_view_get_selected_text (view);

	if (selected_text != NULL && g_strcmp0 (selected_text, "") != 0) {
		pps_search_context_set_search_term (priv->search_context, selected_text);
		g_action_group_change_action_state (G_ACTION_GROUP (priv->document_action_group),
		                                    "toggle-find", g_variant_new_boolean (TRUE));
		pps_document_view_find_restart (pps_doc_view);
	} else {
		gtk_widget_activate_action (GTK_WIDGET (pps_doc_view), "doc.toggle-find", NULL);
	}
}

static void
pps_document_view_cmd_find_next (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_view_find_next (PPS_VIEW (priv->view));
	pps_find_sidebar_next (PPS_FIND_SIDEBAR (priv->find_sidebar));
}

static void
pps_document_view_cmd_find_previous (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_view_find_previous (PPS_VIEW (priv->view));
	pps_find_sidebar_previous (PPS_FIND_SIDEBAR (priv->find_sidebar));
}

static void
pps_document_view_cmd_edit_copy (GSimpleAction *action,
                                 GVariant *parameter,
                                 gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_view_copy (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_edit_rotate_left (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gint rotation = pps_document_model_get_rotation (priv->model);

	pps_document_model_set_rotation (priv->model, rotation - 90);
}

static void
pps_document_view_cmd_edit_rotate_right (GSimpleAction *action,
                                         GVariant *parameter,
                                         gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gint rotation = pps_document_model_get_rotation (priv->model);

	pps_document_model_set_rotation (priv->model, rotation + 90);
}

static void
pps_document_view_cmd_view_enable_spellchecking (GSimpleAction *action,
                                                 GVariant *state,
                                                 gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_view_set_enable_spellchecking (PPS_VIEW (priv->view),
	                                   g_variant_get_boolean (state));
	g_simple_action_set_state (action, state);
}

static void
pps_document_view_cmd_view_zoom_in (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FREE);
	pps_view_zoom_in (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_view_zoom_out (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_document_model_set_sizing_mode (priv->model, PPS_SIZING_FREE);
	pps_view_zoom_out (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_go_back_history (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gint old_page = pps_document_model_get_page (priv->model);

	if (old_page >= 0)
		pps_history_add_page (priv->history, old_page);

	pps_history_go_back (priv->history);
}

static void
pps_document_view_cmd_go_forward_history (GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_history_go_forward (priv->history);
}

static void
pps_document_view_cmd_go_previous_page (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_previous_page (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_go_next_page (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_next_page (PPS_VIEW (priv->view));
}

static void
pps_document_view_cmd_go_first_page (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	gint old_page = pps_document_model_get_page (priv->model);
	gint new_page = 0;
	pps_document_model_set_page (priv->model, 0);
	if (old_page >= 0) {
		pps_history_add_page (priv->history, old_page);
		pps_history_add_page (priv->history, new_page);
	}
}

static void
pps_document_view_cmd_go_last_page (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	gint old_page = pps_document_model_get_page (priv->model);
	gint new_page = pps_document_get_n_pages (priv->document) - 1;

	pps_document_model_set_page (priv->model,
	                             pps_document_get_n_pages (priv->document) - 1);
	if (old_page >= 0 && new_page >= 0) {
		pps_history_add_page (priv->history, old_page);
		pps_history_add_page (priv->history, new_page);
	}
}

static void
pps_document_view_cmd_go_forward (GSimpleAction *action,
                                  GVariant *parameter,
                                  gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	int n_pages, current_page;

	n_pages = pps_document_get_n_pages (priv->document);
	current_page = pps_document_model_get_page (priv->model);

	if (current_page + 10 < n_pages) {
		pps_document_model_set_page (priv->model, current_page + 10);
	}
}

static void
pps_document_view_cmd_go_backwards (GSimpleAction *action,
                                    GVariant *parameter,
                                    gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	int current_page;

	current_page = pps_document_model_get_page (priv->model);

	if (current_page - 10 >= 0) {
		pps_document_model_set_page (priv->model, current_page - 10);
	}
}

static void
pps_document_view_cmd_bookmarks_add (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsBookmark bm;
	gchar *page_label;

	bm.page = pps_document_model_get_page (priv->model);
	page_label = pps_document_get_page_label (priv->document, bm.page);
	bm.title = g_strdup_printf (_ ("Page %s"), page_label);
	g_free (page_label);

	/* PpsBookmarks takes ownership of bookmark */
	pps_bookmarks_add (priv->bookmarks, &bm);
}

static void
pps_document_view_cmd_bookmarks_delete (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsBookmark bm;

	bm.page = pps_document_model_get_page (priv->model);
	bm.title = NULL;

	pps_bookmarks_delete (priv->bookmarks, &bm);
}

static void
pps_document_view_cmd_escape (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer user_data)
{
	PpsDocumentView *window = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	GtkNative *native = gtk_widget_get_native (GTK_WIDGET (window));

	if (gtk_stack_get_visible_child (GTK_STACK (priv->sidebar_stack)) == priv->find_sidebar)
		gtk_widget_activate_action (GTK_WIDGET (window), "doc.toggle-find", NULL);
	else if (gtk_window_is_fullscreen (GTK_WINDOW (native))) {
		g_action_group_change_action_state (G_ACTION_GROUP (native), "fullscreen", g_variant_new_boolean (FALSE));
	} else if (adw_overlay_split_view_get_collapsed (priv->split_view) &&
	           adw_overlay_split_view_get_show_sidebar (priv->split_view))
		adw_overlay_split_view_set_show_sidebar (priv->split_view, FALSE);
}

static void
save_sizing_mode (PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsSizingMode mode;
	GEnumValue *enum_value;

	if (!priv->metadata || pps_document_view_is_empty (window))
		return;

	mode = pps_document_model_get_sizing_mode (priv->model);
	enum_value = g_enum_get_value (g_type_class_peek (PPS_TYPE_SIZING_MODE), mode);
	pps_metadata_set_string (priv->metadata, "sizing-mode",
	                         enum_value->value_nick);
}

static void
pps_document_view_document_modified_cb (PpsDocument *document,
                                        GParamSpec *pspec,
                                        PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	AdwWindowTitle *window_title = ADW_WINDOW_TITLE (
	    adw_header_bar_get_title_widget (ADW_HEADER_BAR (priv->header_bar)));
	const gchar *title = adw_window_title_get_title (window_title);
	gchar *new_title;

	if (priv->is_modified)
		return;

	priv->is_modified = TRUE;
	if (gtk_widget_get_direction (GTK_WIDGET (pps_doc_view)) == GTK_TEXT_DIR_RTL)
		new_title = g_strconcat ("• ", title, NULL);
	else
		new_title = g_strconcat (title, " •", NULL);

	if (new_title) {
		adw_window_title_set_title (window_title, new_title);
		g_free (new_title);
	}
}

static void
sizing_mode_changed_cb (PpsDocumentModel *model,
                        GParamSpec *pspec,
                        PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsSizingMode sizing_mode = pps_document_model_get_sizing_mode (model);

	g_object_set (priv->scrolled_window,
	              "hscrollbar-policy",
	              sizing_mode == PPS_SIZING_FREE ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER,
	              "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	              NULL);

	update_sizing_buttons (pps_doc_view);
	save_sizing_mode (pps_doc_view);
}

static void
zoom_changed_cb (PpsDocumentModel *model, GParamSpec *pspec, PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsView *view = PPS_VIEW (priv->view);

	pps_document_view_set_action_enabled (pps_doc_view, "zoom-in",
	                                      pps_view_can_zoom_in (view));
	pps_document_view_set_action_enabled (pps_doc_view, "zoom-out",
	                                      pps_view_can_zoom_out (view));

	if (!priv->metadata)
		return;

	if (pps_document_model_get_sizing_mode (model) == PPS_SIZING_FREE && !pps_document_view_is_empty (pps_doc_view)) {
		gdouble zoom;

		zoom = pps_document_model_get_scale (model);
		zoom *= 72.0 / pps_document_misc_get_widget_dpi (GTK_WIDGET (pps_doc_view));
		pps_metadata_set_double (priv->metadata, "zoom", zoom);
	}
}

static void
continuous_changed_cb (PpsDocumentModel *model,
                       GParamSpec *pspec,
                       PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean continuous;
	GAction *action;

	continuous = pps_document_model_get_continuous (model);

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "continuous");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (continuous));

	if (priv->metadata && !pps_document_view_is_empty (pps_doc_view))
		pps_metadata_set_boolean (priv->metadata, "continuous", continuous);
}

static void
rotation_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gint rotation = pps_document_model_get_rotation (model);

	if (priv->metadata && !pps_document_view_is_empty (window))
		pps_metadata_set_int (priv->metadata, "rotation",
		                      rotation);
}

static void
inverted_colors_changed_cb (PpsDocumentModel *model,
                            GParamSpec *pspec,
                            PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gboolean inverted_colors = pps_document_model_get_inverted_colors (model);
	GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (window));
	AdwStyleManager *manager = adw_style_manager_get_for_display (display);
	GtkNative *native = gtk_widget_get_native (GTK_WIDGET (window));
	GAction *action;

	if (native) {
		action = g_action_map_lookup_action (G_ACTION_MAP (native), "inverted-colors");
		g_simple_action_set_state (G_SIMPLE_ACTION (action),
		                           g_variant_new_boolean (inverted_colors));
	}

	if (inverted_colors)
		adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_FORCE_DARK);
	else
		adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_DEFAULT);

	if (priv->metadata && !pps_document_view_is_empty (window))
		pps_metadata_set_boolean (priv->metadata, "inverted-colors",
		                          inverted_colors);
}

static void
page_layout_changed_cb (PpsDocumentModel *model,
                        GParamSpec *pspec,
                        PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean dual_page;
	GAction *action;

	dual_page = pps_document_model_get_page_layout (model) == PPS_PAGE_LAYOUT_DUAL;

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "dual-page");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (dual_page));

	if (priv->metadata && !pps_document_view_is_empty (pps_doc_view))
		pps_metadata_set_boolean (priv->metadata, "dual-page", dual_page);
}

static void
dual_mode_odd_pages_left_changed_cb (PpsDocumentModel *model,
                                     GParamSpec *pspec,
                                     PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean odd_left;
	GAction *action;

	odd_left = pps_document_model_get_dual_page_odd_pages_left (model);

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "dual-odd-left");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (odd_left));

	if (priv->metadata && !pps_document_view_is_empty (pps_doc_view))
		pps_metadata_set_boolean (priv->metadata, "dual-page-odd-left",
		                          odd_left);
}

static void
direction_changed_cb (PpsDocumentModel *model,
                      GParamSpec *pspec,
                      PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean rtl;
	GAction *action;

	rtl = pps_document_model_get_rtl (model);

	action = g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "rtl");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (rtl));

	if (priv->metadata && !pps_document_view_is_empty (pps_doc_view))
		pps_metadata_set_boolean (priv->metadata, "rtl",
		                          rtl);
}

static void
pps_document_view_view_cmd_toggle_sidebar (GSimpleAction *action,
                                           GVariant *state,
                                           gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean show_side_pane;

	show_side_pane = g_variant_get_boolean (state);
	g_simple_action_set_state (action, g_variant_new_boolean (show_side_pane));
	adw_overlay_split_view_set_show_sidebar (priv->split_view, show_side_pane);
}

static void
sidebar_visibility_changed_cb (AdwOverlaySplitView *split_view,
                               GParamSpec *pspec,
                               PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GSettings *settings = priv->default_settings;
	gboolean visible = adw_overlay_split_view_get_show_sidebar (split_view);

	g_action_group_change_action_state (G_ACTION_GROUP (priv->document_action_group), "show-sidebar",
	                                    g_variant_new_boolean (visible));

	if (priv->metadata && gtk_stack_get_visible_child (GTK_STACK (priv->sidebar_stack)) != priv->find_sidebar && !adw_overlay_split_view_get_collapsed (priv->split_view))
		g_settings_set_boolean (settings, "show-sidebar", visible);
}

static void
sidebar_collapsed_changed_cb (AdwOverlaySplitView *split_view,
                              GParamSpec *pspec,
                              PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	gboolean collapsed = adw_overlay_split_view_get_collapsed (split_view);

	if (collapsed) {
		priv->sidebar_was_open_before_collapsed =
		    adw_overlay_split_view_get_show_sidebar (split_view) && priv->sidebar_was_open_before_find;
		adw_overlay_split_view_set_show_sidebar (split_view, FALSE);
	} else {
		adw_overlay_split_view_set_show_sidebar (split_view,
		                                         priv->sidebar_was_open_before_collapsed);
	}
}

static void
sidebar_navigate_to_view (PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (adw_overlay_split_view_get_collapsed (priv->split_view)) {
		adw_overlay_split_view_set_show_sidebar (priv->split_view, FALSE);
		gtk_widget_grab_focus (priv->view);
	}
}

static void
view_menu_link_popup (PpsDocumentView *pps_doc_view,
                      PpsLink *link)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean show_external = FALSE;
	gboolean show_internal = FALSE;

	g_set_object (&priv->link, link);

	if (link) {
		PpsLinkAction *pps_action;

		pps_action = pps_link_get_action (link);
		if (pps_action) {
			switch (pps_link_action_get_action_type (pps_action)) {
			case PPS_LINK_ACTION_TYPE_GOTO_DEST:
			case PPS_LINK_ACTION_TYPE_GOTO_REMOTE:
				show_internal = TRUE;
				break;
			case PPS_LINK_ACTION_TYPE_EXTERNAL_URI:
			case PPS_LINK_ACTION_TYPE_LAUNCH:
				show_external = TRUE;
				break;
			default:
				break;
			}
		}
	}

	pps_document_view_set_action_enabled (pps_doc_view, "open-link", show_external);
	pps_document_view_set_action_enabled (pps_doc_view, "copy-link-address", show_external);
	pps_document_view_set_action_enabled (pps_doc_view, "go-to-link", show_internal);

	pps_document_view_set_action_enabled (pps_doc_view, "open-link-new-window", show_internal);
}

static void
view_menu_image_popup (PpsDocumentView *pps_doc_view,
                       PpsImage *image)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean show_image = !!image;

	g_set_object (&priv->image, image);

	pps_document_view_set_action_enabled (pps_doc_view, "save-image", show_image);
	pps_document_view_set_action_enabled (pps_doc_view, "copy-image", show_image);
}

static void
view_menu_annot_popup (PpsDocumentView *pps_doc_view,
                       PpsAnnotation *annot)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean show_annot_props = FALSE;
	gboolean show_attachment = FALSE;
	gboolean can_remove_annots = FALSE;

	g_set_object (&priv->annot, annot);

	if (annot) {
		show_annot_props = PPS_IS_ANNOTATION_MARKUP (annot);

		if (PPS_IS_ANNOTATION_ATTACHMENT (annot)) {
			PpsAttachment *attachment;

			attachment = pps_annotation_attachment_get_attachment (PPS_ANNOTATION_ATTACHMENT (annot));
			if (attachment) {
				GListStore *attachments;

				show_attachment = TRUE;

				g_clear_object (&priv->attachments);

				attachments = g_list_store_new (PPS_TYPE_ATTACHMENT);
				g_list_store_append (attachments, attachment);
				priv->attachments = G_LIST_MODEL (attachments);
			}
		}
	}

	if (PPS_IS_DOCUMENT_ANNOTATIONS (priv->document))
		can_remove_annots = pps_document_annotations_can_remove_annotation (PPS_DOCUMENT_ANNOTATIONS (priv->document));

	pps_document_view_set_action_enabled (pps_doc_view, "annot-properties", show_annot_props);
	pps_document_view_set_action_enabled (pps_doc_view, "remove-annot", annot != NULL && can_remove_annots);
	pps_document_view_set_action_enabled (pps_doc_view, "open-attachment", show_attachment);
	pps_document_view_set_action_enabled (pps_doc_view, "save-attachment", show_attachment);
}

static void
view_menu_popup_cb (PpsView *view,
                    GList *items,
                    double x,
                    double y,
                    PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GList *l;
	gboolean has_link = FALSE;
	gboolean has_image = FALSE;
	gboolean has_annot = FALSE;
	graphene_point_t window_point;

	for (l = items; l; l = g_list_next (l)) {
		if (PPS_IS_LINK (l->data)) {
			view_menu_link_popup (pps_doc_view, PPS_LINK (l->data));
			has_link = TRUE;
		} else if (PPS_IS_IMAGE (l->data)) {
			view_menu_image_popup (pps_doc_view, PPS_IMAGE (l->data));
			has_image = TRUE;
		} else if (PPS_IS_ANNOTATION (l->data)) {
			view_menu_annot_popup (pps_doc_view, PPS_ANNOTATION (l->data));
			has_annot = TRUE;
		}
	}

	if (!has_link)
		view_menu_link_popup (pps_doc_view, NULL);
	if (!has_image)
		view_menu_image_popup (pps_doc_view, NULL);
	if (!has_annot)
		view_menu_annot_popup (pps_doc_view, NULL);

	if (!gtk_widget_compute_point (GTK_WIDGET (view),
	                               gtk_widget_get_parent (priv->view_popup),
	                               &GRAPHENE_POINT_INIT (x, y), &window_point))
		g_warn_if_reached ();

	gtk_popover_set_pointing_to (GTK_POPOVER (priv->view_popup),
	                             &(const GdkRectangle) { window_point.x, window_point.y, 1, 1 });
	gtk_popover_popup (GTK_POPOVER (priv->view_popup));
}

static gboolean
attachment_bar_menu_popup_cb (GtkWidget *attachbar,
                              gdouble x,
                              gdouble y,
                              GListModel *attachments,
                              PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	graphene_point_t new_point;

	g_assert (attachments != NULL);

	pps_document_view_set_action_enabled (pps_doc_view, "open-attachment", TRUE);

	pps_document_view_set_action_enabled (pps_doc_view, "save-attachment", TRUE);

	priv->attachments = g_object_ref (attachments);

	if (!gtk_widget_compute_point (GTK_WIDGET (attachbar),
	                               gtk_widget_get_parent (priv->attachment_popup),
	                               &GRAPHENE_POINT_INIT (x, y),
	                               &new_point))
		return FALSE;

	gtk_popover_set_pointing_to (GTK_POPOVER (priv->attachment_popup),
	                             &(const GdkRectangle) { new_point.x, new_point.y, 1, 1 });
	gtk_popover_popup (GTK_POPOVER (priv->attachment_popup));

	return TRUE;
}

void
pps_document_view_handle_annot_popup (PpsDocumentView *pps_doc_view,
                                      PpsAnnotation *annot)
{
	view_menu_annot_popup (pps_doc_view, annot);
}

static void
pps_document_view_show_find_bar (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (gtk_stack_get_visible_child (GTK_STACK (priv->sidebar_stack)) == priv->find_sidebar) {
		gtk_widget_grab_focus (priv->find_sidebar);
		return;
	}

	if (priv->document == NULL || !PPS_IS_DOCUMENT_FIND (priv->document)) {
		g_error ("Find action should be insensitive since document doesn't support find");
		return;
	}

	if (!adw_overlay_split_view_get_collapsed (priv->split_view)) {
		priv->sidebar_was_open_before_find =
		    g_variant_get_boolean (g_action_group_get_action_state (G_ACTION_GROUP (priv->document_action_group),
		                                                            "show-sidebar"));
	} else {
		priv->sidebar_was_open_before_find = priv->sidebar_was_open_before_collapsed;
	}

	pps_history_freeze (priv->history);

	gtk_stack_set_visible_child (GTK_STACK (priv->sidebar_stack),
	                             priv->find_sidebar);

	gtk_widget_grab_focus (priv->find_sidebar);
	g_action_group_change_action_state (G_ACTION_GROUP (priv->document_action_group), "show-sidebar",
	                                    g_variant_new_boolean (TRUE));
	pps_document_view_set_action_enabled (pps_doc_view, "find-next", TRUE);
	pps_document_view_set_action_enabled (pps_doc_view, "find-previous", TRUE);
}

static void
pps_document_view_close_find_bar (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (gtk_stack_get_visible_child (GTK_STACK (priv->sidebar_stack)) != priv->find_sidebar)
		return;

	if (!adw_overlay_split_view_get_collapsed (priv->split_view))
		adw_overlay_split_view_set_show_sidebar (priv->split_view,
		                                         priv->sidebar_was_open_before_find);

	priv->sidebar_was_open_before_find = TRUE;

	gtk_stack_set_visible_child (GTK_STACK (priv->sidebar_stack),
	                             priv->sidebar);

	pps_document_view_set_action_enabled (pps_doc_view, "find-next", FALSE);
	pps_document_view_set_action_enabled (pps_doc_view, "find-previous", FALSE);

	pps_history_thaw (priv->history);
}

static void
pps_document_view_set_caret_navigation_enabled (PpsDocumentView *window,
                                                gboolean enabled)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (priv->metadata)
		pps_metadata_set_boolean (priv->metadata, "caret-navigation", enabled);

	pps_view_set_caret_navigation_enabled (PPS_VIEW (priv->view), enabled);
}

static void
pps_document_view_cmd_view_toggle_caret_navigation (GSimpleAction *action,
                                                    GVariant *state,
                                                    gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	AdwToast *toast;
	g_autofree gchar *msg = NULL;
	gboolean enabled;

	enabled = pps_view_is_caret_navigation_enabled (PPS_VIEW (priv->view));
	pps_document_view_set_caret_navigation_enabled (window, !enabled);

	if (!enabled) {
		msg = g_strdup_printf (_ ("Caret navigation mode is now enabled, press F7 to disable."));
	} else {
		msg = g_strdup_printf (_ ("Caret navigation mode is now disabled, press F7 to enable."));
	}

	toast = adw_toast_new (msg);
	adw_toast_set_timeout (toast, 5);

	adw_toast_overlay_add_toast (priv->toast_overlay, toast);
}

static void
pps_document_view_cmd_add_highlight_annotation (GSimpleAction *action,
                                                GVariant *state,
                                                gpointer user_data)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (PPS_DOCUMENT_VIEW (user_data));

	pps_view_add_text_markup_annotation_for_selected_text (PPS_VIEW (priv->view));
	g_signal_emit_by_name (priv->sidebar_annots, "annot-added", NULL);
}

static void
pps_document_view_cmd_add_text_annotation (GSimpleAction *action,
                                           GVariant *state,
                                           gpointer user_data)
{
	PpsDocumentView *self = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (self);
	GdkRectangle rect;
	gint x, y;

	pps_document_misc_get_pointer_position (priv->view, &x, &y);
	if (x == y && x == -1) {
		// Check if the pointer is not over the current surface, then
		// it should be in the popover, and we should get the point
		// from where the popover is pointing
		gtk_popover_get_pointing_to (GTK_POPOVER (priv->view_popup), &rect);
		x = rect.x;
		y = rect.y;
	}

	if (pps_view_add_text_annotation_at_point (PPS_VIEW (priv->view), x, y)) {
		/* FIXME: We use a indirect way to call a method to rust object.
		 *        We should use a method after rust port of PpsWindow.
		 */
		g_signal_emit_by_name (priv->sidebar_annots, "annot-added", NULL);
	}
}

static void
pps_document_view_dispose (GObject *object)
{
	PpsDocumentView *window = PPS_DOCUMENT_VIEW (object);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	g_clear_object (&priv->attachment_context);
	g_clear_object (&priv->search_context);
	g_clear_object (&priv->bookmarks);
	g_clear_object (&priv->metadata);

	g_clear_object (&priv->attachment_popup_menu);

	g_settings_apply (priv->default_settings);

	g_clear_object (&priv->lockdown_settings);

	g_clear_object (&priv->document);

	pps_document_view_clear_save_job (window);
	pps_document_view_clear_local_uri (window);
	g_clear_object (&priv->progress_cancellable);

	g_clear_object (&priv->link);
	g_clear_object (&priv->image);
	g_clear_object (&priv->annot);
	g_clear_object (&priv->attachments);

	g_clear_pointer (&priv->uri, g_free);

	g_clear_pointer (&priv->display_name, g_free);
	g_clear_pointer (&priv->edit_name, g_free);

	g_clear_object (&priv->history);

	g_clear_pointer (&priv->print_queue, g_queue_free);

	G_OBJECT_CLASS (pps_document_view_parent_class)->dispose (object);
}

static void
pps_document_view_button_pressed (GtkGestureClick *self,
                                  gint n_press,
                                  gdouble x,
                                  gdouble y,
                                  gpointer user_data)
{
	PpsDocumentView *window = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (self));

	if (gdk_event_get_event_type (event) == GDK_TOUCH_BEGIN)
		return;

	switch (gdk_button_event_get_button (event)) {
	case MOUSE_BACK_BUTTON: {
		gint old_page = pps_document_model_get_page (priv->model);

		if (old_page >= 0)
			pps_history_add_page (priv->history, old_page);

		pps_history_go_back (priv->history);
		break;
	}
	case MOUSE_FORWARD_BUTTON:
		pps_history_go_forward (priv->history);
		break;
	default:
		break;
	}
}

struct PasswordData {
	GMainLoop *loop;
	GtkWidget *entry;
	char *password;
};

static void
on_password (AdwMessageDialog *self,
             char *response,
             gpointer user_data)
{
	struct PasswordData *data = user_data;

	if (g_strcmp0 (response, "login") == 0)
		data->password = g_strdup (gtk_editable_get_text (GTK_EDITABLE (data->entry)));
	else
		data->password = NULL;

	g_main_loop_quit (data->loop);
}

static char *
pps_document_view_signature_password_callback (const char *text, gpointer user_data)
{
	AdwDialog *dialog;
	GtkWindow *parent = NULL;
	g_autofree char *body = NULL;
	struct PasswordData data;

	parent = gtk_application_get_active_window (GTK_APPLICATION (g_application_get_default ()));
	body = g_strdup_printf (_ ("Please enter password for %s"), text);

	dialog = adw_alert_dialog_new (_ ("Password Required"), body);
	adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"));
	adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "login", _ ("_Login"));
	adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "login", ADW_RESPONSE_SUGGESTED);
	adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "login");

	data.entry = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (data.entry), TRUE);
	gtk_entry_set_visibility (GTK_ENTRY (data.entry), FALSE);

	adw_alert_dialog_set_extra_child (ADW_ALERT_DIALOG (dialog), data.entry);

	/* Note: Poppler (NSS) requires this function to return the requested value sync.
	 * There is no async API so we need a nested loop here -.-
	 */
	data.loop = g_main_loop_new (NULL, FALSE);

	g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (on_password), &data);
	adw_dialog_present (dialog, GTK_WIDGET (parent));
	gtk_widget_grab_focus (data.entry);
	g_main_loop_run (data.loop);

	return data.password;
}

static void
load_signed_document (gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	g_autofree char *uri = g_filename_to_uri (pps_signature_get_destination_file (priv->signature), NULL, NULL);

	pps_spawn (uri, NULL);
	g_clear_object (&priv->signature);
}

static void
on_document_signed (GObject *source_object,
                    GAsyncResult *result,
                    gpointer user_data)
{
	g_autoptr (GError) error = NULL;
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gboolean ret;

	ret = pps_document_signatures_sign_finish (PPS_DOCUMENT_SIGNATURES (priv->document), result, &error);
	if (!ret) {
		g_warning ("Could not sign document: %s", error->message);
		g_clear_object (&priv->signature);
	} else {
		g_idle_add_once (load_signed_document, pps_doc_view);
	}
}

static void
pps_document_view_certificate_save_file (PpsDocumentView *pps_doc_view,
                                         const PpsCertificateInfo *certificate_info,
                                         const char *filename)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_signature_set_destination_file (priv->signature, filename);
	pps_signature_set_page (priv->signature, pps_document_model_get_page (priv->model));
	pps_document_signatures_sign (PPS_DOCUMENT_SIGNATURES (priv->document), priv->signature, NULL, on_document_signed, pps_doc_view);
}

static void
on_signed_save (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_autoptr (GError) error = NULL;
	GFile *file = gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source_object), res, &error);
	g_autofree char *filename = NULL;

	if (error) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("Could not save signed file: %s", error->message);

		g_clear_pointer (&priv->signature_certificate_info, pps_certificate_info_free);
		return;
	}

	filename = g_file_get_path (file);
	pps_document_view_certificate_save_file (pps_doc_view, priv->signature_certificate_info, filename);
}

static void
pps_document_view_certificate_save_as_dialog (PpsDocumentView *pps_doc_view)
{
	GtkFileDialog *dialog;
	GtkWindow *parent_window = GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pps_doc_view)));
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_autofree char *tmp = NULL;
	char *last_dot = NULL;

	dialog = gtk_file_dialog_new ();
	gtk_file_dialog_set_title (dialog, _ ("Save Signed File"));

	last_dot = strrchr (priv->edit_name, '.');
	if (last_dot) {
		g_autofree char *prefix = g_strdup (priv->edit_name);
		prefix[last_dot - priv->edit_name] = '\0';

		tmp = g_strdup_printf ("%s-signed.%s", prefix, last_dot + 1);
	} else {
		tmp = g_strdup_printf ("%s-signed", priv->edit_name);
	}
	gtk_file_dialog_set_initial_name (dialog, tmp);

	gtk_file_dialog_save (dialog, parent_window, NULL, on_signed_save, pps_doc_view);
}

static void
pps_document_view_banner_cancel_cb (AdwBanner *self,
                                    gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsView *view = PPS_VIEW (priv->view);

	pps_view_cancel_signature_rect (view);
	adw_banner_set_title (ADW_BANNER (priv->banner), "");
	adw_banner_set_revealed (ADW_BANNER (priv->banner), FALSE);
}

static void
pps_document_view_draw_rect_action (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	PpsView *view = PPS_VIEW (priv->view);

	adw_banner_set_title (ADW_BANNER (priv->banner), _ ("Draw a rectangle to insert a signature field"));
	adw_banner_set_button_label (ADW_BANNER (priv->banner), _ ("_Cancel"));
	g_signal_connect_object (G_OBJECT (priv->banner), "button-clicked", G_CALLBACK (pps_document_view_banner_cancel_cb), pps_doc_view, 0);
	adw_banner_set_revealed (ADW_BANNER (priv->banner), TRUE);

	pps_view_start_signature_rect (view);
}

static void
pps_document_view_certificate_selection_response (GtkWidget *dialog,
                                                  char *response,
                                                  gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_autofree char *tmp = NULL;
	time_t t;

	if (!priv->signature_certificate_info)
		return;

	priv->signature = pps_signature_new (NULL, PPS_SIGNATURE_STATUS_INVALID, PPS_CERTIFICATE_STATUS_GENERIC_ERROR, NULL);
	;
	pps_signature_set_certificate_info (priv->signature, priv->signature_certificate_info);

	time (&t);
	tmp = g_strdup_printf (_ ("Digitally signed by %s\nDate: %s"),
	                       pps_certificate_info_get_subject_common_name (priv->signature_certificate_info),
	                       ctime (&t));
	pps_signature_set_signature (priv->signature, tmp);
	pps_signature_set_signature_left (priv->signature, pps_certificate_info_get_subject_common_name (priv->signature_certificate_info));

	pps_document_view_draw_rect_action (pps_doc_view);
}

static void
on_radio_button_toggled (GtkWidget *button,
                         gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);

	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (button))) {
		PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
		GtkWidget *row = gtk_widget_get_ancestor (GTK_WIDGET (button), ADW_TYPE_ACTION_ROW);
		const char *nick = adw_preferences_row_get_title (ADW_PREFERENCES_ROW (row));

		g_clear_pointer (&priv->signature_certificate_info, pps_certificate_info_free);

		priv->signature_certificate_info = pps_document_signature_get_certificate_info (PPS_DOCUMENT_SIGNATURES (priv->document), (const char *) nick);
	}
}

static void
pps_document_view_create_certificate_selection (PpsDocumentView *pps_doc_view)
{
	AdwDialog *dialog;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GList *certificates;
	GList *list;

	certificates = pps_document_signatures_get_available_signing_certificates (PPS_DOCUMENT_SIGNATURES (priv->document));

	dialog = adw_alert_dialog_new (_ ("Certificate Required"), NULL);

	if (certificates != NULL) {
		GtkWidget *check_button_group = NULL;

		adw_alert_dialog_set_body (ADW_ALERT_DIALOG (dialog), _ ("Select signing certificate"));
		adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "select", _ ("Select"));
		adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "select", ADW_RESPONSE_SUGGESTED);

		adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"));
		adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
		adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "select");

		priv->certificate_listbox = gtk_list_box_new ();
		gtk_list_box_set_selection_mode (GTK_LIST_BOX (priv->certificate_listbox), GTK_SELECTION_NONE);
		gtk_widget_add_css_class (priv->certificate_listbox, "content");

		for (list = certificates; list && list->data; list = list->next) {
			PpsCertificateInfo *certificate_info = list->data;
			GtkWidget *row = adw_action_row_new ();
			GtkWidget *check_button;

			adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), pps_certificate_info_get_id (certificate_info));

			check_button = gtk_check_button_new ();
			gtk_widget_set_valign (check_button, GTK_ALIGN_CENTER);
			g_signal_connect_object (G_OBJECT (check_button), "toggled", G_CALLBACK (on_radio_button_toggled), pps_doc_view, 0);
			adw_action_row_add_prefix (ADW_ACTION_ROW (row), GTK_WIDGET (check_button));
			adw_action_row_set_activatable_widget (ADW_ACTION_ROW (row), check_button);
			gtk_check_button_set_group (GTK_CHECK_BUTTON (check_button), GTK_CHECK_BUTTON (check_button_group));

			if (!check_button_group) {
				check_button_group = check_button;
				gtk_check_button_set_active (GTK_CHECK_BUTTON (check_button), TRUE);
			}

			adw_action_row_set_subtitle (ADW_ACTION_ROW (row), pps_certificate_info_get_subject_common_name (certificate_info));
			gtk_list_box_insert (GTK_LIST_BOX (priv->certificate_listbox), row, -1);
		}

		gtk_list_box_select_row (GTK_LIST_BOX (priv->certificate_listbox), gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->certificate_listbox), 0));
		adw_alert_dialog_set_extra_child (ADW_ALERT_DIALOG (dialog), priv->certificate_listbox);
		g_list_free_full (certificates, (GDestroyNotify) pps_certificate_info_free);
		g_signal_connect (dialog, "response::select", G_CALLBACK (pps_document_view_certificate_selection_response), pps_doc_view);
	} else {
		adw_alert_dialog_set_body (ADW_ALERT_DIALOG (dialog), _ ("A certificate is required to sign this document"));
		// show close if the certificate is null, or if no cert is found
		adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _ ("_Close"));
		adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "close");
	}

	adw_dialog_present (dialog, GTK_WIDGET (pps_doc_view));
}

static void
pps_document_view_on_signature_rect_too_small_response (PpsDocumentView *pps_doc_view,
                                                        gchar *response)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	if (g_str_equal (response, "sign")) {
		pps_document_view_create_certificate_selection (pps_doc_view);
		return;
	}

	g_action_group_activate_action (G_ACTION_GROUP (priv->document_action_group), "digital-signing", NULL);
}

static void
pps_document_view_show_signature_rect_too_small_warning (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	adw_dialog_present (ADW_DIALOG (priv->rect_small_alert), GTK_WIDGET (pps_doc_view));
}

static gboolean
calculate_font_size (PpsRectangle *rect,
                     const char *text,
                     int border_size,
                     unsigned int *font_size)
{
	double width;
	double height;
	int chars;
	unsigned int current_size;

	width = (rect->x2 - rect->x1) / 2 - 2 * border_size;
	// Y is starting from bottom
	height = rect->y2 - rect->y1 - 2 * border_size;
	chars = strlen (text);
	current_size = 1;

	/* Try to find the biggest font size that will fit in the rect */
	do {
		int chars_per_line = floor (width / current_size);
		int lines = height / current_size;

		if (chars_per_line * lines < chars)
			break;
	} while (current_size++);

	if (current_size != 1) {
		if (font_size)
			*font_size = current_size - 1;

		return TRUE;
	}

	return FALSE;
}

static void
pps_document_view_on_signature_rect (PpsView *view,
                                     guint page,
                                     PpsRectangle *rect,
                                     PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	gdouble width, height;
	unsigned int font_size;

	adw_banner_set_revealed (ADW_BANNER (priv->banner), FALSE);

	if (!rect)
		return;

	pps_document_get_page_size (priv->document, page, &width, &height);

	priv->signature_page = page;
	g_clear_pointer (&priv->signature_bounding_box, pps_rectangle_free);
	priv->signature_bounding_box = pps_rectangle_copy (rect);

	/* Calculate font size for main (right) signature text */
	if (!calculate_font_size (rect,
	                          pps_signature_get_signature (priv->signature),
	                          pps_signature_get_border_width (priv->signature),
	                          &font_size)) {
		pps_document_view_show_signature_rect_too_small_warning (pps_doc_view);
		return;
	}

	pps_signature_set_font_size (priv->signature, font_size);

	/* Calculate font size for left signature text */
	if (!calculate_font_size (rect,
	                          pps_signature_get_signature_left (priv->signature),
	                          pps_signature_get_border_width (priv->signature),
	                          &font_size)) {
		pps_document_view_show_signature_rect_too_small_warning (pps_doc_view);
		return;
	}

	pps_signature_set_left_font_size (priv->signature, font_size);
	pps_signature_set_rect (priv->signature, rect);

	pps_document_view_certificate_save_as_dialog (pps_doc_view);
}

static void
pps_document_view_cmd_digital_signing (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer user_data)
{
	PpsDocumentView *pps_doc_view = user_data;

	pps_document_view_create_certificate_selection (pps_doc_view);
}

static const GActionEntry actions[] = {
	{ "open-with", pps_document_view_cmd_file_open_with },
	{ "escape", pps_document_view_cmd_escape },
	{ "go-forward", pps_document_view_cmd_go_forward },
	{ "go-backwards", pps_document_view_cmd_go_backwards },
	{ "go-back-history", pps_document_view_cmd_go_back_history },
	{ "go-forward-history", pps_document_view_cmd_go_forward_history },
	{ "find", pps_document_view_cmd_find },
	{ "toggle-find", NULL, NULL, "false", pps_document_view_cmd_toggle_find },
	{ "find-next", pps_document_view_cmd_find_next },
	{ "find-previous", pps_document_view_cmd_find_previous },
	{ "continuous", NULL, NULL, "true", pps_document_view_cmd_continuous },
	{ "show-properties", pps_document_view_cmd_file_properties },
	{ "open-copy", pps_document_view_cmd_file_open_copy },
	{ "save-as", pps_document_view_cmd_save_as },
	{ "print", pps_document_view_cmd_file_print },
	{ "copy", pps_document_view_cmd_edit_copy },
	{ "select-all", pps_document_view_cmd_edit_select_all },
	{ "go-previous-page", pps_document_view_cmd_go_previous_page },
	{ "go-next-page", pps_document_view_cmd_go_next_page },
	{ "go-first-page", pps_document_view_cmd_go_first_page },
	{ "go-last-page", pps_document_view_cmd_go_last_page },
	{ "select-page", pps_document_view_cmd_select_page },
	{ "dual-page", NULL, NULL, "false", pps_document_view_cmd_dual },
	{ "dual-odd-left", NULL, NULL, "false", pps_document_view_cmd_dual_odd_pages_left },
	{ "rtl", NULL, NULL, "false", pps_document_view_cmd_rtl },
	{ "show-sidebar", NULL, NULL, "false", pps_document_view_view_cmd_toggle_sidebar },
	{ "enable-spellchecking", NULL, NULL, "false", pps_document_view_cmd_view_enable_spellchecking },
	{ "rotate-left", pps_document_view_cmd_edit_rotate_left },
	{ "rotate-right", pps_document_view_cmd_edit_rotate_right },
	{ "zoom-in", pps_document_view_cmd_view_zoom_in },
	{ "zoom-out", pps_document_view_cmd_view_zoom_out },
	{ "add-bookmark", pps_document_view_cmd_bookmarks_add },
	{ "delete-bookmark", pps_document_view_cmd_bookmarks_delete },
	{ "sizing-mode", NULL, "s", "'free'", pps_document_view_change_sizing_mode_action_state },
	{ "zoom", pps_document_view_cmd_view_zoom, "d" },
	{ "caret-navigation", pps_document_view_cmd_view_toggle_caret_navigation },
	{ "add-text-annotation", pps_document_view_cmd_add_text_annotation },
	{ "add-highlight-annotation", pps_document_view_cmd_add_highlight_annotation },
	{ "annot-color", NULL, "s", "'yellow'", pps_document_view_cmd_annotation_color_action_state },
	/* Popups specific items */
	{ "open-link", pps_document_view_popup_cmd_open_link },
	{ "open-link-new-window", pps_document_view_popup_cmd_open_link_new_window },
	{ "go-to-link", pps_document_view_popup_cmd_open_link },
	{ "copy-link-address", pps_document_view_popup_cmd_copy_link_address },
	{ "save-image", pps_document_view_popup_cmd_save_image_as },
	{ "copy-image", pps_document_view_popup_cmd_copy_image },
	{ "open-attachment", pps_document_view_popup_cmd_open_attachment },
	{ "save-attachment", pps_document_view_popup_cmd_save_attachment_as },
	{ "annot-properties", pps_document_view_popup_cmd_annot_properties },
	{ "remove-annot", pps_document_view_popup_cmd_remove_annotation },
	{ "remove-annot", pps_document_view_popup_cmd_remove_annotation },
	{ "digital-signing", pps_document_view_cmd_digital_signing }

};

static void
activate_link_cb (GObject *object, PpsLink *link, PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_handle_link (PPS_VIEW (priv->view), link);
}

static void
history_changed_cb (PpsHistory *history,
                    PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_document_view_set_action_enabled (window, "go-back-history",
	                                      pps_history_can_go_back (priv->history));
	pps_document_view_set_action_enabled (window, "go-forward-history",
	                                      pps_history_can_go_forward (priv->history));
}

static void
sidebar_layers_visibility_changed (GObject *layers,
                                   PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_reload (PPS_VIEW (priv->view));
}

static void
sidebar_annots_annot_activated_cb (GObject *sidebar_annots,
                                   PpsMapping *annot_mapping,
                                   PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_focus_annotation (PPS_VIEW (priv->view), annot_mapping);
}

static void
view_annot_removed (PpsView *view,
                    PpsAnnotation *annot,
                    PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	/* FIXME: We use a indirect way to call a method to rust object.
	 *        We should use a method after rust port of PpsWindow.
	 */
	g_signal_emit_by_name (priv->sidebar_annots, "annot-removed", NULL);
}

static gchar *
get_uri (const char *filename, PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gchar *ret;

	/* The filename can be a valid URI (file:///) or a path in filesystem */
	if (g_uri_is_valid (filename, G_URI_FLAGS_NONE, NULL))
		ret = g_strdup (filename);
	else if (g_path_is_absolute (filename))
		ret = g_filename_to_uri (filename, NULL, NULL);
	else {
		g_autoptr (GFile) base_file = NULL;
		g_autoptr (GFile) file = NULL;
		g_autofree gchar *dir = NULL;

		dir = g_path_get_dirname (priv->uri);
		base_file = g_file_new_for_uri (dir);
		file = g_file_resolve_relative_path (base_file, filename);
		ret = g_file_get_uri (file);
	}

	return ret;
}

static gboolean
file_is_pdf (const char *uri)
{
	g_autofree gchar *content_type = NULL;

	content_type = pps_file_get_mime_type (uri, FALSE, NULL);

	if (content_type && g_ascii_strcasecmp (content_type, "application/pdf") == 0)
		return true;

	return false;
}

static void
launch_action (PpsDocumentView *pps_doc_view, PpsLinkAction *action)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	const char *filename = pps_link_action_get_filename (action);
	gchar *uri;

	if (filename == NULL)
		return;

	uri = get_uri (filename, pps_doc_view);

	if (!file_is_pdf (uri)) {
		pps_document_view_warning_message (pps_doc_view,
		                                   _ ("Security alert: this document has been prevented from opening the file “%s”"),
		                                   filename);
		return;
	}

	// The launch action should not reference the file itself. If it
	// does, simply ignore it
	if (g_strcmp0 (priv->uri, uri) != 0)
		pps_spawn (uri, pps_link_action_get_dest (action));

	g_free (uri);
}

static void
launch_external_uri (PpsDocumentView *window, PpsLinkAction *action)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	const gchar *uri = pps_link_action_get_uri (action);
	GError *error = NULL;
	gboolean ret;
	GdkAppLaunchContext *context;
	GdkDisplay *display;
	GFile *file;
	gchar *uri_scheme;

	display = gtk_widget_get_display (GTK_WIDGET (window));
	context = gdk_display_get_app_launch_context (display);
	file = g_file_new_for_uri (uri);
	uri_scheme = g_file_get_uri_scheme (file);
	g_object_unref (file);

	if (uri_scheme == NULL) {
		gchar *new_uri;

		/* Not a valid uri, assume http if it starts with www */
		if (g_str_has_prefix (uri, "www.")) {
			new_uri = g_strdup_printf ("http://%s", uri);
		} else {
			GFile *parent;

			file = g_file_new_for_uri (priv->uri);
			parent = g_file_get_parent (file);
			g_object_unref (file);
			if (parent) {
				gchar *parent_uri = g_file_get_uri (parent);

				new_uri = g_build_filename (parent_uri, uri, NULL);
				g_free (parent_uri);
				g_object_unref (parent);
			} else {
				new_uri = g_filename_to_uri (uri, NULL, NULL);
			}
		}
		ret = g_app_info_launch_default_for_uri (new_uri, G_APP_LAUNCH_CONTEXT (context), &error);
		g_free (new_uri);
	} else {
		ret = g_app_info_launch_default_for_uri (uri, G_APP_LAUNCH_CONTEXT (context), &error);
	}

	if (ret == FALSE) {
		pps_document_view_error_message (window, error,
		                                 "%s", _ ("Unable to open external link"));
		g_error_free (error);
	}

	g_object_unref (context);
}

static void
open_remote_link (PpsDocumentView *window, PpsLinkAction *action)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	gchar *uri;
	gchar *dir;

	dir = g_path_get_dirname (priv->uri);

	uri = g_build_filename (dir, pps_link_action_get_filename (action),
	                        NULL);
	g_free (dir);

	// The goto-remote action should not reference the file itself. If it
	// does, simply ignore it. Ideally, we would not launch a new instance
	// but open a new tab or a new window, but that's not possible until
	// https://gitlab.gnome.org/GNOME/Incubator/papers/-/issues/104 is fixed
	if (g_strcmp0 (priv->uri, uri) != 0)
		pps_spawn (uri, pps_link_action_get_dest (action));

	g_free (uri);
}

static void
do_action_named (PpsDocumentView *window, PpsLinkAction *action)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	const gchar *name = pps_link_action_get_name (action);
	GActionGroup *group = G_ACTION_GROUP (priv->document_action_group);

	if (g_ascii_strcasecmp (name, "FirstPage") == 0) {
		g_action_group_activate_action (group, "go-first-page", NULL);
	} else if (g_ascii_strcasecmp (name, "PrevPage") == 0) {
		g_action_group_activate_action (group, "go-previous-page", NULL);
	} else if (g_ascii_strcasecmp (name, "NextPage") == 0) {
		g_action_group_activate_action (group, "go-next-page", NULL);
	} else if (g_ascii_strcasecmp (name, "LastPage") == 0) {
		g_action_group_activate_action (group, "go-last-page", NULL);
	} else if (g_ascii_strcasecmp (name, "GoToPage") == 0) {
		g_action_group_activate_action (group, "select-page", NULL);
	} else if (g_ascii_strcasecmp (name, "Find") == 0) {
		g_action_group_activate_action (group, "find", NULL);
	} else if (g_ascii_strcasecmp (name, "Close") == 0) {
		gtk_window_close (GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (window))));
	} else if (g_ascii_strcasecmp (name, "Print") == 0) {
		g_action_group_activate_action (group, "print", NULL);
	} else if (g_ascii_strcasecmp (name, "SaveAs") == 0) {
		g_action_group_activate_action (group, "save-as", NULL);
	} else {
		g_warning ("Unimplemented named action: %s, please post a "
		           "bug report in Papers Gitlab "
		           "(https://gitlab.gnome.org/GNOME/Incubator/papers/issues) "
		           "with a testcase.",
		           name);
	}
}

static void
reset_form (PpsDocumentView *window, PpsLinkAction *action)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	PpsDocument *document = priv->document;

	if (PPS_IS_DOCUMENT_FORMS (document)) {
		pps_document_forms_reset_form (PPS_DOCUMENT_FORMS (document), action);
		pps_view_reload (PPS_VIEW (priv->view));
	}
}

static void
view_external_link_cb (PpsDocumentView *window, PpsLinkAction *action)
{
	switch (pps_link_action_get_action_type (action)) {
	case PPS_LINK_ACTION_TYPE_GOTO_DEST: {
		PpsLinkDest *dest;

		dest = pps_link_action_get_dest (action);
		if (!dest)
			return;

		pps_document_view_open_copy_at_dest (window, dest);
	} break;
	case PPS_LINK_ACTION_TYPE_EXTERNAL_URI:
		launch_external_uri (window, action);
		break;
	case PPS_LINK_ACTION_TYPE_LAUNCH:
		launch_action (window, action);
		break;
	case PPS_LINK_ACTION_TYPE_GOTO_REMOTE:
		open_remote_link (window, action);
		break;
	case PPS_LINK_ACTION_TYPE_NAMED:
		do_action_named (window, action);
		break;
	case PPS_LINK_ACTION_TYPE_RESET_FORM:
		reset_form (window, action);
		break;
	default:
		g_assert_not_reached ();
	}
}

static void
pps_document_view_popup_cmd_open_link (GSimpleAction *action,
                                       GVariant *parameter,
                                       gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_handle_link (PPS_VIEW (priv->view), priv->link);
}

static void
pps_document_view_popup_cmd_open_link_new_window (GSimpleAction *action,
                                                  GVariant *parameter,
                                                  gpointer user_data)
{
	PpsLinkAction *pps_action = NULL;
	PpsLinkDest *dest;
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_action = pps_link_get_action (priv->link);
	if (!pps_action)
		return;

	dest = pps_link_action_get_dest (pps_action);
	if (!dest)
		return;

	pps_document_view_open_copy_at_dest (window, dest);
}

static void
pps_document_view_popup_cmd_copy_link_address (GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer user_data)
{
	PpsLinkAction *pps_action;
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_action = pps_link_get_action (priv->link);
	if (!pps_action)
		return;

	pps_view_copy_link_address (PPS_VIEW (priv->view),
	                            pps_action);
}

static GFile *
create_file_from_uri_for_format (const gchar *uri,
                                 GdkPixbufFormat *format)
{
	GFile *target_file;
	gchar **extensions;
	gchar *uri_extension;
	gint i;

	extensions = gdk_pixbuf_format_get_extensions (format);
	for (i = 0; extensions[i]; i++) {
		if (g_str_has_suffix (uri, extensions[i])) {
			g_strfreev (extensions);
			return g_file_new_for_uri (uri);
		}
	}

	uri_extension = g_strconcat (uri, ".", extensions[0], NULL);
	target_file = g_file_new_for_uri (uri_extension);
	g_free (uri_extension);
	g_strfreev (extensions);

	return target_file;
}

static void
image_save_dialog_response_cb (GtkFileDialog *dialog,
                               GAsyncResult *result,
                               PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	GFile *target_file;
	gboolean is_native;
	GError *error = NULL;
	GdkPixbuf *pixbuf;
	gchar *uri;
	gchar *filename;
	gchar *file_format;
	GdkPixbufFormat *format = NULL;
	GFile *file;

	file = gtk_file_dialog_save_finish (dialog, result, NULL);

	if (!file)
		return;

	pps_document_view_file_dialog_save_folder (pps_doc_view, file,
	                                           G_USER_DIRECTORY_PICTURES);

	uri = g_file_get_uri (file);
	g_clear_object (&file);

	format = pps_gdk_pixbuf_format_by_extension (uri);

	if (format == NULL && g_strrstr (uri, ".") == NULL) {
		/* no extension found and no extension provided within uri */
		format = pps_gdk_pixbuf_format_by_extension (".png");
		if (format == NULL) {
			/* no .png support, try .jpeg */
			format = pps_gdk_pixbuf_format_by_extension (".jpeg");
		}
	}

	if (format == NULL) {
		pps_document_view_error_message (pps_doc_view, NULL,
		                                 "%s",
		                                 _ ("Couldn’t find appropriate format to save image"));
		g_free (uri);
		return;
	}

	target_file = create_file_from_uri_for_format (uri, format);
	g_free (uri);

	is_native = g_file_is_native (target_file);
	if (is_native) {
		filename = g_file_get_path (target_file);
	} else {
		/* Create a temporary local file to save to */
		if (pps_mkstemp ("saveimage.XXXXXX", &filename, &error) == -1)
			goto has_error;
	}

	pps_document_doc_mutex_lock (priv->document);
	pixbuf = pps_document_images_get_image (PPS_DOCUMENT_IMAGES (priv->document),
	                                        priv->image);
	pps_document_doc_mutex_unlock (priv->document);

	file_format = gdk_pixbuf_format_get_name (format);
	gdk_pixbuf_save (pixbuf, filename, file_format, &error, NULL);
	g_free (file_format);
	g_object_unref (pixbuf);

has_error:
	if (error) {
		pps_document_view_error_message (pps_doc_view, error,
		                                 "%s", _ ("The image could not be saved."));
		g_error_free (error);
		g_free (filename);
		g_object_unref (target_file);
		return;
	}

	if (!is_native) {
		GFile *source_file;

		source_file = g_file_new_for_path (filename);

		pps_document_view_save_remote (pps_doc_view, source_file, target_file);
		g_object_unref (source_file);
	}

	g_free (filename);
	g_object_unref (target_file);
}

static void
pps_document_view_popup_cmd_save_image_as (GSimpleAction *action,
                                           GVariant *parameter,
                                           gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	GtkFileDialog *dialog;
	g_autoptr (GDateTime) now = NULL;
	g_autofree gchar *initial_name = NULL;

	if (!priv->image)
		return;

	dialog = gtk_file_dialog_new ();

	gtk_file_dialog_set_title (dialog, _ ("Save Image"));
	gtk_file_dialog_set_modal (dialog, TRUE);

	now = g_date_time_new_now_local ();

	/* We simply give user a default name here. The extension is not hardcoded
	 * and we will detect the target file extension to determine the format.
	 * We will fallback to png or jpeg when no extension is specified.
	 */
	initial_name = g_date_time_format (now, "%c.png");
	gtk_file_dialog_set_initial_name (dialog, initial_name);

	pps_document_view_file_dialog_restore_folder (window, dialog,
	                                              G_USER_DIRECTORY_PICTURES);

	gtk_file_dialog_save (dialog, GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (window))), NULL,
	                      (GAsyncReadyCallback) image_save_dialog_response_cb,
	                      window);
}

static void
pps_document_view_popup_cmd_copy_image (GSimpleAction *action,
                                        GVariant *parameter,
                                        gpointer user_data)
{
	GdkClipboard *clipboard;
	GdkPixbuf *pixbuf;
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (!priv->image)
		return;

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window));
	pps_document_doc_mutex_lock (priv->document);
	pixbuf = pps_document_images_get_image (PPS_DOCUMENT_IMAGES (priv->document),
	                                        priv->image);
	pps_document_doc_mutex_unlock (priv->document);

	gdk_clipboard_set_texture (clipboard,
	                           gdk_texture_new_for_pixbuf (pixbuf));
	g_object_unref (pixbuf);
}

static void
pps_document_view_popup_cmd_annot_properties_response_cb (AdwAlertDialog *dialog,
                                                          gchar *response,
                                                          PpsDocumentView *window)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	g_autoptr (GdkRGBA) rgba = NULL;
	gdouble opacity;
	g_autofree gchar *author = NULL;
	gboolean popup_is_open;
	PpsAnnotation *annot = priv->annot;
	PpsAnnotationsSaveMask mask = PPS_ANNOTATIONS_SAVE_NONE;

	if (!g_str_equal (response, "apply")) {
		return;
	}

	g_object_get (dialog, "author", &author,
	              "rgba", &rgba,
	              "opacity", &opacity,
	              "popup-open", &popup_is_open,
	              NULL);
	/* Set annotations changes */
	if (pps_annotation_markup_set_label (PPS_ANNOTATION_MARKUP (annot), author))
		mask |= PPS_ANNOTATIONS_SAVE_LABEL;

	if (pps_annotation_set_rgba (annot, rgba))
		mask |= PPS_ANNOTATIONS_SAVE_COLOR;

	if (pps_annotation_markup_set_opacity (PPS_ANNOTATION_MARKUP (annot), opacity))
		mask |= PPS_ANNOTATIONS_SAVE_OPACITY;

	if (pps_annotation_markup_set_popup_is_open (PPS_ANNOTATION_MARKUP (annot), popup_is_open))
		mask |= PPS_ANNOTATIONS_SAVE_POPUP_IS_OPEN;

	if (PPS_IS_ANNOTATION_TEXT (annot)) {
		PpsAnnotationTextIcon icon;

		g_object_get (dialog, "text-icon", &icon, NULL);
		if (pps_annotation_text_set_icon (PPS_ANNOTATION_TEXT (annot), icon))
			mask |= PPS_ANNOTATIONS_SAVE_TEXT_ICON;
	}

	if (PPS_IS_ANNOTATION_TEXT_MARKUP (annot)) {
		PpsAnnotationTextMarkupType markup_type;

		g_object_get (dialog, "markup-type", &markup_type, NULL);
		if (pps_annotation_text_markup_set_markup_type (PPS_ANNOTATION_TEXT_MARKUP (annot), markup_type))
			mask |= PPS_ANNOTATIONS_SAVE_TEXT_MARKUP_TYPE;
	}

	if (mask != PPS_ANNOTATIONS_SAVE_NONE) {
		pps_document_doc_mutex_lock (priv->document);
		pps_document_annotations_save_annotation (PPS_DOCUMENT_ANNOTATIONS (priv->document),
		                                          priv->annot,
		                                          mask);
		pps_document_doc_mutex_unlock (priv->document);

		/* FIXME: update annot region only */
		pps_view_reload (PPS_VIEW (priv->view));
	}
}

static void
pps_document_view_popup_cmd_annot_properties (GSimpleAction *action,
                                              GVariant *parameter,
                                              gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);
	AdwAlertDialog *dialog;
	PpsAnnotation *annot = priv->annot;

	if (!annot)
		return;

	dialog = g_object_new (g_type_from_name ("PpsAnnotationPropertiesDialog"),
	                       "annotation", priv->annot, NULL);

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (pps_document_view_popup_cmd_annot_properties_response_cb),
	                  window);
	adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (window));
}

static void
pps_document_view_popup_cmd_remove_annotation (GSimpleAction *action,
                                               GVariant *parameter,
                                               gpointer user_data)
{
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	pps_view_remove_annotation (PPS_VIEW (priv->view),
	                            priv->annot);
}

static void
pps_document_view_popup_cmd_open_attachment (GSimpleAction *action,
                                             GVariant *parameter,
                                             gpointer user_data)
{
	GdkDisplay *display;
	guint n_items, i;
	PpsDocumentView *window = user_data;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (window);

	if (!priv->attachments)
		return;

	display = gtk_widget_get_display (GTK_WIDGET (window));

	n_items = g_list_model_get_n_items (priv->attachments);

	for (i = 0; i < n_items; i++) {
		PpsAttachment *attachment;
		GError *error = NULL;
		GdkAppLaunchContext *context = gdk_display_get_app_launch_context (display);

		attachment = g_list_model_get_item (priv->attachments, i);

		pps_attachment_open (attachment, G_APP_LAUNCH_CONTEXT (context), &error);

		if (error) {
			pps_document_view_error_message (window, error,
			                                 "%s", _ ("Unable to open attachment"));
			g_error_free (error);
		}

		g_clear_object (&context);
	}
}

static void
attachments_save_response_cb (PpsAttachmentContext *attachment_context,
                              GAsyncResult *result,
                              PpsDocumentView *document_view)
{
	g_autoptr (GError) error = NULL;

	pps_attachment_context_save_attachments_finish (attachment_context, result, &error);

	if (error)
		pps_document_view_error_message (document_view, error,
		                                 "%s", _ ("The attachment could not be saved."));
}

static void
pps_document_view_popup_cmd_save_attachment_as (GSimpleAction *action,
                                                GVariant *parameter,
                                                gpointer document_view_pointer)
{
	PpsDocumentView *document_view = PPS_DOCUMENT_VIEW (document_view_pointer);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (document_view);

	if (!priv->attachments)
		return;

	pps_document_view_reset_progress_cancellable (document_view);

	pps_attachment_context_save_attachments_async (priv->attachment_context,
	                                               g_object_ref (priv->attachments),
	                                               GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (document_view))),
	                                               priv->progress_cancellable,
	                                               (GAsyncReadyCallback) attachments_save_response_cb,
	                                               document_view);
}

static void
view_details_cb (AdwBanner *self,
                 gpointer user_data)
{
	PpsDocumentView *pps_doc_view = PPS_DOCUMENT_VIEW (user_data);
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_signature_details_show (priv->document, GTK_WINDOW (pps_doc_view));
}

static void
pps_document_view_init (PpsDocumentView *pps_doc_view)
{
	guint page_cache_mb;
	gboolean allow_links_change_zoom;
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	/* for drop target support */
	g_type_ensure (GDK_TYPE_FILE_LIST);

	g_type_ensure (PPS_TYPE_VIEW);
	g_type_ensure (PPS_TYPE_FIND_SIDEBAR);
	g_type_ensure (PPS_TYPE_SIDEBAR_BOOKMARKS);
	gtk_widget_init_template (GTK_WIDGET (pps_doc_view));

	priv->sidebar_was_open_before_find = TRUE;

	priv->history = pps_history_new (priv->model);

	g_signal_connect (priv->history, "activate-link",
	                  G_CALLBACK (activate_link_cb),
	                  pps_doc_view);
	g_signal_connect (priv->history, "changed",
	                  G_CALLBACK (history_changed_cb),
	                  pps_doc_view);

	priv->document_action_group = g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP (priv->document_action_group),
	                                 actions,
	                                 G_N_ELEMENTS (actions),
	                                 pps_doc_view);
	gtk_widget_insert_action_group (GTK_WIDGET (pps_doc_view),
	                                "doc", G_ACTION_GROUP (priv->document_action_group));
	// These are only enabled once the user takes action
	pps_document_view_set_action_enabled (pps_doc_view, "find-next", FALSE);
	pps_document_view_set_action_enabled (pps_doc_view, "find-previous", FALSE);
	pps_document_view_set_action_enabled (pps_doc_view, "add-highlight-annotation", FALSE);
	pps_document_view_set_action_enabled (pps_doc_view, "copy", FALSE);

	page_cache_mb = g_settings_get_uint (priv->settings,
	                                     GS_PAGE_CACHE_SIZE);
	pps_view_set_page_cache_size (PPS_VIEW (priv->view),
	                              (gsize) page_cache_mb * 1024 * 1024);
	allow_links_change_zoom = g_settings_get_boolean (priv->settings,
	                                                  GS_ALLOW_LINKS_CHANGE_ZOOM);
	pps_view_set_allow_links_change_zoom (PPS_VIEW (priv->view),
	                                      allow_links_change_zoom);
	pps_view_set_model (PPS_VIEW (priv->view), priv->model);

	g_settings_delay (priv->default_settings);
	pps_document_view_setup_default (pps_doc_view);

	sizing_mode_changed_cb (priv->model, NULL, pps_doc_view);

	g_object_bind_property (g_action_map_lookup_action (G_ACTION_MAP (priv->document_action_group), "find"), "enabled",
	                        priv->find_sidebar, "visible",
	                        G_BINDING_SYNC_CREATE);

	priv->search_context = pps_search_context_new (priv->model);

	pps_find_sidebar_set_search_context (PPS_FIND_SIDEBAR (priv->find_sidebar), priv->search_context);
	pps_view_set_search_context (PPS_VIEW (priv->view), priv->search_context);

	g_signal_connect_object (priv->search_context, "result-activated",
	                         G_CALLBACK (sidebar_navigate_to_view),
	                         pps_doc_view, G_CONNECT_SWAPPED);

	pps_document_view_setup_lockdown (pps_doc_view);
}

static void
pps_document_view_class_init (PpsDocumentViewClass *pps_document_view_class)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (pps_document_view_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (pps_document_view_class);

	g_object_class->dispose = pps_document_view_dispose;

	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/papers/ui/document-view.ui");
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, document_toolbar_view);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, split_view);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, scrolled_window);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, toast_overlay);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, error_alert);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, print_cancel_alert);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, rect_small_alert);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, banner);

	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, model);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, view);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, signature_banner);

	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, page_selector);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, header_bar);

	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, default_settings);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, settings);

	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, zoom_fit_best_revealer);

	/* sidebar */
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar_stack);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar_links);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar_annots);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, attachment_context);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar_bookmarks);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, sidebar_layers);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, find_sidebar);

	/* menu button */
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, action_menu_button);

	/* popup menu */
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, view_popup);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, attachment_popup);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, annot_menu);
	gtk_widget_class_bind_template_child_private (widget_class, PpsDocumentView, annot_menu_child);

	/* bind signal callback */
	gtk_widget_class_bind_template_callback (widget_class, activate_link_cb);
	gtk_widget_class_bind_template_callback (widget_class, sidebar_visibility_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, sidebar_collapsed_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, pps_document_view_button_pressed);
	gtk_widget_class_bind_template_callback (widget_class, scroll_child_history_cb);
	gtk_widget_class_bind_template_callback (widget_class, print_jobs_confirmation_dialog_response);
	gtk_widget_class_bind_template_callback (widget_class, view_details_cb);
	gtk_widget_class_bind_template_callback (widget_class, pps_document_view_on_signature_rect_too_small_response);

	gtk_widget_class_bind_template_callback (widget_class, view_external_link_cb);
	gtk_widget_class_bind_template_callback (widget_class, view_handle_link_cb);
	gtk_widget_class_bind_template_callback (widget_class, view_menu_popup_cb);
	gtk_widget_class_bind_template_callback (widget_class, view_selection_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, scroll_history_cb);
	gtk_widget_class_bind_template_callback (widget_class, view_annot_removed);
	gtk_widget_class_bind_template_callback (widget_class, view_layers_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, view_caret_cursor_moved_cb);
	gtk_widget_class_bind_template_callback (widget_class, pps_document_view_on_signature_rect);

	/* model */
	gtk_widget_class_bind_template_callback (widget_class, page_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, zoom_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, sizing_mode_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, rotation_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, continuous_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, page_layout_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, dual_mode_odd_pages_left_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, direction_changed_cb);
	gtk_widget_class_bind_template_callback (widget_class, inverted_colors_changed_cb);

	/* sidebar */
	gtk_widget_class_bind_template_callback (widget_class, sidebar_annots_annot_activated_cb);
	gtk_widget_class_bind_template_callback (widget_class, attachment_bar_menu_popup_cb);
	gtk_widget_class_bind_template_callback (widget_class, sidebar_layers_visibility_changed);
	gtk_widget_class_bind_template_callback (widget_class, bookmark_activated_cb);
	gtk_widget_class_bind_template_callback (widget_class, sidebar_navigate_to_view);

	/* settings */
	gtk_widget_class_bind_template_callback (widget_class, doc_restrictions_changed);
	gtk_widget_class_bind_template_callback (widget_class, page_cache_size_changed);
	gtk_widget_class_bind_template_callback (widget_class, allow_links_change_zoom_changed);
}

/**
 * pps_document_view_new:
 *
 * Creates a #GtkWidget that represents the window.
 *
 * Returns: the #GtkWidget that represents the window.
 */
PpsDocumentView *
pps_document_view_new (void)
{
	return g_object_new (PPS_TYPE_DOCUMENT_VIEW, NULL);
}

/**
 * pps_document_view_get_metadata:
 * @pps_doc_view: the #PpsDocumentView
 *
 * Returns: (transfer none): the #PpsMetadata of the document shown in the window.
 */
PpsMetadata *
pps_document_view_get_metadata (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_return_val_if_fail (PPS_DOCUMENT_VIEW (pps_doc_view), NULL);

	return priv->metadata;
}

void
pps_document_view_reload_document (PpsDocumentView *pps_doc_view, PpsDocument *document)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	pps_document_view_set_document (pps_doc_view, document);

	/* Restart the search after reloading */
	if (gtk_stack_get_visible_child (GTK_STACK (priv->sidebar_stack)) == priv->find_sidebar)
		pps_search_context_restart (priv->search_context);
}

gboolean
pps_document_view_close_handled (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	g_clear_signal_handler (&priv->modified_handler_id, priv->document);

	if (pps_document_view_check_document_modified (pps_doc_view))
		return TRUE;

	if (pps_document_view_check_print_queue (pps_doc_view))
		return TRUE;

	pps_document_view_save_settings (pps_doc_view);

	return FALSE;
}

void
pps_document_view_set_filenames (PpsDocumentView *pps_doc_view,
                                 const gchar *display_name,
                                 const gchar *edit_name)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	g_clear_pointer (&priv->display_name, g_free);
	g_clear_pointer (&priv->edit_name, g_free);

	priv->display_name = g_strdup (display_name);
	priv->edit_name = g_strdup (edit_name);

	pps_document_view_update_title (pps_doc_view);
}

/**
 * pps_document_view_get_model:
 * @pps_doc_view: the #PpsDocumentView
 *
 * Returns: (transfer none): the #PpsDocumentModel
 */
PpsDocumentModel *
pps_document_view_get_model (PpsDocumentView *pps_doc_view)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_return_val_if_fail (PPS_DOCUMENT_VIEW (pps_doc_view), NULL);

	return priv->model;
}

void
pps_document_view_set_fullscreen_mode (PpsDocumentView *pps_doc_view,
                                       gboolean fullscreened)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);

	adw_header_bar_set_show_end_title_buttons (ADW_HEADER_BAR (priv->header_bar), !fullscreened);
}

void
pps_document_view_set_inverted_colors (PpsDocumentView *pps_doc_view,
                                       gboolean inverted)
{
	PpsDocumentViewPrivate *priv = GET_PRIVATE (pps_doc_view);
	g_return_if_fail (PPS_DOCUMENT_VIEW (pps_doc_view));

	pps_document_model_set_inverted_colors (priv->model, inverted);
}
