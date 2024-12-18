/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
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
#error "This is a private header."
#endif

#include "adwaita.h"
#include "pps-form-field.h"
#include "pps-image.h"
#include "pps-jobs.h"
#include "pps-page-cache.h"
#include "pps-pixbuf-cache.h"
#include "pps-selection.h"
#include "pps-view-cursor.h"
#include "pps-view.h"

/* Information for middle clicking and moving around the doc */
typedef struct
{
	gdouble hadj;
	gdouble vadj;
	guint release_timeout_id;
	gdouble momentum_x;
	gdouble momentum_y;
	gboolean in_notify;
} DragInfo;

/* Information for handling selection */
typedef struct
{
	gdouble start_x;
	gdouble start_y;
	GList *selections;
	PpsSelectionStyle style;
	gdouble motion_x;
	gdouble motion_y;
} SelectionInfo;

/* Annotation popup windows */
typedef struct
{
	GtkWidget *window;
	guint page;

	gboolean visible;
} PpsViewWindowChild;

typedef enum {
	SCROLL_TO_KEEP_POSITION,
	SCROLL_TO_PAGE_POSITION,
	SCROLL_TO_CENTER,
	SCROLL_TO_FIND_LOCATION,
} PendingScroll;

typedef struct _PpsHeightToPageCache {
	gint rotation;
	gboolean dual_even_left;
	gdouble *height_to_page;
	gdouble *dual_height_to_page;
} PpsHeightToPageCache;

/* Information for handling annotations */
typedef struct
{
	PpsPoint cursor_offset;
	PpsAnnotation *annot;
} MovingAnnotInfo;

/* Information for handling link preview thumbnails */
typedef struct
{
	PpsJob *job;
	gdouble left;
	gdouble top;
	GtkWidget *popover;
	PpsLink *link;
	guint delay_timeout_id;
} PpsLinkPreview;

typedef struct
{
	gboolean active;
	gboolean in_selection;
	gdouble start_x;
	gdouble start_y;
	gdouble stop_x;
	gdouble stop_y;
} SigningInfo;

typedef struct _PpsViewPrivate {
	PpsDocument *document;

	/* Find */
	PpsSearchContext *search_context;
	PpsSearchResult *find_result; /* The selected find_result */
	gboolean highlight_find_results;

	PpsDocumentModel *model;
	PpsPixbufCache *pixbuf_cache;
	gsize pixbuf_cache_size;
	PpsPageCache *page_cache;
	PpsHeightToPageCache *height_to_page_cache;
	PpsViewCursor cursor;

	GtkRequisition requisition;

	/* Scrolling */
	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;
	/* GtkScrollablePolicy needs to be checked when
	 * driving the scrollable adjustment values */
	guint hscroll_policy : 1;
	guint vscroll_policy : 1;

	gint scroll_x;
	gint scroll_y;

	guint update_cursor_idle_id;

	/* Delta sum for emulating normal scrolling */
	gdouble total_delta;
	PendingScroll pending_scroll;
	gboolean pending_resize;
	PpsPoint pending_point;

	/* Animation for scrolling with keys */
	AdwAnimation *scroll_animation_vertical;
	AdwAnimation *scroll_animation_horizontal;
	gboolean pending_scroll_animation;

	/* Current geometry */

	gint start_page;
	gint end_page;
	gint current_page;

	gint rotation;
	gdouble scale;
	gint spacing;

	gboolean can_zoom_in;
	gboolean can_zoom_out;
	gboolean continuous;
	gboolean dual_even_left;
	PpsSizingMode sizing_mode;
	PpsPageLayout page_layout;
	gboolean allow_links_change_zoom;

	/* Key bindings propagation */
	gboolean key_binding_handled;

	/* Information for middle clicking and dragging around. */
	DragInfo drag_info;

	/* Selection */
	gdouble motion_x;
	gdouble motion_y;
	guint selection_update_id;
	guint selection_scroll_id;

	SelectionInfo selection_info;

	/* Copy link address selection */
	PpsLinkAction *link_selected;

	/* Image DND */
	PpsImage *dnd_image;

	/* Annotations */
	GList *window_children;
	MovingAnnotInfo moving_annot_info;
	GHashTable *annot_window_map;
	gboolean enable_spellchecking;
	GdkRGBA annot_color;

	/* Focus */
	PpsMapping *focused_element;
	guint focused_element_page;
	guint child_focus_idle_id;

	/* Caret navigation */
	gboolean caret_enabled;
	gint cursor_offset;
	gint cursor_page;
	gdouble cursor_line_offset;
	gboolean cursor_visible;
	guint cursor_blink_timeout_id;
	guint cursor_blink_time;

	/* Gestures */
	GtkGesture *middle_clicked_drag_gesture;
	GtkGesture *middle_clicked_drag_swipe_gesture;
	gdouble prev_zoom_gesture_scale;

	/* Current zoom center */
	gdouble zoom_center_x;
	gdouble zoom_center_y;

	/* Link preview */
	PpsLinkPreview link_preview;

	/* Signing Info */
	SigningInfo signing_info;
} PpsViewPrivate;

struct _PpsViewClass {
	GtkWidgetClass parent_class;

	void (*scroll) (PpsView *view,
	                GtkScrollType scroll,
	                GtkOrientation orientation);
	void (*handle_link) (PpsView *view,
	                     gint old_page,
	                     PpsLink *link);
	void (*external_link) (PpsView *view,
	                       PpsLinkAction *action);
	void (*popup_menu) (PpsView *view,
	                    GList *items);
	void (*selection_changed) (PpsView *view);
	void (*annot_added) (PpsView *view,
	                     PpsAnnotation *annot);
	void (*annot_cancel_add) (PpsView *view);
	void (*annot_changed) (PpsView *view,
	                       PpsAnnotation *annot);
	void (*annot_removed) (PpsView *view,
	                       PpsAnnotation *annot);
	void (*layers_changed) (PpsView *view);
	gboolean (*move_cursor) (PpsView *view,
	                         GtkMovementStep step,
	                         gint count,
	                         gboolean extend_selection);
	void (*activate) (PpsView *view);
	void (*signature_rect) (PpsView *view,
	                        guint page,
	                        PpsRectangle *rectangle);
};

void _get_page_size_for_scale_and_rotation (PpsDocument *document,
                                            gint page,
                                            gdouble scale,
                                            gint rotation,
                                            gint *page_width,
                                            gint *page_height);
void _pps_view_transform_view_point_to_doc_point (PpsView *view,
                                                  gdouble view_point_x,
                                                  gdouble view_point_y,
                                                  GdkRectangle *page_area,
                                                  GtkBorder *border,
                                                  gdouble *doc_point_x,
                                                  gdouble *doc_point_y);
void _pps_view_transform_view_rect_to_doc_rect (PpsView *view,
                                                GdkRectangle *view_rect,
                                                GdkRectangle *page_area,
                                                GtkBorder *border,
                                                PpsRectangle *doc_rect);
void _pps_view_transform_doc_point_to_view_point (PpsView *view,
                                                  int page,
                                                  PpsPoint *doc_point,
                                                  gdouble *view_point_x,
                                                  gdouble *view_point_y);
void _pps_view_transform_doc_point_by_rotation_scale (PpsView *view,
                                                      int page,
                                                      PpsPoint *doc_point,
                                                      gdouble *view_point_x,
                                                      gdouble *view_point_y);
void _pps_view_transform_doc_rect_to_view_rect (PpsView *view,
                                                int page,
                                                PpsRectangle *doc_rect,
                                                GdkRectangle *view_rect);
void _pps_view_get_selection_colors (PpsView *view,
                                     GdkRGBA *bg_color,
                                     GdkRGBA *fg_color);
gint _pps_view_get_caret_cursor_offset_at_doc_point (PpsView *view,
                                                     gint page,
                                                     gdouble doc_x,
                                                     gdouble doc_y);
void _pps_view_clear_selection (PpsView *view);
void _pps_view_set_selection (PpsView *view,
                              gdouble start_x,
                              gdouble start_y,
                              gdouble stop_x,
                              gdouble stop_y);

void _pps_view_set_focused_element (PpsView *view,
                                    PpsMapping *element_mapping,
                                    gint page);
void _pps_view_focus_form_field (PpsView *view,
                                 PpsFormField *field);

void _pps_view_ensure_rectangle_is_visible (PpsView *view,
                                            GdkRectangle *rect);
