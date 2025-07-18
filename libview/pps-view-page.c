// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-view-page.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pps-view-page.h"

#include <string.h>

#include "pps-colors.h"
#include "pps-debug.h"
#include "pps-overlay.h"

#define PPS_STYLE_CLASS_DOCUMENT_PAGE "document-page"
#define PPS_STYLE_CLASS_INVERTED "inverted"

typedef struct
{
	PpsPoint cursor_offset;
	PpsAnnotation *annot;
} MovingAnnotInfo;

typedef struct
{
	gint index;

	PpsDocumentModel *model;
	PpsAnnotationsContext *annots_context;
	PpsSearchContext *search_context;

	PpsPageCache *page_cache;
	PpsPixbufCache *pixbuf_cache;

	gboolean had_search_results;
	MovingAnnotInfo moving_annot_info;
} PpsViewPagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsViewPage, pps_view_page, GTK_TYPE_WIDGET)

#define GET_PRIVATE(o) pps_view_page_get_instance_private (o)

enum {
	PROP_PAGE = 1,
	PROP_LAST
};
static GParamSpec *properties[PROP_LAST];

static void
pps_view_page_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
	PpsViewPage *page = PPS_VIEW_PAGE (object);
	switch (prop_id) {
	case PROP_PAGE:
		pps_view_page_set_page (page, g_value_get_int (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_view_page_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
	PpsViewPage *page = PPS_VIEW_PAGE (object);
	switch (prop_id) {
	case PROP_PAGE:
		g_value_set_int (value, pps_view_page_get_page (page));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_view_page_measure (GtkWidget *widget,
                       GtkOrientation orientation,
                       int for_size,
                       int *minimum,
                       int *natural,
                       int *minimum_baseline,
                       int *natural_baseline)
{
	PpsViewPage *page = PPS_VIEW_PAGE (widget);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gint width = 0, height = 0;
	gdouble width_raw, height_raw;
	PpsDocument *document;
	gdouble scale;
	gint rotation;

	if (priv->model != NULL && pps_document_model_get_document (priv->model) != NULL && priv->index >= 0) {
		document = pps_document_model_get_document (priv->model);
		scale = pps_document_model_get_scale (priv->model);
		rotation = pps_document_model_get_rotation (priv->model);

		pps_document_get_page_size (document, priv->index, &width_raw, &height_raw);

		width = (gint) (((rotation == 0 || rotation == 180) ? width_raw : height_raw) * scale + 0.5);
		height = (gint) (((rotation == 0 || rotation == 180) ? height_raw : width_raw) * scale + 0.5);
	}

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		*minimum = 0;
		*natural = width;
	} else {
		*minimum = 0;
		*natural = height;
	}
}

static void
doc_rect_to_view_rect (PpsViewPage *page,
                       const PpsRectangle *doc_rect,
                       GdkRectangle *view_rect)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gdouble scale = pps_document_model_get_scale (priv->model);
	gdouble page_width = gtk_widget_get_width (GTK_WIDGET (page));
	gdouble page_height = gtk_widget_get_height (GTK_WIDGET (page));
	gdouble x, y, width, height;

	switch (pps_document_model_get_rotation (priv->model)) {
	case 0:
		x = doc_rect->x1 * scale;
		y = doc_rect->y1 * scale;
		width = (doc_rect->x2 - doc_rect->x1) * scale;
		height = (doc_rect->y2 - doc_rect->y1) * scale;

		break;
	case 90: {
		x = page_width - doc_rect->y2 * scale;
		y = doc_rect->x1 * scale;
		width = (doc_rect->y2 - doc_rect->y1) * scale;
		height = (doc_rect->x2 - doc_rect->x1) * scale;
	} break;
	case 180: {
		x = page_width - doc_rect->x2 * scale;
		y = page_height - doc_rect->y2 * scale;
		width = (doc_rect->x2 - doc_rect->x1) * scale;
		height = (doc_rect->y2 - doc_rect->y1) * scale;
	} break;
	case 270: {
		x = doc_rect->y1 * scale;
		y = page_height - doc_rect->x2 * scale;
		width = (doc_rect->y2 - doc_rect->y1) * scale;
		height = (doc_rect->x2 - doc_rect->x1) * scale;
	} break;
	default:
		g_assert_not_reached ();
	}

	view_rect->x = (gint) (x + 0.5);
	view_rect->y = (gint) (y + 0.5);
	view_rect->width = (gint) (width + 0.5);
	view_rect->height = (gint) (height + 0.5);
}

static PpsPoint
view_point_to_doc_point (PpsViewPage *page, double x, double y)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gdouble scale = pps_document_model_get_scale (priv->model);
	gdouble page_width = gtk_widget_get_width (GTK_WIDGET (page));
	gdouble page_height = gtk_widget_get_height (GTK_WIDGET (page));
	PpsPoint point;

	switch (pps_document_model_get_rotation (priv->model)) {
	case 0:
		point.x = x / scale;
		point.y = y / scale;
		break;
	case 90: {
		point.x = (page_width - x) * scale;
		point.y = y / scale;
	} break;
	case 180: {
		point.x = (page_width - x) / scale;
		point.y = (page_height - y) / scale;
	} break;
	case 270: {
		point.x = x / scale;
		point.y = (page_height - y) / scale;
	} break;
	default:
		g_assert_not_reached ();
	}

	return point;
}

static void
stroke_view_rect (GtkSnapshot *snapshot,
                  GdkRectangle *clip,
                  GdkRGBA *color,
                  GdkRectangle *view_rect)
{
	GdkRectangle intersect;
	GdkRGBA border_color[4] = { *color, *color, *color, *color };
	float border_width[4] = { 1, 1, 1, 1 };

	if (gdk_rectangle_intersect (view_rect, clip, &intersect)) {
		gtk_snapshot_append_border (snapshot,
		                            &GSK_ROUNDED_RECT_INIT (intersect.x, intersect.y,
		                                                    intersect.width, intersect.height),
		                            border_width, border_color);
	}
}

static void
stroke_doc_rect (GtkSnapshot *snapshot,
                 PpsViewPage *page,
                 GdkRGBA *color,
                 GdkRectangle *clip,
                 PpsRectangle *doc_rect)
{
	GdkRectangle view_rect;

	doc_rect_to_view_rect (page, doc_rect, &view_rect);
	stroke_view_rect (snapshot, clip, color, &view_rect);
}

static void
show_chars_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	guint i;
	GdkRGBA color = { 1, 0, 0, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	pps_page_cache_get_text_layout (priv->page_cache, priv->index, &areas, &n_areas);
	if (!areas)
		return;

	for (i = 0; i < n_areas; i++) {
		PpsRectangle *doc_rect = areas + i;

		stroke_doc_rect (snapshot, page, &color, clip, doc_rect);
	}
}

static void
show_mapping_list_border (GtkSnapshot *snapshot,
                          PpsViewPage *page,
                          GdkRGBA *color,
                          GdkRectangle *clip,
                          PpsMappingList *mapping_list)
{
	GList *l;

	for (l = pps_mapping_list_get_list (mapping_list); l; l = g_list_next (l)) {
		PpsMapping *mapping = (PpsMapping *) l->data;

		stroke_doc_rect (snapshot, page, color, clip, &mapping->area);
	}
}

static void
show_links_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	GdkRGBA color = { 0, 0, 1, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_link_mapping (priv->page_cache, priv->index));
}

static void
show_forms_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 0, 1, 0, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_form_field_mapping (priv->page_cache, priv->index));
}

static void
show_annots_border (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 0, 1, 1, 1 };
	GListModel *model =
	    pps_annotations_context_get_annots_model (priv->annots_context);

	// To make this generic, in the future we should have an interface
	// to get areas, instead of the Mapping. See #382
	for (gint i = 0; i < g_list_model_get_n_items (model); i++) {
		g_autoptr (PpsAnnotation) annot = g_list_model_get_item (model, i);
		PpsRectangle area;
		if (priv->index != pps_annotation_get_page_index (annot))
			continue;
		pps_annotation_get_area (annot, &area);
		stroke_doc_rect (snapshot, page, &color, clip, &area);
	}
}

static void
show_images_border (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 1, 0, 1, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_image_mapping (priv->page_cache, priv->index));
}

static void
show_media_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 1, 1, 0, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_media_mapping (priv->page_cache, priv->index));
}

static void
show_selections_border (GtkSnapshot *snapshot,
                        PpsViewPage *page,
                        GdkRectangle *clip)
{
	cairo_region_t *region;
	guint i, n_rects;
	GdkRGBA color = { 0.75, 0.50, 0.25, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	region = pps_page_cache_get_text_mapping (priv->page_cache, priv->index);
	if (!region)
		return;

	region = cairo_region_copy (region);
	n_rects = cairo_region_num_rectangles (region);
	for (i = 0; i < n_rects; i++) {
		GdkRectangle doc_rect_int;
		PpsRectangle doc_rect_float;

		cairo_region_get_rectangle (region, i, &doc_rect_int);

		/* Convert the doc rect to a PpsRectangle */
		doc_rect_float.x1 = doc_rect_int.x;
		doc_rect_float.y1 = doc_rect_int.y;
		doc_rect_float.x2 = doc_rect_int.x + doc_rect_int.width;
		doc_rect_float.y2 = doc_rect_int.y + doc_rect_int.height;

		stroke_doc_rect (snapshot, page, &color, clip, &doc_rect_float);
	}
	cairo_region_destroy (region);
}

static void
draw_debug_borders (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsDebugBorders borders = pps_debug_get_debug_borders ();

	if (borders & PPS_DEBUG_BORDER_CHARS)
		show_chars_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_LINKS)
		show_links_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_FORMS)
		show_forms_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_ANNOTS)
		show_annots_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_IMAGES)
		show_images_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_MEDIA)
		show_media_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_SELECTIONS)
		show_selections_border (snapshot, page, clip);
}

static void
draw_rect (GtkSnapshot *snapshot,
           const GdkRectangle *rect,
           const GdkRGBA *color)
{
	graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (rect->x,
	                                                    rect->y,
	                                                    rect->width,
	                                                    rect->height);

	gtk_snapshot_append_color (snapshot, color, &graphene_rect);
}

static void
draw_selection_region (GtkSnapshot *snapshot,
                       cairo_region_t *region,
                       GdkRGBA *color)
{
	cairo_rectangle_int_t box;
	gint n_boxes, i;

	n_boxes = cairo_region_num_rectangles (region);

	for (i = 0; i < n_boxes; i++) {
		cairo_region_get_rectangle (region, i, &box);
		draw_rect (snapshot, &box, color);
	}
}

static void
highlight_find_results (GtkSnapshot *snapshot,
                        PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GtkSingleSelection *model = pps_search_context_get_result_model (priv->search_context);
	g_autoptr (GPtrArray) results = pps_search_context_get_results_on_page (priv->search_context, priv->index);
	PpsRectangle pps_rect;
	GdkRGBA color_selected, color_default;

	get_accent_color (&color_selected, NULL);
	color_selected.alpha *= 0.6;
	get_accent_color (&color_default, NULL);
	color_default.alpha *= 0.3;

	for (gint i = 0; i < results->len; i++) {
		PpsSearchResult *result = g_ptr_array_index (results, i);
		PpsFindRectangle *find_rect;
		GList *rectangles;
		GdkRectangle view_rectangle;
		GdkRGBA color = color_default;

		rectangles = pps_search_result_get_rectangle_list (result);
		find_rect = (PpsFindRectangle *) rectangles->data;
		pps_rect.x1 = find_rect->x1;
		pps_rect.x2 = find_rect->x2;
		pps_rect.y1 = find_rect->y1;
		pps_rect.y2 = find_rect->y2;

		if (result == gtk_single_selection_get_selected_item (model))
			color = color_selected;

		doc_rect_to_view_rect (page, &pps_rect, &view_rectangle);
		draw_rect (snapshot, &view_rectangle, &color);

		if (rectangles->next) {
			/* Draw now next result (which is second part of multi-line match) */
			find_rect = (PpsFindRectangle *) rectangles->next->data;
			pps_rect.x1 = find_rect->x1;
			pps_rect.x2 = find_rect->x2;
			pps_rect.y1 = find_rect->y1;
			pps_rect.y2 = find_rect->y2;
			doc_rect_to_view_rect (page, &pps_rect, &view_rectangle);
			draw_rect (snapshot, &view_rectangle, &color);
		}
	}
}

static void
draw_surface (GtkSnapshot *snapshot,
              GdkTexture *texture,
              const graphene_rect_t *area,
              gboolean inverted)
{
	gboolean snap_texture = gdk_texture_get_height (texture) == floor (area->size.height);
	gtk_snapshot_save (snapshot);

	if (inverted) {
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_COLOR);
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_DIFFERENCE);
		gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 1., 1., 1., 1. }, area);
		gtk_snapshot_pop (snapshot);
		if (snap_texture) {
			gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST, area);
		} else {
			gtk_snapshot_append_texture (snapshot, texture, area);
		}
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_pop (snapshot);
		if (snap_texture) {
			gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST, area);
		} else {
			gtk_snapshot_append_texture (snapshot, texture, area);
		}
		gtk_snapshot_pop (snapshot);
	} else {
		if (snap_texture) {
			gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST, area);
		} else {
			gtk_snapshot_append_texture (snapshot, texture, area);
		}
	}

	gtk_snapshot_restore (snapshot);
}

static void
pps_view_page_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	PpsViewPage *page = PPS_VIEW_PAGE (widget);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	guint width = gtk_widget_get_width (widget);
	guint height = gtk_widget_get_height (widget);
	GdkTexture *page_texture = NULL, *selection_texture = NULL;
	GdkRectangle area_rect;
	graphene_rect_t area;
	cairo_region_t *region = NULL;
	gboolean inverted;
	gdouble scale;
	GtkNative *native = gtk_widget_get_native (widget);
	gdouble fractional_scale = gdk_surface_get_scale (gtk_native_get_surface (native));

	if (priv->model == NULL || priv->index < 0)
		return;

	inverted = pps_document_model_get_inverted_colors (priv->model);
	scale = pps_document_model_get_scale (priv->model);
	page_texture = pps_pixbuf_cache_get_texture (priv->pixbuf_cache, priv->index);

	if (!page_texture)
		return;

	area_rect.x = 0;
	area_rect.y = 0;
	area_rect.width = width;
	area_rect.height = height;

	/* snap the texture to a physical pixel so it is not blurred */
	/* FIXME: it is not clear why a translation of 1 - ceil(fractional_scale) / fractional_scale
	is necessary, but it seems to be so in practice. It looks like it is important to have
	a translation of a number in the interval (0, 1) (if fractional_scale is not an integer)
	so that there is no (widget) pixel on the boundary of two physical pixels. In the future,
	snapping API in GTK should provide a cleaner solution. */
	area = GRAPHENE_RECT_INIT (1 - ceil (fractional_scale) / fractional_scale,
	                           1 - ceil (fractional_scale) / fractional_scale,
	                           ceil (width * fractional_scale),
	                           ceil (height * fractional_scale));
	gtk_snapshot_save (snapshot);
	gtk_snapshot_scale (snapshot, 1 / fractional_scale, 1 / fractional_scale);

	draw_surface (snapshot, page_texture, &area, inverted);

	/* Get the selection pixbuf iff we have something to draw */
	selection_texture = pps_pixbuf_cache_get_selection_texture (priv->pixbuf_cache,
	                                                            priv->index,
	                                                            scale);

	if (selection_texture) {
		draw_surface (snapshot, selection_texture, &area, false);
		// Restore fractional scaling
		gtk_snapshot_restore (snapshot);
	} else {
		// Restore fractional scaling
		gtk_snapshot_restore (snapshot);
		region = pps_pixbuf_cache_get_selection_region (priv->pixbuf_cache,
		                                                priv->index,
		                                                scale);
		if (region) {
			GdkRGBA color;

			get_accent_color (&color, NULL);
			draw_selection_region (snapshot, region, &color);
		}
	}

	if (pps_search_context_get_active (priv->search_context))
		highlight_find_results (snapshot, page);

	if (pps_debug_get_debug_borders ())
		draw_debug_borders (snapshot, page, &area_rect);
}

static void
inverted_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsViewPage *page)
{
	if (pps_document_model_get_inverted_colors (model)) {
		gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);
	} else {
		gtk_widget_remove_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);
	}
}

static void
job_finished_cb (PpsPixbufCache *pixbuf_cache,
                 int finished_page,
                 PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	if (finished_page == priv->index)
		gtk_widget_queue_draw (GTK_WIDGET (page));
}

static void
search_results_changed_cb (PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gboolean has_search_results = pps_search_context_has_results_on_page (priv->search_context, priv->index);

	/*
	 * If there are currently no search results shown (has_search_results) nor
	 * there are any to be shown in the next round (has_search_results), there
	 * is no reason to redraw. If either is true, we might have changes.
	 */
	if (has_search_results || priv->had_search_results)
		gtk_widget_queue_draw (GTK_WIDGET (page));

	priv->had_search_results = has_search_results;
}

static void
move_annot_to_point (PpsViewPage *page,
                     gdouble view_point_x,
                     gdouble view_point_y)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	PpsDocument *document = pps_document_model_get_document (priv->model);
	PpsRectangle new_area, old_area;
	PpsPoint doc_point;
	double page_width, page_height;

	pps_annotation_get_area (priv->moving_annot_info.annot, &old_area);
	pps_document_get_page_size (document, priv->index, &page_width, &page_height);
	doc_point = view_point_to_doc_point (page, view_point_x, view_point_y);

	new_area.x1 = MAX (0, doc_point.x - priv->moving_annot_info.cursor_offset.x);
	new_area.y1 = MAX (0, doc_point.y - priv->moving_annot_info.cursor_offset.y);
	new_area.x2 = new_area.x1 + old_area.x2 - old_area.x1;
	new_area.y2 = new_area.y1 + old_area.y2 - old_area.y1;

	/* Prevent the annotation from being moved off the page */
	if (new_area.x2 > page_width) {
		new_area.x2 = page_width;
		new_area.x1 = page_width - old_area.x2 + old_area.x1;
	}
	if (new_area.y2 > page_height) {
		new_area.y2 = page_height;
		new_area.y1 = page_height - old_area.y2 + old_area.y1;
	}

	pps_annotation_set_area (priv->moving_annot_info.annot, &new_area);
}

static void
annotation_drag_update_cb (GtkGestureDrag *annotation_drag_gesture,
                           gdouble offset_x,
                           gdouble offset_y,
                           PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkEventSequence *sequence = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (annotation_drag_gesture));
	gdouble start_x, start_y, view_point_x, view_point_y;

	if (!priv->moving_annot_info.annot)
		g_assert_not_reached ();

	if (gtk_drag_check_threshold (GTK_WIDGET (page), 0, 0,
	                              offset_x, offset_y))
		gtk_gesture_set_state (GTK_GESTURE (annotation_drag_gesture),
		                       GTK_EVENT_SEQUENCE_CLAIMED);

	if (gtk_gesture_get_sequence_state (GTK_GESTURE (annotation_drag_gesture), sequence) != GTK_EVENT_SEQUENCE_CLAIMED)
		return;

	gtk_gesture_drag_get_start_point (annotation_drag_gesture, &start_x, &start_y);

	view_point_x = start_x + offset_x;
	view_point_y = start_y + offset_y;
	move_annot_to_point (page, view_point_x, view_point_y);
}

static void
annotation_drag_begin_cb (GtkGestureDrag *annotation_drag_gesture,
                          gdouble x,
                          gdouble y,
                          PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	PpsAnnotation *annot;
	PpsDocumentPoint doc_point;
	PpsRectangle annot_area;

	doc_point.page_index = priv->index;
	doc_point.point_on_page = view_point_to_doc_point (page, x, y);

	annot = pps_annotations_context_get_annot_at_doc_point (priv->annots_context,
	                                                        &doc_point);

	if (!PPS_IS_ANNOTATION_TEXT (annot)) {
		gtk_gesture_set_state (GTK_GESTURE (annotation_drag_gesture),
		                       GTK_EVENT_SEQUENCE_DENIED);
		return;
	}

	if (pps_document_model_get_annotation_editing_state (priv->model) != PPS_ANNOTATION_EDITING_STATE_NONE) {
		gtk_gesture_set_state (GTK_GESTURE (annotation_drag_gesture),
		                       GTK_EVENT_SEQUENCE_DENIED);
		return;
	}

	g_set_object (&(priv->moving_annot_info.annot), annot);

	pps_annotation_get_area (annot, &annot_area);
	/* Remember the offset of the cursor with respect to
	 * the annotation area in order to prevent the annotation from
	 * jumping under the cursor while moving it. */
	priv->moving_annot_info.cursor_offset.x = doc_point.point_on_page.x - annot_area.x1;
	priv->moving_annot_info.cursor_offset.y = doc_point.point_on_page.y - annot_area.y1;
}

static void
annotation_drag_end_cb (GtkGestureDrag *annotation_drag_gesture,
                        gdouble offset_x,
                        gdouble offset_y,
                        PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_clear_object (&(priv->moving_annot_info.annot));
}

static void
pps_view_page_init (PpsViewPage *page)
{
	GtkGesture *annot_drag;

	annot_drag = gtk_gesture_drag_new ();
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (annot_drag),
	                                            GTK_PHASE_CAPTURE);
	gtk_gesture_single_set_exclusive (GTK_GESTURE_SINGLE (annot_drag), TRUE);
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (annot_drag), 1);

	g_signal_connect (annot_drag, "drag-begin",
	                  G_CALLBACK (annotation_drag_begin_cb), page);

	gtk_widget_add_controller (GTK_WIDGET (page),
	                           GTK_EVENT_CONTROLLER (annot_drag));
	g_signal_connect (annot_drag, "drag-end",
	                  G_CALLBACK (annotation_drag_end_cb), page);
	g_signal_connect (annot_drag, "drag-update",
	                  G_CALLBACK (annotation_drag_update_cb), page);

	gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_DOCUMENT_PAGE);
	gtk_widget_add_css_class (GTK_WIDGET (page), "card");
}

static void
pps_view_page_dispose (GObject *object)
{
	PpsViewPage *page = PPS_VIEW_PAGE (object);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_clear_object (&priv->model);
	g_clear_object (&priv->pixbuf_cache);
	g_clear_object (&priv->page_cache);
	g_clear_object (&priv->annots_context);
	g_clear_object (&priv->search_context);
	g_clear_object (&priv->annots_context);

	G_OBJECT_CLASS (pps_view_page_parent_class)->dispose (object);
}

static void
pps_view_page_size_allocate (GtkWidget *widget,
                             int width,
                             int height,
                             int baseline)
{
	PpsViewPage *view_page = PPS_VIEW_PAGE (widget);
	PpsViewPagePrivate *priv = GET_PRIVATE (view_page);
	gdouble scale = pps_document_model_get_scale (priv->model);

	for (GtkWidget *child = gtk_widget_get_first_child (widget);
	     child != NULL;
	     child = gtk_widget_get_next_sibling (child)) {
		GdkRectangle real_view_area;
		if (PPS_IS_OVERLAY (child)) {
			gdouble padding;
			g_autofree PpsRectangle *doc_rect;

			doc_rect = pps_overlay_get_area (PPS_OVERLAY (child), &padding);

			real_view_area.x = doc_rect->x1 * scale - padding;
			real_view_area.y = doc_rect->y1 * scale - padding;
			real_view_area.width = (doc_rect->x2 - doc_rect->x1) * scale + 2 * padding;
			real_view_area.height = (doc_rect->y2 - doc_rect->y1) * scale + 2 * padding;

			gtk_widget_set_size_request (child, real_view_area.width, real_view_area.height);

			gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, real_view_area.height, NULL, NULL, NULL, NULL);
			gtk_widget_size_allocate (child, &real_view_area, baseline);
		}
	}
}

static void
pps_view_page_class_init (PpsViewPageClass *page_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (page_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (page_class);

	object_class->dispose = pps_view_page_dispose;
	object_class->get_property = pps_view_page_get_property;
	object_class->set_property = pps_view_page_set_property;
	widget_class->snapshot = pps_view_page_snapshot;
	widget_class->measure = pps_view_page_measure;
	widget_class->size_allocate = pps_view_page_size_allocate;
	properties[PROP_PAGE] =
	    g_param_spec_int ("page",
	                      "Page",
	                      "Current page index",
	                      -1, G_MAXINT, 0,
	                      G_PARAM_READWRITE |
	                          G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class,
	                                 PROP_PAGE,
	                                 properties[PROP_PAGE]);
}

void
pps_view_page_setup (PpsViewPage *page,
                     PpsDocumentModel *model,
                     PpsAnnotationsContext *annots_context,
                     PpsSearchContext *search_context,
                     PpsPageCache *page_cache,
                     PpsPixbufCache *pixbuf_cache)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_return_if_fail (PPS_IS_VIEW (gtk_widget_get_parent (GTK_WIDGET (page))));

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, page);
	if (priv->pixbuf_cache != NULL)
		g_signal_handlers_disconnect_by_data (priv->pixbuf_cache, page);
	if (priv->search_context != NULL)
		g_signal_handlers_disconnect_by_data (priv->search_context, page);
	if (priv->annots_context != NULL)
		g_signal_handlers_disconnect_by_data (priv->annots_context, page);

	g_set_object (&priv->model, model);
	g_set_object (&priv->annots_context, annots_context);
	g_set_object (&priv->search_context, search_context);
	g_set_object (&priv->page_cache, page_cache);
	g_set_object (&priv->pixbuf_cache, pixbuf_cache);
	g_set_object (&priv->annots_context, annots_context);
	pps_view_page_set_page (page, -1);

	g_signal_connect_swapped (priv->model, "notify::scale",
	                          G_CALLBACK (gtk_widget_queue_resize), page);
	g_signal_connect (priv->model, "notify::inverted-colors",
	                  G_CALLBACK (inverted_changed_cb), page);
	g_signal_connect (priv->pixbuf_cache, "job-finished",
	                  G_CALLBACK (job_finished_cb), page);
	g_signal_connect_swapped (priv->search_context, "finished",
	                          G_CALLBACK (search_results_changed_cb), page);
	g_signal_connect_swapped (priv->search_context, "result-activated",
	                          G_CALLBACK (search_results_changed_cb), page);
	g_signal_connect_swapped (priv->search_context, "notify::active",
	                          G_CALLBACK (gtk_widget_queue_draw), page);

	if (pps_document_model_get_inverted_colors (priv->model))
		gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);

	g_signal_connect_object (PPS_VIEW (gtk_widget_get_parent (GTK_WIDGET (page))),
	                         "selection-changed",
	                         G_CALLBACK (gtk_widget_queue_draw),
	                         page, G_CONNECT_SWAPPED);
}

PpsViewPage *
pps_view_page_new (void)
{
	return g_object_new (PPS_TYPE_VIEW_PAGE,
	                     "overflow", GTK_OVERFLOW_HIDDEN,
	                     NULL);
}

void
pps_view_page_set_page (PpsViewPage *page, gint index)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_return_if_fail (priv->model != NULL && pps_document_model_get_document (priv->model) != NULL);

	if (priv->index == index)
		return;

	if (0 <= index && index < pps_document_get_n_pages (pps_document_model_get_document (priv->model))) {
		priv->index = index;
	} else {
		priv->index = -1;
	}

	priv->had_search_results = FALSE;

	g_object_notify_by_pspec (G_OBJECT (page), properties[PROP_PAGE]);

	gtk_widget_queue_resize (GTK_WIDGET (page));
}

gint
pps_view_page_get_page (PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	return priv->index;
}
