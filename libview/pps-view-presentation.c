// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-view-presentation.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2010 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include "config.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <stdlib.h>

#include "pps-job-scheduler.h"
#include "pps-jobs.h"
#include "pps-page-cache.h"
#include "pps-view-cursor.h"
#include "pps-view-presentation.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum {
	PROP_0,
	PROP_DOCUMENT,
	PROP_CURRENT_PAGE,
	PROP_ROTATION,
	PROP_INVERTED_COLORS
};

enum {
	CHANGE_PAGE,
	FINISHED,
	SIGNAL_EXTERNAL_LINK,
	N_SIGNALS
};

typedef enum {
	PPS_PRESENTATION_NORMAL,
	PPS_PRESENTATION_BLACK,
	PPS_PRESENTATION_WHITE,
	PPS_PRESENTATION_END
} PpsPresentationState;

struct _PpsViewPresentationPrivate {
	guint is_constructing : 1;

	gint64 start_time;
	gdouble transition_time;
	gint animation_tick_id;
	gint inhibit_id;

	guint current_page;
	guint previous_page;
	GdkTexture *current_texture;
	GdkTexture *previous_texture;
	PpsDocument *document;
	guint rotation;
	gboolean inverted_colors;
	PpsPresentationState state;

	/* Cursors */
	PpsViewCursor cursor;
	guint hide_cursor_timeout_id;

	/* Goto Window */
	GtkWidget *goto_popup;
	GtkWidget *goto_entry;

	/* Page Transition */
	guint trans_timeout_id;

	/* Links */
	PpsPageCache *page_cache;

	PpsJob *prev_job;
	PpsJob *curr_job;
	PpsJob *next_job;
};

typedef struct _PpsViewPresentationPrivate PpsViewPresentationPrivate;

#define GET_PRIVATE(o) pps_view_presentation_get_instance_private (o)

struct _PpsViewPresentationClass {
	GtkWidgetClass base_class;

	/* signals */
	void (*change_page) (PpsViewPresentation *pview,
	                     GtkScrollType scroll);
	void (*finished) (PpsViewPresentation *pview);
	void (*external_link) (PpsViewPresentation *pview,
	                       PpsLinkAction *action);
};

static guint signals[N_SIGNALS] = { 0 };

static void pps_view_presentation_set_cursor_for_location (PpsViewPresentation *pview,
                                                           gdouble x,
                                                           gdouble y);

static void pps_view_presentation_update_current_texture (PpsViewPresentation *pview,
                                                          GdkTexture *surface);

#define HIDE_CURSOR_TIMEOUT 5000

G_DEFINE_TYPE_WITH_PRIVATE (PpsViewPresentation, pps_view_presentation, GTK_TYPE_WIDGET)

static void
pps_view_presentation_set_normal_or_end (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);

	if (priv->state == PPS_PRESENTATION_NORMAL)
		return;

	if (priv->state == PPS_PRESENTATION_END)
		return;

	if (priv->current_page + 1 == pps_document_get_n_pages (priv->document))
		priv->state = PPS_PRESENTATION_END;
	else
		priv->state = PPS_PRESENTATION_NORMAL;

	gtk_widget_remove_css_class (widget, "white-mode");
	gtk_widget_queue_draw (widget);
}

static void
pps_view_presentation_set_black (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);

	if (priv->state == PPS_PRESENTATION_BLACK)
		return;

	priv->state = PPS_PRESENTATION_BLACK;

	gtk_widget_remove_css_class (widget, "white-mode");
	gtk_widget_queue_draw (widget);
}

static void
pps_view_presentation_set_white (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);

	if (priv->state == PPS_PRESENTATION_WHITE)
		return;

	priv->state = PPS_PRESENTATION_WHITE;

	gtk_widget_add_css_class (widget, "white-mode");
}

static void
pps_view_presentation_set_end (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);

	if (priv->state == PPS_PRESENTATION_END)
		return;

	priv->state = PPS_PRESENTATION_END;
	gtk_widget_queue_draw (widget);
}

static void
pps_view_presentation_get_view_size (PpsViewPresentation *pview,
                                     guint page,
                                     int *view_width,
                                     int *view_height)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	gdouble width, height;
	int widget_width, widget_height;

	pps_document_get_page_size (priv->document, page, &width, &height);
	if (priv->rotation == 90 || priv->rotation == 270) {
		gdouble tmp;

		tmp = width;
		width = height;
		height = tmp;
	}

	widget_width = gtk_widget_get_width (GTK_WIDGET (pview));
	widget_height = gtk_widget_get_height (GTK_WIDGET (pview));

	if (widget_width / width < widget_height / height) {
		*view_width = widget_width;
		*view_height = (int) ((widget_width / width) * height + 0.5);
	} else {
		*view_width = (int) ((widget_height / height) * width + 0.5);
		*view_height = widget_height;
	}
}

static void
pps_view_presentation_get_page_area (PpsViewPresentation *pview,
                                     GdkRectangle *area)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);
	gint view_width, view_height, widget_width, widget_height;

	pps_view_presentation_get_view_size (pview, priv->current_page,
	                                     &view_width, &view_height);

	widget_width = gtk_widget_get_width (widget);
	widget_height = gtk_widget_get_height (widget);

	area->x = (MAX (0, widget_width - view_width)) / 2;
	area->y = (MAX (0, widget_height - view_height)) / 2;
	area->width = view_width;
	area->height = view_height;
}

/* Page Transition */
static void
transition_next_page (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	priv->trans_timeout_id = 0;
	pps_view_presentation_next_page (pview);
}

static void
pps_view_presentation_transition_stop (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	g_clear_handle_id (&priv->trans_timeout_id, g_source_remove);
}

static void
pps_view_presentation_transition_start (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	gdouble duration;

	if (!PPS_IS_DOCUMENT_TRANSITION (priv->document))
		return;

	pps_view_presentation_transition_stop (pview);

	duration = pps_document_transition_get_page_duration (PPS_DOCUMENT_TRANSITION (priv->document),
	                                                      priv->current_page);
	if (duration >= 0) {
		priv->trans_timeout_id =
		    g_timeout_add_once (duration * 1000,
		                        (GSourceOnceFunc) transition_next_page,
		                        pview);
	}
}

static gboolean
animation_tick_cb (GtkWidget *widget,
                   GdkFrameClock *clock,
                   gpointer unused)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	gint64 frame_time = gdk_frame_clock_get_frame_time (clock);
	PpsTransitionEffect *effect;
	gdouble duration = 0;

	if (!PPS_IS_DOCUMENT_TRANSITION (priv->document))
		return G_SOURCE_REMOVE;

	if (priv->start_time == 0)
		priv->start_time = frame_time;

	priv->transition_time = (frame_time - priv->start_time) / (float) G_USEC_PER_SEC;

	gtk_widget_queue_draw (widget);

	effect = pps_document_transition_get_effect (
	    PPS_DOCUMENT_TRANSITION (priv->document),
	    priv->current_page);
	g_object_get (effect, "duration-real", &duration, NULL);

	if (priv->transition_time >= duration) {
		pps_view_presentation_transition_start (pview);
		return G_SOURCE_REMOVE;
	} else {
		return G_SOURCE_CONTINUE;
	}
}

static void
pps_view_presentation_animation_cancel (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (priv->animation_tick_id) {
		gtk_widget_remove_tick_callback (GTK_WIDGET (pview), priv->animation_tick_id);
		priv->animation_tick_id = 0;
	}
}

static void
pps_view_presentation_animation_start (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWidget *widget = GTK_WIDGET (pview);

	priv->start_time = 0;
	priv->animation_tick_id = gtk_widget_add_tick_callback (widget,
	                                                        animation_tick_cb, pview, NULL);
	gtk_widget_queue_draw (widget);
}

static GdkTexture *
get_texture_from_job (PpsViewPresentation *pview,
                      PpsJob *job)
{
	if (!job)
		return NULL;

	return PPS_JOB_RENDER_TEXTURE (job)->texture;
}

/* Page Navigation */
static void
job_finished_cb (PpsJob *job,
                 PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (job != priv->curr_job)
		return;

	pps_view_presentation_update_current_texture (pview,
	                                              get_texture_from_job (pview, job));

	pps_view_presentation_animation_start (pview);
}

static PpsJob *
pps_view_presentation_schedule_new_job (PpsViewPresentation *pview,
                                        gint page,
                                        PpsJobPriority priority)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	GtkWindow *window;
	PpsJob *job;
	int view_width, view_height;

	// Do not render if the window is not yet fullscreened
	window = GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pview)));

	if (page < 0 || page >= pps_document_get_n_pages (priv->document) || !gtk_window_is_fullscreen (window))
		return NULL;

	pps_view_presentation_get_view_size (pview, page, &view_width, &view_height);
	gdouble device_scale = gdk_surface_get_scale (gtk_native_get_surface (gtk_widget_get_native (GTK_WIDGET (pview))));
	view_width = (gint) view_width * device_scale;
	view_height = (gint) view_height * device_scale;
	job = pps_job_render_texture_new (priv->document, page, priv->rotation, 0.,
	                                  view_width, view_height, PPS_RENDER_ANNOTS_ALL);
	g_signal_connect (job, "finished",
	                  G_CALLBACK (job_finished_cb),
	                  pview);
	pps_job_scheduler_push_job (job, priority);

	return job;
}

static void
pps_view_presentation_clear_job (PpsViewPresentation *pview,
                                 PpsJob **job)
{
	if (!*job)
		return;

	g_signal_handlers_disconnect_by_func (*job, job_finished_cb, pview);
	pps_job_cancel (*job);
	g_clear_object (job);
}

static void
pps_view_presentation_reset_jobs (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	pps_view_presentation_clear_job (pview, &priv->curr_job);
	pps_view_presentation_clear_job (pview, &priv->prev_job);
	pps_view_presentation_clear_job (pview, &priv->next_job);
}

static void
pps_view_presentation_update_current_texture (PpsViewPresentation *pview,
                                              GdkTexture *texture)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (!texture || priv->current_texture == texture)
		return;

	g_set_object (&priv->previous_texture, priv->current_texture);
	g_set_object (&priv->current_texture, texture);
}

static void
pps_view_presentation_update_current_page (PpsViewPresentation *pview,
                                           guint page)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	gint jump;

	if (!priv->document)
		return;

	if (page < 0 || page >= pps_document_get_n_pages (priv->document))
		return;

	pps_view_presentation_animation_cancel (pview);
	pps_view_presentation_transition_stop (pview);

	jump = page - priv->current_page;

	switch (jump) {
	case 0:
		if (!priv->curr_job)
			priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		if (!priv->next_job)
			priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_HIGH);
		if (!priv->prev_job)
			priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_LOW);
		break;
	case -1:
		pps_view_presentation_clear_job (pview, &priv->next_job);
		priv->next_job = priv->curr_job;
		priv->curr_job = priv->prev_job;

		if (!priv->curr_job)
			priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		else
			pps_job_scheduler_update_job (priv->curr_job, PPS_JOB_PRIORITY_URGENT);
		priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_HIGH);
		pps_job_scheduler_update_job (priv->next_job, PPS_JOB_PRIORITY_LOW);

		break;
	case 1:
		pps_view_presentation_clear_job (pview, &priv->prev_job);
		priv->prev_job = priv->curr_job;
		priv->curr_job = priv->next_job;

		if (!priv->curr_job)
			priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		else
			pps_job_scheduler_update_job (priv->curr_job, PPS_JOB_PRIORITY_URGENT);
		priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_HIGH);

		if (priv->prev_job)
			pps_job_scheduler_update_job (priv->prev_job, PPS_JOB_PRIORITY_LOW);

		break;
	case -2:
		pps_view_presentation_clear_job (pview, &priv->next_job);
		pps_view_presentation_clear_job (pview, &priv->curr_job);
		priv->next_job = priv->prev_job;

		priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_HIGH);
		if (!priv->next_job)
			priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_LOW);
		else
			pps_job_scheduler_update_job (priv->next_job, PPS_JOB_PRIORITY_LOW);
		break;
	case 2:
		pps_view_presentation_clear_job (pview, &priv->prev_job);
		pps_view_presentation_clear_job (pview, &priv->curr_job);
		priv->prev_job = priv->next_job;

		priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_HIGH);
		if (!priv->prev_job)
			priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_LOW);
		else
			pps_job_scheduler_update_job (priv->prev_job, PPS_JOB_PRIORITY_LOW);
		break;
	default:
		pps_view_presentation_clear_job (pview, &priv->prev_job);
		pps_view_presentation_clear_job (pview, &priv->curr_job);
		pps_view_presentation_clear_job (pview, &priv->next_job);

		priv->curr_job = pps_view_presentation_schedule_new_job (pview, page, PPS_JOB_PRIORITY_URGENT);
		if (jump > 0) {
			priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_HIGH);
			priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_LOW);
		} else {
			priv->prev_job = pps_view_presentation_schedule_new_job (pview, page - 1, PPS_JOB_PRIORITY_HIGH);
			priv->next_job = pps_view_presentation_schedule_new_job (pview, page + 1, PPS_JOB_PRIORITY_LOW);
		}
	}

	if (priv->current_page != page) {
		priv->previous_page = priv->current_page;
		priv->current_page = page;
		g_object_notify (G_OBJECT (pview), "current-page");
	}

	if (priv->page_cache)
		pps_page_cache_set_page_range (priv->page_cache, page, page);

	if (priv->cursor != PPS_VIEW_CURSOR_HIDDEN) {
		gint x, y;

		pps_document_misc_get_pointer_position (GTK_WIDGET (pview), &x, &y);
		pps_view_presentation_set_cursor_for_location (pview, x, y);
	}

	if (priv->curr_job && PPS_JOB_RENDER_TEXTURE (priv->curr_job)->texture) {
		pps_view_presentation_update_current_texture (pview,
		                                              PPS_JOB_RENDER_TEXTURE (priv->curr_job)->texture);

		pps_view_presentation_animation_start (pview);
	}
}

static void
pps_view_presentation_set_current_page (PpsViewPresentation *pview,
                                        guint page)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (priv->current_page == page)
		return;

	if (!gtk_widget_get_realized (GTK_WIDGET (pview))) {
		priv->current_page = page;
		g_object_notify (G_OBJECT (pview), "current-page");
	} else {
		pps_view_presentation_update_current_page (pview, page);
	}
}

void
pps_view_presentation_next_page (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	guint n_pages;
	gint new_page;

	switch (priv->state) {
	case PPS_PRESENTATION_BLACK:
	case PPS_PRESENTATION_WHITE:
		pps_view_presentation_set_normal_or_end (pview);
	case PPS_PRESENTATION_END:
		return;
	case PPS_PRESENTATION_NORMAL:
		break;
	}

	n_pages = pps_document_get_n_pages (priv->document);
	new_page = priv->current_page + 1;

	if (new_page == n_pages)
		pps_view_presentation_set_end (pview);
	else
		pps_view_presentation_update_current_page (pview, new_page);
}

void
pps_view_presentation_previous_page (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	gint new_page = 0;

	switch (priv->state) {
	case PPS_PRESENTATION_BLACK:
	case PPS_PRESENTATION_WHITE:
		pps_view_presentation_set_normal_or_end (pview);
		return;
	case PPS_PRESENTATION_END:
		priv->state = PPS_PRESENTATION_NORMAL;
		new_page = priv->current_page;
		break;
	case PPS_PRESENTATION_NORMAL:
		new_page = priv->current_page - 1;
		break;
	}

	pps_view_presentation_update_current_page (pview, new_page);
}

/* Goto Window */
static int
key_to_digit (int keyval)
{
	if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9)
		return keyval - GDK_KEY_0;

	if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9)
		return keyval - GDK_KEY_KP_0;

	return -1;
}

static gboolean
key_is_numeric (int keyval)
{
	return key_to_digit (keyval) >= 0;
}

static void
pps_view_presentation_goto_entry_activate (GtkEntry *entry,
                                           PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	const gchar *text;
	gint page;

	text = gtk_editable_get_text (GTK_EDITABLE (entry));
	page = atoi (text) - 1;

	gtk_popover_popdown (GTK_POPOVER (priv->goto_popup));
	pps_view_presentation_update_current_page (pview, page);
}

/* Links */
static gboolean
pps_view_presentation_link_is_supported (PpsViewPresentation *pview,
                                         PpsLink *link)
{
	PpsLinkAction *action;

	action = pps_link_get_action (link);
	if (!action)
		return FALSE;

	switch (pps_link_action_get_action_type (action)) {
	case PPS_LINK_ACTION_TYPE_GOTO_DEST:
		return pps_link_action_get_dest (action) != NULL;
	case PPS_LINK_ACTION_TYPE_NAMED:
	case PPS_LINK_ACTION_TYPE_GOTO_REMOTE:
	case PPS_LINK_ACTION_TYPE_EXTERNAL_URI:
	case PPS_LINK_ACTION_TYPE_LAUNCH:
		return TRUE;
	default:
		return FALSE;
	}

	return FALSE;
}

static PpsLink *
pps_view_presentation_get_link_at_location (PpsViewPresentation *pview,
                                            gdouble x,
                                            gdouble y)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GdkRectangle page_area;
	PpsMappingList *link_mapping;
	PpsLink *link;
	gdouble width, height;
	PpsPoint point;

	if (!priv->page_cache)
		return NULL;

	pps_document_get_page_size (priv->document, priv->current_page, &width, &height);
	pps_view_presentation_get_page_area (pview, &page_area);
	x = (x - page_area.x) / page_area.width;
	y = (y - page_area.y) / page_area.height;
	switch (priv->rotation) {
	case 0:
	case 360:
		point.x = width * x;
		point.y = height * y;
		break;
	case 90:
		point.x = width * y;
		point.y = height * (1 - x);
		break;
	case 180:
		point.x = width * (1 - x);
		point.y = height * (1 - y);
		break;
	case 270:
		point.x = width * (1 - y);
		point.y = height * x;
		break;
	default:
		g_assert_not_reached ();
	}

	link_mapping = pps_page_cache_get_link_mapping (priv->page_cache, priv->current_page);

	link = link_mapping ? pps_mapping_list_get_data (link_mapping, &point) : NULL;

	return link && pps_view_presentation_link_is_supported (pview, link) ? link : NULL;
}

static void
pps_view_presentation_handle_link (PpsViewPresentation *pview,
                                   PpsLink *link)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	PpsLinkAction *action;

	action = pps_link_get_action (link);

	switch (pps_link_action_get_action_type (action)) {
	case PPS_LINK_ACTION_TYPE_NAMED: {
		const gchar *name = pps_link_action_get_name (action);

		if (g_ascii_strcasecmp (name, "FirstPage") == 0) {
			pps_view_presentation_update_current_page (pview, 0);
		} else if (g_ascii_strcasecmp (name, "PrevPage") == 0) {
			pps_view_presentation_update_current_page (pview, priv->current_page - 1);
		} else if (g_ascii_strcasecmp (name, "NextPage") == 0) {
			pps_view_presentation_update_current_page (pview, priv->current_page + 1);
		} else if (g_ascii_strcasecmp (name, "LastPage") == 0) {
			gint n_pages;

			n_pages = pps_document_get_n_pages (priv->document);
			pps_view_presentation_update_current_page (pview, n_pages - 1);
		}
	} break;

	case PPS_LINK_ACTION_TYPE_GOTO_DEST: {
		PpsLinkDest *dest;
		gint page;

		dest = pps_link_action_get_dest (action);
		page = pps_document_links_get_dest_page (PPS_DOCUMENT_LINKS (priv->document), dest);
		pps_view_presentation_update_current_page (pview, page);
	} break;
	case PPS_LINK_ACTION_TYPE_GOTO_REMOTE:
	case PPS_LINK_ACTION_TYPE_EXTERNAL_URI:
	case PPS_LINK_ACTION_TYPE_LAUNCH:
		g_signal_emit (pview, signals[SIGNAL_EXTERNAL_LINK], 0, action);
		break;
	default:
		break;
	}
}

/* Cursors */
static void
pps_view_presentation_set_cursor (PpsViewPresentation *pview,
                                  PpsViewCursor view_cursor)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GtkWidget *widget = GTK_WIDGET (pview);

	if (priv->cursor == view_cursor)
		return;

	if (!gtk_widget_get_realized (widget))
		gtk_widget_realize (widget);

	priv->cursor = view_cursor;

	gtk_widget_set_cursor_from_name (widget,
	                                 pps_view_cursor_name (view_cursor));
}

static void
pps_view_presentation_set_cursor_for_location (PpsViewPresentation *pview,
                                               gdouble x,
                                               gdouble y)
{
	if (pps_view_presentation_get_link_at_location (pview, x, y))
		pps_view_presentation_set_cursor (pview, PPS_VIEW_CURSOR_LINK);
	else
		pps_view_presentation_set_cursor (pview, PPS_VIEW_CURSOR_NORMAL);
}

static void
hide_cursor_timeout_cb (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	pps_view_presentation_set_cursor (pview, PPS_VIEW_CURSOR_HIDDEN);
	priv->hide_cursor_timeout_id = 0;
}

static void
pps_view_presentation_hide_cursor_timeout_stop (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	g_clear_handle_id (&priv->hide_cursor_timeout_id, g_source_remove);
}

static void
pps_view_presentation_hide_cursor_timeout_start (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	pps_view_presentation_hide_cursor_timeout_stop (pview);
	priv->hide_cursor_timeout_id =
	    g_timeout_add_once (HIDE_CURSOR_TIMEOUT,
	                        (GSourceOnceFunc) hide_cursor_timeout_cb,
	                        pview);
}

static void
pps_view_presentation_inhibit_screenlock (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GtkWindow *window = GTK_WINDOW (gtk_widget_get_native (GTK_WIDGET (pview)));

	if (priv->inhibit_id != 0)
		return;

	priv->inhibit_id = gtk_application_inhibit (GTK_APPLICATION (g_application_get_default ()),
	                                            window,
	                                            GTK_APPLICATION_INHIBIT_IDLE,
	                                            _ ("Running in presentation mode"));
}

static void
pps_view_presentation_uninhibit_screenlock (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (priv->inhibit_id == 0)
		return;

	gtk_application_uninhibit (GTK_APPLICATION (g_application_get_default ()),
	                           priv->inhibit_id);
	priv->inhibit_id = 0;
}

static void
pps_view_presentation_dispose (GObject *object)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (object);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	g_clear_object (&priv->document);

	pps_view_presentation_transition_stop (pview);
	pps_view_presentation_hide_cursor_timeout_stop (pview);
	pps_view_presentation_reset_jobs (pview);

	g_clear_object (&priv->current_texture);
	g_clear_object (&priv->page_cache);
	g_clear_object (&priv->previous_texture);

	g_clear_pointer (&priv->goto_popup, gtk_widget_unparent);

	g_clear_handle_id (&priv->trans_timeout_id, g_source_remove);
	g_clear_handle_id (&priv->hide_cursor_timeout_id, g_source_remove);

	pps_view_presentation_uninhibit_screenlock (pview);

	G_OBJECT_CLASS (pps_view_presentation_parent_class)->dispose (object);
}

static void
pps_view_presentation_snapshot_end_page (PpsViewPresentation *pview, GtkSnapshot *snapshot)
{
	GtkWidget *widget = GTK_WIDGET (pview);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	PangoLayout *layout;
	PangoFontDescription *font_desc;
	gchar *markup;
	int text_width, text_height, x_center;
	const gchar *text = _ ("End of presentation. Press Esc or click to exit.");

	if (priv->state != PPS_PRESENTATION_END)
		return;

	layout = gtk_widget_create_pango_layout (widget, NULL);
	markup = g_strdup_printf ("<span foreground=\"white\">%s</span>", text);
	pango_layout_set_markup (layout, markup, -1);
	g_free (markup);

	font_desc = pango_font_description_new ();
	pango_font_description_set_size (font_desc, 16 * PANGO_SCALE);
	pango_layout_set_font_description (layout, font_desc);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	x_center = gtk_widget_get_width (widget) / 2 - text_width / 2;

	gtk_snapshot_render_layout (snapshot, gtk_widget_get_style_context (widget),
	                            x_center, 15, layout);

	pango_font_description_free (font_desc);
	g_object_unref (layout);
}

static GskGLShader *
pps_view_presentation_load_shader (PpsTransitionEffectType type)
{
	static const char *shader_resources[] = {
		[PPS_TRANSITION_EFFECT_REPLACE] = NULL,
		[PPS_TRANSITION_EFFECT_SPLIT] = "/org/gnome/papers/shader/split.glsl",
		[PPS_TRANSITION_EFFECT_WIPE] = "/org/gnome/papers/shader/wipe.glsl",
		[PPS_TRANSITION_EFFECT_COVER] = "/org/gnome/papers/shader/cover.glsl",
		[PPS_TRANSITION_EFFECT_UNCOVER] = "/org/gnome/papers/shader/uncover.glsl",
		[PPS_TRANSITION_EFFECT_DISSOLVE] = "/org/gnome/papers/shader/dissolve.glsl",
		[PPS_TRANSITION_EFFECT_PUSH] = "/org/gnome/papers/shader/push.glsl",
		[PPS_TRANSITION_EFFECT_BOX] = "/org/gnome/papers/shader/box.glsl",
		[PPS_TRANSITION_EFFECT_BLINDS] = "/org/gnome/papers/shader/blinds.glsl",
		[PPS_TRANSITION_EFFECT_FLY] = "/org/gnome/papers/shader/fly.glsl",
		[PPS_TRANSITION_EFFECT_GLITTER] = "/org/gnome/papers/shader/glitter.glsl",
		[PPS_TRANSITION_EFFECT_FADE] = "/org/gnome/papers/shader/fade.glsl",
	};

	static GskGLShader *shaders[G_N_ELEMENTS (shader_resources)] = {};

	if (type >= G_N_ELEMENTS (shader_resources) || !shader_resources[type])
		return NULL;

	if (!shaders[type]) {
		shaders[type] = gsk_gl_shader_new_from_resource (shader_resources[type]);
		g_assert (shaders[type] != NULL);
	}

	return shaders[type];
}

static GBytes *
pps_view_presentation_build_shader_args (GskGLShader *shader,
                                         PpsTransitionEffect *effect,
                                         gdouble progress)
{
	GskShaderArgsBuilder *builder = gsk_shader_args_builder_new (shader, NULL);
	PpsTransitionEffectType type;
	PpsTransitionEffectAlignment alignment;
	PpsTransitionEffectDirection direction;
	gint angle;
	gdouble scale;

	g_object_get (effect, "type", &type, NULL);

	switch (type) {
	case PPS_TRANSITION_EFFECT_SPLIT:
		g_object_get (effect, "direction", &direction,
		              "alignment", &alignment, NULL);
		gsk_shader_args_builder_set_int (builder, 1, direction);
		gsk_shader_args_builder_set_int (builder, 2, alignment);
		break;
	case PPS_TRANSITION_EFFECT_WIPE:
	case PPS_TRANSITION_EFFECT_PUSH:
	case PPS_TRANSITION_EFFECT_COVER:
	case PPS_TRANSITION_EFFECT_UNCOVER:
	case PPS_TRANSITION_EFFECT_GLITTER:
		g_object_get (effect, "angle", &angle, NULL);
		gsk_shader_args_builder_set_int (builder, 1, angle);
		break;
	case PPS_TRANSITION_EFFECT_BOX:
		g_object_get (effect, "direction", &direction, NULL);
		gsk_shader_args_builder_set_int (builder, 1, direction);
		break;
	case PPS_TRANSITION_EFFECT_BLINDS:
		g_object_get (effect, "alignment", &alignment, NULL);
		gsk_shader_args_builder_set_int (builder, 1, alignment);
		break;
	case PPS_TRANSITION_EFFECT_FLY:
		g_object_get (effect, "angle", &angle,
		              "scale", &scale, NULL);
		gsk_shader_args_builder_set_int (builder, 1, angle);
		gsk_shader_args_builder_set_float (builder, 2, scale);
		break;
	default:
		g_assert_not_reached ();
	}

	gsk_shader_args_builder_set_float (builder, 0, progress);

	return gsk_shader_args_builder_free_to_args (builder);
}

static void
pps_view_presentation_animation_snapshot (PpsViewPresentation *pview,
                                          GtkSnapshot *snapshot,
                                          graphene_rect_t *area)
{
	GtkNative *native = gtk_widget_get_native (GTK_WIDGET (pview));
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GskRenderer *renderer = gtk_native_get_renderer (native);
	GskGLShader *shader;
	double duration;
	PpsTransitionEffectType type;
	GError *error = NULL;

	int width = gtk_widget_get_width (GTK_WIDGET (pview));
	int height = gtk_widget_get_height (GTK_WIDGET (pview));

	PpsTransitionEffect *effect = pps_document_transition_get_effect (
	    PPS_DOCUMENT_TRANSITION (priv->document),
	    priv->current_page);
	g_object_get (effect, "duration-real", &duration, "type", &type, NULL);

	shader = pps_view_presentation_load_shader (type);

	gdouble progress = priv->transition_time / duration;

	if (shader && gsk_gl_shader_compile (shader, renderer, &error)) {
		gtk_snapshot_push_gl_shader (snapshot, shader, &GRAPHENE_RECT_INIT (0, 0, width, height),
		                             pps_view_presentation_build_shader_args (shader, effect, progress));

		// TODO: handle different page size
		if (priv->previous_texture)
			gtk_snapshot_append_texture (snapshot, priv->previous_texture, area);
		else
			gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 0., 0., 0., 1. }, area);

		gtk_snapshot_gl_shader_pop_texture (snapshot); /* current child */

		gtk_snapshot_append_texture (snapshot, priv->current_texture, area);
		gtk_snapshot_gl_shader_pop_texture (snapshot); /* next child */
		gtk_snapshot_pop (snapshot);
	} else {
		if (error)
			g_warning ("failed to compile shader '%s'\n", error->message);
		else if (type != PPS_TRANSITION_EFFECT_REPLACE)
			g_warning ("shader for type %d is not implemented\n", type);

		gtk_snapshot_append_texture (snapshot, priv->current_texture, area);
	}

	g_clear_pointer (&error, g_error_free);
}

static void
pps_view_presentation_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GtkStyleContext *context = gtk_widget_get_style_context (widget);
	GtkSnapshot *document_snapshot = gtk_snapshot_new ();
	GskRenderNode *document_node;
	GdkRectangle page_area;
	graphene_rect_t area;

	gtk_snapshot_render_background (snapshot, context, 0, 0,
	                                gtk_widget_get_width (widget),
	                                gtk_widget_get_height (widget));

	switch (priv->state) {
	case PPS_PRESENTATION_END:
		pps_view_presentation_snapshot_end_page (pview, snapshot);
		return;
	case PPS_PRESENTATION_BLACK:
	case PPS_PRESENTATION_WHITE:
		return;
	case PPS_PRESENTATION_NORMAL:
		break;
	}

	if (!priv->curr_job) {
		pps_view_presentation_update_current_page (pview, priv->current_page);
		pps_view_presentation_hide_cursor_timeout_start (pview);
		return;
	}

	if (!priv->current_texture)
		return;

	pps_view_presentation_get_page_area (pview, &page_area);
	area = GRAPHENE_RECT_INIT (page_area.x, page_area.y,
	                           page_area.width, page_area.height);

	if (PPS_IS_DOCUMENT_TRANSITION (priv->document))
		pps_view_presentation_animation_snapshot (pview, document_snapshot, &area);
	else
		gtk_snapshot_append_texture (document_snapshot, priv->current_texture, &area);

	document_node = gtk_snapshot_free_to_node (document_snapshot);

	if (!document_node)
		return;

	if (priv->inverted_colors) {
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_COLOR);
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_DIFFERENCE);
		gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 1., 1., 1., 1. }, &area);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_append_node (snapshot, document_node);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_append_node (snapshot, document_node);
		gtk_snapshot_pop (snapshot);
	} else {
		gtk_snapshot_append_node (snapshot, document_node);
	}

	gsk_render_node_unref (document_node);
}

static gboolean
pps_view_presentation_key_press_event (GtkEventControllerKey *self,
                                       guint keyval,
                                       guint keycode,
                                       GdkModifierType state,
                                       GtkWidget *widget)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	switch (keyval) {
	case GDK_KEY_b:
	case GDK_KEY_B:
	case GDK_KEY_period:
	case GDK_KEY_KP_Decimal:
		if (priv->state == PPS_PRESENTATION_BLACK)
			pps_view_presentation_set_normal_or_end (pview);
		else
			pps_view_presentation_set_black (pview);

		return TRUE;
	case GDK_KEY_w:
	case GDK_KEY_W:
		if (priv->state == PPS_PRESENTATION_WHITE)
			pps_view_presentation_set_normal_or_end (pview);
		else
			pps_view_presentation_set_white (pview);

		return TRUE;
	case GDK_KEY_Home:
		if (priv->state == PPS_PRESENTATION_NORMAL) {
			pps_view_presentation_update_current_page (pview, 0);
			return TRUE;
		}
		break;
	case GDK_KEY_End:
		if (priv->state == PPS_PRESENTATION_NORMAL) {
			gint page;

			page = pps_document_get_n_pages (priv->document) - 1;
			pps_view_presentation_update_current_page (pview, page);

			return TRUE;
		}
		break;
	default:
		break;
	}

	pps_view_presentation_set_normal_or_end (pview);

	if (pps_document_get_n_pages (priv->document) > 1 && key_is_numeric (keyval)) {
		gint x, y;
		gchar *digit = g_strdup_printf ("%d", key_to_digit (keyval));

		pps_document_misc_get_pointer_position (GTK_WIDGET (pview), &x, &y);
		gtk_popover_set_pointing_to (GTK_POPOVER (priv->goto_popup),
		                             &(GdkRectangle) { x, y, 1, 1 });

		gtk_editable_set_text (GTK_EDITABLE (priv->goto_entry), digit);
		gtk_editable_set_position (GTK_EDITABLE (priv->goto_entry), -1);
		gtk_entry_grab_focus_without_selecting (GTK_ENTRY (priv->goto_entry));
		gtk_popover_popup (GTK_POPOVER (priv->goto_popup));
		g_free (digit);
		return TRUE;
	}

	return FALSE;
}

static void
pps_view_presentation_primary_button_released (GtkGestureClick *self,
                                               gint n_press,
                                               gdouble x,
                                               gdouble y,
                                               GtkWidget *widget)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	PpsLink *link;

	if (priv->state == PPS_PRESENTATION_END) {
		g_signal_emit (pview, signals[FINISHED], 0, NULL);
		return;
	}

	link = pps_view_presentation_get_link_at_location (pview, x, y);
	if (link)
		pps_view_presentation_handle_link (pview, link);
	else
		pps_view_presentation_next_page (pview);
}

static void
pps_view_presentation_secondary_button_released (GtkGestureClick *self,
                                                 gint n_press,
                                                 gdouble x,
                                                 gdouble y,
                                                 GtkWidget *widget)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);

	pps_view_presentation_previous_page (pview);
}

static void
pps_view_presentation_motion_notify_event (GtkEventControllerMotion *self,
                                           gdouble x,
                                           gdouble y,
                                           GtkWidget *widget)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);

	pps_view_presentation_hide_cursor_timeout_start (pview);
	pps_view_presentation_set_cursor_for_location (pview, x, y);
}

static void
pps_view_presentation_change_page (PpsViewPresentation *pview,
                                   GtkScrollType scroll)
{
	switch (scroll) {
	case GTK_SCROLL_PAGE_FORWARD:
		pps_view_presentation_next_page (pview);
		break;
	case GTK_SCROLL_PAGE_BACKWARD:
		pps_view_presentation_previous_page (pview);
		break;
	default:
		g_assert_not_reached ();
	}
}

static gboolean
pps_view_presentation_scroll_event (GtkEventControllerScroll *self,
                                    gdouble dx,
                                    gdouble dy,
                                    PpsViewPresentation *pview)
{
	GtkEventController *controller = GTK_EVENT_CONTROLLER (self);
	GdkEvent *event = gtk_event_controller_get_current_event (controller);
	guint state = gtk_event_controller_get_current_event_state (controller) & gtk_accelerator_get_default_mod_mask ();
	if (state != 0)
		return FALSE;

	switch (gdk_scroll_event_get_direction (event)) {
	case GDK_SCROLL_DOWN:
	case GDK_SCROLL_RIGHT:
		pps_view_presentation_change_page (pview, GTK_SCROLL_PAGE_FORWARD);
		break;
	case GDK_SCROLL_UP:
	case GDK_SCROLL_LEFT:
		pps_view_presentation_change_page (pview, GTK_SCROLL_PAGE_BACKWARD);
		break;
	case GDK_SCROLL_SMOOTH:
		return FALSE;
	}

	return TRUE;
}

static void
add_change_page_binding_keypad (GtkWidgetClass *widget_class,
                                guint keyval,
                                GdkModifierType modifiers,
                                GtkScrollType scroll)
{
	guint keypad_keyval = keyval - GDK_KEY_Left + GDK_KEY_KP_Left;

	gtk_widget_class_add_binding_signal (widget_class, keyval, modifiers,
	                                     "change_page", "(i)", scroll);
	gtk_widget_class_add_binding_signal (widget_class, keypad_keyval, modifiers,
	                                     "change_page", "(i)", scroll);
}

static void
pps_view_presentation_set_document (PpsViewPresentation *pview,
                                    PpsDocument *document)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (g_set_object (&priv->document, document)) {
		g_clear_object (&priv->page_cache);

		if (priv->document && PPS_IS_DOCUMENT_LINKS (priv->document)) {
			priv->page_cache = pps_page_cache_new (priv->document);
			pps_page_cache_set_flags (priv->page_cache, PPS_PAGE_DATA_INCLUDE_LINKS);
		}

		g_object_notify (G_OBJECT (pview), "document");
	}
}

static void
pps_view_presentation_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (object);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	switch (prop_id) {
	case PROP_DOCUMENT:
		pps_view_presentation_set_document (pview, PPS_DOCUMENT (g_value_get_object (value)));
		break;
	case PROP_CURRENT_PAGE:
		pps_view_presentation_set_current_page (pview, g_value_get_uint (value));
		break;
	case PROP_ROTATION:
		pps_view_presentation_set_rotation (pview, g_value_get_uint (value));
		break;
	case PROP_INVERTED_COLORS:
		priv->inverted_colors = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_view_presentation_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (object);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		g_value_set_uint (value, priv->current_page);
		break;
	case PROP_ROTATION:
		g_value_set_uint (value, pps_view_presentation_get_rotation (pview));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_view_presentation_notify_scale_factor (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	if (!gtk_widget_get_realized (GTK_WIDGET (pview)))
		return;

	pps_view_presentation_reset_jobs (pview);
	pps_view_presentation_update_current_page (pview, priv->current_page);
}

static GObject *
pps_view_presentation_constructor (GType type,
                                   guint n_construct_properties,
                                   GObjectConstructParam *construct_params)
{
	GObject *object;
	PpsViewPresentation *pview;

	object = G_OBJECT_CLASS (pps_view_presentation_parent_class)->constructor (type, n_construct_properties, construct_params);
	pview = PPS_VIEW_PRESENTATION (object);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	priv->is_constructing = FALSE;

	g_signal_connect (object, "notify::scale-factor",
	                  G_CALLBACK (pps_view_presentation_notify_scale_factor), NULL);

	return object;
}

static void
pps_view_presentation_size_allocate (GtkWidget *widget,
                                     int width,
                                     int height,
                                     int baseline)
{
	PpsViewPresentation *pview = PPS_VIEW_PRESENTATION (widget);
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);
	GtkWidgetClass *parent_class = GTK_WIDGET_CLASS (
	    pps_view_presentation_parent_class);
	parent_class->size_allocate (widget, width, height, baseline);

	if (priv->goto_popup)
		gtk_popover_present (GTK_POPOVER (priv->goto_popup));
}

static void
pps_view_presentation_class_init (PpsViewPresentationClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	klass->change_page = pps_view_presentation_change_page;

	gobject_class->dispose = pps_view_presentation_dispose;

	widget_class->snapshot = pps_view_presentation_snapshot;
	widget_class->size_allocate = pps_view_presentation_size_allocate;

	gtk_widget_class_set_css_name (widget_class, "pps-presentation-view");

	gobject_class->constructor = pps_view_presentation_constructor;
	gobject_class->set_property = pps_view_presentation_set_property;
	gobject_class->get_property = pps_view_presentation_get_property;

	g_object_class_install_property (gobject_class,
	                                 PROP_DOCUMENT,
	                                 g_param_spec_object ("document",
	                                                      "Document",
	                                                      "Document",
	                                                      PPS_TYPE_DOCUMENT,
	                                                      G_PARAM_WRITABLE |
	                                                          G_PARAM_CONSTRUCT |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class,
	                                 PROP_CURRENT_PAGE,
	                                 g_param_spec_uint ("current-page",
	                                                    "Current Page",
	                                                    "The current page",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class,
	                                 PROP_ROTATION,
	                                 g_param_spec_uint ("rotation",
	                                                    "Rotation",
	                                                    "Current rotation angle",
	                                                    0, 360, 0,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class,
	                                 PROP_INVERTED_COLORS,
	                                 g_param_spec_boolean ("inverted-colors",
	                                                       "Inverted Colors",
	                                                       "Whether presentation is displayed with inverted colors",
	                                                       FALSE,
	                                                       G_PARAM_WRITABLE |
	                                                           G_PARAM_CONSTRUCT |
	                                                           G_PARAM_STATIC_STRINGS));

	signals[CHANGE_PAGE] =
	    g_signal_new ("change_page",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsViewPresentationClass, change_page),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__ENUM,
	                  G_TYPE_NONE, 1,
	                  GTK_TYPE_SCROLL_TYPE);
	signals[FINISHED] =
	    g_signal_new ("finished",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsViewPresentationClass, finished),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0,
	                  G_TYPE_NONE);
	signals[SIGNAL_EXTERNAL_LINK] =
	    g_signal_new ("external-link",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsViewPresentationClass, external_link),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1,
	                  PPS_TYPE_LINK_ACTION);

	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/papers/ui/view-presentation.ui");

	gtk_widget_class_bind_template_child_private (widget_class, PpsViewPresentation, goto_popup);
	gtk_widget_class_bind_template_child_private (widget_class, PpsViewPresentation, goto_entry);

	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_scroll_event);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_key_press_event);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_motion_notify_event);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_primary_button_released);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_secondary_button_released);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_goto_entry_activate);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_inhibit_screenlock);
	gtk_widget_class_bind_template_callback (widget_class, pps_view_presentation_uninhibit_screenlock);

	add_change_page_binding_keypad (widget_class, GDK_KEY_Left, 0, GTK_SCROLL_PAGE_BACKWARD);
	add_change_page_binding_keypad (widget_class, GDK_KEY_Right, 0, GTK_SCROLL_PAGE_FORWARD);
	add_change_page_binding_keypad (widget_class, GDK_KEY_Up, 0, GTK_SCROLL_PAGE_BACKWARD);
	add_change_page_binding_keypad (widget_class, GDK_KEY_Down, 0, GTK_SCROLL_PAGE_FORWARD);

	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_FORWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_FORWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_space, GDK_SHIFT_MASK,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_BACKWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_BackSpace, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_BACKWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_Page_Down, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_FORWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_Page_Up, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_BACKWARD);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
	                                     "change_page", "(i)", GTK_SCROLL_PAGE_FORWARD);
}

static void
pps_view_presentation_init (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	priv->is_constructing = TRUE;

	gtk_widget_init_template (GTK_WIDGET (pview));
}

PpsViewPresentation *
pps_view_presentation_new (PpsDocument *document,
                           guint current_page,
                           guint rotation,
                           gboolean inverted_colors)
{
	g_return_val_if_fail (PPS_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (current_page < pps_document_get_n_pages (document), NULL);

	return g_object_new (PPS_TYPE_VIEW_PRESENTATION,
	                     "document", document,
	                     "current_page", current_page,
	                     "rotation", rotation,
	                     "inverted_colors", inverted_colors,
	                     NULL);
}

guint
pps_view_presentation_get_current_page (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	return priv->current_page;
}

void
pps_view_presentation_set_rotation (PpsViewPresentation *pview,
                                    gint rotation)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	if (rotation >= 360)
		rotation -= 360;
	else if (rotation < 0)
		rotation += 360;

	if (priv->rotation == rotation)
		return;

	priv->rotation = rotation;
	g_object_notify (G_OBJECT (pview), "rotation");
	if (priv->is_constructing)
		return;

	pps_view_presentation_reset_jobs (pview);
	pps_view_presentation_update_current_page (pview, priv->current_page);
}

guint
pps_view_presentation_get_rotation (PpsViewPresentation *pview)
{
	PpsViewPresentationPrivate *priv = GET_PRIVATE (pview);

	return priv->rotation;
}

G_GNUC_END_IGNORE_DEPRECATIONS
