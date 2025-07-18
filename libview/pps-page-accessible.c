// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia S.L.
 * Author: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#include <config.h>

#include "pps-form-field-accessible.h"
#include "pps-image-accessible.h"
#include "pps-link-accessible.h"
#include "pps-page-accessible.h"
#include "pps-view-private.h"
#include <glib/gi18n-lib.h>

struct _PpsPageAccessiblePrivate {
	PpsViewAccessible *view_accessible;
	gint page;
	GHashTable *links;
	GPtrArray *children;
	gboolean children_initialized;
};

enum {
	PROP_0,
	PROP_VIEW_ACCESSIBLE,
	PROP_PAGE,
};

static void pps_page_accessible_component_iface_init (AtkComponentIface *iface);
static void pps_page_accessible_hypertext_iface_init (AtkHypertextIface *iface);
static void pps_page_accessible_text_iface_init (AtkTextIface *iface);

G_DEFINE_TYPE_WITH_CODE (PpsPageAccessible, pps_page_accessible, ATK_TYPE_OBJECT, G_ADD_PRIVATE (PpsPageAccessible) G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, pps_page_accessible_component_iface_init) G_IMPLEMENT_INTERFACE (ATK_TYPE_HYPERTEXT, pps_page_accessible_hypertext_iface_init) G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, pps_page_accessible_text_iface_init))

gint
pps_page_accessible_get_page (PpsPageAccessible *page_accessible)
{
	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (page_accessible), -1);

	return page_accessible->priv->page;
}

PpsViewAccessible *
pps_page_accessible_get_view_accessible (PpsPageAccessible *page_accessible)
{
	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (page_accessible), NULL);

	return page_accessible->priv->view_accessible;
}

static AtkObject *
pps_page_accessible_get_parent (AtkObject *obj)
{
	PpsPageAccessible *self;

	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (obj), NULL);

	self = PPS_PAGE_ACCESSIBLE (obj);

	return ATK_OBJECT (self->priv->view_accessible);
}

static gint
compare_mappings (PpsMapping *a, PpsMapping *b)
{
	gdouble dx, dy;

	/* Very rough heuristic for simple, non-tagged PDFs. */

	dy = a->area.y1 - b->area.y1;
	dx = a->area.x1 - b->area.x1;

	return ABS (dy) > 10 ? dy : dx;
}

static void
pps_page_accessible_initialize_children (PpsPageAccessible *self)
{
	PpsView *view;
	PpsMappingList *images;
	PpsMappingList *links;
	PpsMappingList *fields;
	GList *children = NULL;
	GList *list;

	if (self->priv->children_initialized)
		return;

	view = pps_page_accessible_get_view (self);
	if (!pps_page_cache_is_page_cached (view->page_cache, self->priv->page))
		return;

	self->priv->children_initialized = TRUE;

	links = pps_page_cache_get_link_mapping (view->page_cache, self->priv->page);
	images = pps_page_cache_get_image_mapping (view->page_cache, self->priv->page);
	fields = pps_page_cache_get_form_field_mapping (view->page_cache, self->priv->page);
	if (!links && !images && !fields)
		return;

	children = g_list_copy (pps_mapping_list_get_list (links));
	children = g_list_concat (children, g_list_copy (pps_mapping_list_get_list (images)));
	children = g_list_concat (children, g_list_copy (pps_mapping_list_get_list (fields)));

	children = g_list_sort (children, (GCompareFunc) compare_mappings);
	self->priv->children = g_ptr_array_new_full (g_list_length (children), (GDestroyNotify) g_object_unref);

	for (list = children; list && list->data; list = list->next) {
		PpsMapping *mapping = list->data;
		AtkObject *child = NULL;

		if (links && pps_mapping_list_find (links, mapping->data)) {
			PpsLinkAccessible *link = pps_link_accessible_new (self, PPS_LINK (mapping->data), &mapping->area);
			AtkHyperlink *atk_link = atk_hyperlink_impl_get_hyperlink (ATK_HYPERLINK_IMPL (link));

			child = atk_hyperlink_get_object (atk_link, 0);
		} else if (images && pps_mapping_list_find (images, mapping->data))
			child = ATK_OBJECT (pps_image_accessible_new (self, PPS_IMAGE (mapping->data), &mapping->area));
		else if (fields && pps_mapping_list_find (fields, mapping->data))
			child = ATK_OBJECT (pps_form_field_accessible_new (self, PPS_FORM_FIELD (mapping->data), &mapping->area));

		if (child)
			g_ptr_array_add (self->priv->children, child);
	}

	g_list_free (children);
}

static void
clear_children (PpsPageAccessible *self)
{
	gint i;
	AtkObject *child;

	if (!self->priv->children)
		return;

	for (i = 0; i < self->priv->children->len; i++) {
		child = g_ptr_array_index (self->priv->children, i);
		atk_object_notify_state_change (child, ATK_STATE_DEFUNCT, TRUE);
	}

	g_clear_pointer (&self->priv->children, g_ptr_array_unref);
}

static void
pps_page_accessible_finalize (GObject *object)
{
	PpsPageAccessiblePrivate *priv = PPS_PAGE_ACCESSIBLE (object)->priv;

	g_clear_pointer (&priv->links, g_hash_table_destroy);
	clear_children (PPS_PAGE_ACCESSIBLE (object));

	G_OBJECT_CLASS (pps_page_accessible_parent_class)->finalize (object);
}

static void
pps_page_accessible_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	PpsPageAccessible *accessible = PPS_PAGE_ACCESSIBLE (object);

	switch (prop_id) {
	case PROP_VIEW_ACCESSIBLE:
		accessible->priv->view_accessible = PPS_VIEW_ACCESSIBLE (g_value_get_object (value));
		break;
	case PROP_PAGE:
		accessible->priv->page = g_value_get_int (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_page_accessible_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	PpsPageAccessible *accessible = PPS_PAGE_ACCESSIBLE (object);

	switch (prop_id) {
	case PROP_VIEW_ACCESSIBLE:
		g_value_set_object (value, pps_page_accessible_get_view_accessible (accessible));
		break;
	case PROP_PAGE:
		g_value_set_int (value, pps_page_accessible_get_page (accessible));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

/*
 * We redefine atk_class->ref_relation_set instead of just calling
 * atk_object_add_relationship on pps_page_accessible_new because at
 * that moment not all the pages could be created, being easier add
 * the relation on demand.
 */
static AtkRelationSet *
pps_page_accessible_ref_relation_set (AtkObject *accessible)
{
	gint n_pages;
	PpsPageAccessible *self;
	AtkRelationSet *relation_set;
	AtkObject *accessible_array[1];
	AtkRelation *relation;

	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (accessible), NULL);
	self = PPS_PAGE_ACCESSIBLE (accessible);

	relation_set = ATK_OBJECT_CLASS (pps_page_accessible_parent_class)->ref_relation_set (accessible);
	if (relation_set == NULL)
		return NULL;

	n_pages = pps_view_accessible_get_n_pages (self->priv->view_accessible);
	if (n_pages == 0)
		return relation_set;

	if ((self->priv->page + 1) < n_pages && !atk_relation_set_contains (relation_set, ATK_RELATION_FLOWS_TO)) {
		AtkObject *next_page;

		next_page = atk_object_ref_accessible_child (ATK_OBJECT (self->priv->view_accessible),
		                                             self->priv->page + 1);
		accessible_array[0] = next_page;
		relation = atk_relation_new (accessible_array, 1, ATK_RELATION_FLOWS_TO);
		atk_relation_set_add (relation_set, relation);

		g_object_unref (relation);
		g_object_unref (next_page);
	}

	if (self->priv->page > 0 && !atk_relation_set_contains (relation_set, ATK_RELATION_FLOWS_FROM)) {
		AtkObject *prpps_page;

		prpps_page = atk_object_ref_accessible_child (ATK_OBJECT (self->priv->view_accessible),
		                                              self->priv->page - 1);
		accessible_array[0] = prpps_page;
		relation = atk_relation_new (accessible_array, 1, ATK_RELATION_FLOWS_FROM);
		atk_relation_set_add (relation_set, relation);

		g_object_unref (relation);
		g_object_unref (prpps_page);
	}

	return relation_set;
}

/* page accessible's state set is a copy pps-view accessible's state
 * set but removing ATK_STATE_SHOWING if the page is not on screen and
 * ATK_STATE_FOCUSED if it is not the relevant page. */
static AtkStateSet *
pps_page_accessible_ref_state_set (AtkObject *accessible)
{
	AtkStateSet *state_set;
	AtkStateSet *copy_set;
	AtkStateSet *view_accessible_state_set;
	PpsPageAccessible *self;
	PpsView *view;
	gint relevant_page;

	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (accessible), NULL);
	self = PPS_PAGE_ACCESSIBLE (accessible);
	view = pps_page_accessible_get_view (self);

	state_set = ATK_OBJECT_CLASS (pps_page_accessible_parent_class)->ref_state_set (accessible);
	atk_state_set_clear_states (state_set);

	view_accessible_state_set = atk_object_ref_state_set (ATK_OBJECT (self->priv->view_accessible));
	copy_set = atk_state_set_or_sets (state_set, view_accessible_state_set);

	if (self->priv->page >= view->start_page && self->priv->page <= view->end_page)
		atk_state_set_add_state (copy_set, ATK_STATE_SHOWING);
	else
		atk_state_set_remove_state (copy_set, ATK_STATE_SHOWING);

	relevant_page = pps_view_accessible_get_relevant_page (self->priv->view_accessible);
	if (atk_state_set_contains_state (view_accessible_state_set, ATK_STATE_FOCUSED) &&
	    self->priv->page == relevant_page)
		atk_state_set_add_state (copy_set, ATK_STATE_FOCUSED);
	else
		atk_state_set_remove_state (copy_set, ATK_STATE_FOCUSED);

	relevant_page = pps_view_accessible_get_relevant_page (self->priv->view_accessible);
	if (atk_state_set_contains_state (view_accessible_state_set, ATK_STATE_FOCUSED) &&
	    self->priv->page == relevant_page)
		atk_state_set_add_state (copy_set, ATK_STATE_FOCUSED);
	else
		atk_state_set_remove_state (copy_set, ATK_STATE_FOCUSED);

	g_object_unref (state_set);
	g_object_unref (view_accessible_state_set);

	return copy_set;
}

static gint
pps_page_accessible_get_n_children (AtkObject *accessible)
{
	PpsPageAccessible *self;

	self = PPS_PAGE_ACCESSIBLE (accessible);

	return self->priv->children == NULL ? 0 : self->priv->children->len;
}

static AtkObject *
pps_page_accessible_ref_child (AtkObject *accessible,
                               gint i)
{
	PpsPageAccessible *self;

	self = PPS_PAGE_ACCESSIBLE (accessible);

	g_return_val_if_fail (i >= 0 || i < self->priv->children->len, NULL);

	return g_object_ref (g_ptr_array_index (self->priv->children, i));
}

static void
pps_page_accessible_class_init (PpsPageAccessibleClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	atk_class->get_parent = pps_page_accessible_get_parent;
	atk_class->ref_relation_set = pps_page_accessible_ref_relation_set;
	atk_class->ref_state_set = pps_page_accessible_ref_state_set;
	atk_class->get_n_children = pps_page_accessible_get_n_children;
	atk_class->ref_child = pps_page_accessible_ref_child;

	g_object_class->get_property = pps_page_accessible_get_property;
	g_object_class->set_property = pps_page_accessible_set_property;
	g_object_class->finalize = pps_page_accessible_finalize;

	g_object_class_install_property (g_object_class,
	                                 PROP_VIEW_ACCESSIBLE,
	                                 g_param_spec_object ("view-accessible",
	                                                      "View Accessible",
	                                                      "The view accessible associated to this page",
	                                                      PPS_TYPE_VIEW_ACCESSIBLE,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_PAGE,
	                                 g_param_spec_int ("page",
	                                                   "Page",
	                                                   "Page index this page represents",
	                                                   -1, G_MAXINT, -1,
	                                                   G_PARAM_READWRITE |
	                                                       G_PARAM_CONSTRUCT_ONLY |
	                                                       G_PARAM_STATIC_STRINGS));
}

PpsView *
pps_page_accessible_get_view (PpsPageAccessible *page_accessible)
{
	g_return_val_if_fail (PPS_IS_PAGE_ACCESSIBLE (page_accessible), NULL);

	return PPS_VIEW (gtk_accessible_get_widget (GTK_ACCESSIBLE (page_accessible->priv->view_accessible)));
}

/* ATs expect to be able to identify sentence boundaries based on content. Valid,
 * content-based boundaries may be present at the end of a newline, for instance
 * at the end of a heading within a document. Thus being able to distinguish hard
 * returns from soft returns is necessary. However, the text we get from Poppler
 * for non-tagged PDFs has "\n" inserted at the end of each line resulting in a
 * broken accessibility implementation w.r.t. sentences.
 */
static gboolean
treat_as_soft_return (PpsView *view,
                      gint page,
                      PangoLogAttr *log_attrs,
                      gint offset)
{
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	gdouble line_spacing, this_line_height, next_word_width;
	PpsRectangle *this_line_start;
	PpsRectangle *this_line_end;
	PpsRectangle *next_line_start;
	PpsRectangle *next_line_end;
	PpsRectangle *next_word_end;
	gint prpps_offset, next_offset;

	if (!log_attrs[offset].is_white)
		return FALSE;

	pps_page_cache_get_text_layout (view->page_cache, page, &areas, &n_areas);
	if (n_areas <= offset + 1)
		return FALSE;

	prpps_offset = offset - 1;
	next_offset = offset + 1;

	/* In wrapped text, the character at the start of the next line starts a word.
	 * Examples where this condition might fail include bullets and images. But it
	 * also includes things like "(", so also check the next character.
	 */
	if (!log_attrs[next_offset].is_word_start &&
	    (next_offset + 1 >= n_areas || !log_attrs[next_offset + 1].is_word_start))
		return FALSE;

	/* In wrapped text, the chars on either side of the newline have very similar heights.
	 * Examples where this condition might fail include a newline at the end of a heading,
	 * and a newline at the end of a paragraph that is followed by a heading.
	 */
	this_line_end = areas + prpps_offset;
	next_line_start = areas + next_offset;
	;

	this_line_height = this_line_end->y2 - this_line_end->y1;
	if (ABS (this_line_height - (next_line_start->y2 - next_line_start->y1)) > 0.25)
		return FALSE;

	/* If there is significant white space between this line and the next, odds are this
	 * is not a soft return in wrapped text. Lines within a typical paragraph are at most
	 * double-spaced. If the spacing is more than that, assume a hard return is present.
	 */
	line_spacing = next_line_start->y1 - this_line_end->y2;
	if (line_spacing - this_line_height > 1)
		return FALSE;

	/* Lines within a typical paragraph have *reasonably* similar x1 coordinates. But
	 * we cannot count on them being nearly identical. Examples where indentation can
	 * be present in wrapped text include indenting the first line of the paragraph,
	 * and hanging indents (e.g. in the works cited within an academic paper). So we'll
	 * be somewhat tolerant here.
	 */
	for (; prpps_offset > 0 && !log_attrs[prpps_offset].is_mandatory_break; prpps_offset--)
		;
	this_line_start = areas + prpps_offset;
	if (ABS (this_line_start->x1 - next_line_start->x1) > 20)
		return FALSE;

	/* Ditto for x2, but this line might be short due to a wide word on the next line. */
	for (; next_offset < n_areas && !log_attrs[next_offset].is_word_end; next_offset++)
		;
	next_word_end = areas + next_offset;
	next_word_width = next_word_end->x2 - next_line_start->x1;

	for (; next_offset < n_areas && !log_attrs[next_offset + 1].is_mandatory_break; next_offset++)
		;
	next_line_end = areas + next_offset;
	if (next_line_end->x2 - (this_line_end->x2 + next_word_width) > 20)
		return FALSE;

	return TRUE;
}

static gchar *
pps_page_accessible_get_substring (AtkText *text,
                                   gint start_offset,
                                   gint end_offset)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	gchar *substring, *normalized;
	const gchar *page_text;

	if (!view->page_cache)
		return NULL;

	page_text = pps_page_cache_get_text (view->page_cache, self->priv->page);
	if (end_offset < 0 || end_offset > g_utf8_strlen (page_text, -1))
		end_offset = strlen (page_text);
	start_offset = CLAMP (start_offset, 0, end_offset);

	substring = g_utf8_substring (page_text, start_offset, end_offset);
	normalized = g_utf8_normalize (substring, -1, G_NORMALIZE_NFKC);
	g_free (substring);

	return normalized;
}

static gchar *
pps_page_accessible_get_text (AtkText *text,
                              gint start_pos,
                              gint end_pos)
{
	return pps_page_accessible_get_substring (text, start_pos, end_pos);
}

static gunichar
pps_page_accessible_get_character_at_offset (AtkText *text,
                                             gint offset)
{
	gchar *string;
	gunichar unichar;

	string = pps_page_accessible_get_substring (text, offset, offset + 1);
	unichar = g_utf8_get_char (string);
	g_free (string);

	return unichar;
}

static void
pps_page_accessible_get_range_for_boundary (AtkText *text,
                                            AtkTextBoundary boundary_type,
                                            gint offset,
                                            gint *start_offset,
                                            gint *end_offset)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	gint start = 0;
	gint end = 0;
	PangoLogAttr *log_attrs = NULL;
	gulong n_attrs;

	if (!view->page_cache)
		return;

	pps_page_cache_get_text_log_attrs (view->page_cache, self->priv->page, &log_attrs, &n_attrs);
	if (!log_attrs)
		return;

	if (offset < 0 || offset >= n_attrs)
		return;

	switch (boundary_type) {
	case ATK_TEXT_BOUNDARY_CHAR:
		start = offset;
		end = offset + 1;
		break;
	case ATK_TEXT_BOUNDARY_WORD_START:
		for (start = offset; start > 0 && !log_attrs[start].is_word_start; start--)
			;
		for (end = offset + 1; end < n_attrs && !log_attrs[end].is_word_start; end++)
			;
		break;
	case ATK_TEXT_BOUNDARY_SENTENCE_START:
		for (start = offset; start > 0; start--) {
			if (log_attrs[start].is_mandatory_break && treat_as_soft_return (view, self->priv->page, log_attrs, start - 1))
				continue;
			if (log_attrs[start].is_sentence_start)
				break;
		}
		for (end = offset + 1; end < n_attrs; end++) {
			if (log_attrs[end].is_mandatory_break && treat_as_soft_return (view, self->priv->page, log_attrs, end - 1))
				continue;
			if (log_attrs[end].is_sentence_start)
				break;
		}
		break;
	case ATK_TEXT_BOUNDARY_LINE_START:
		for (start = offset; start > 0 && !log_attrs[start].is_mandatory_break; start--)
			;
		for (end = offset + 1; end < n_attrs && !log_attrs[end].is_mandatory_break; end++)
			;
		break;
	default:
		/* The "END" boundary types are deprecated */
		break;
	}

	*start_offset = start;
	*end_offset = end;
}

static gchar *
pps_page_accessible_get_text_at_offset (AtkText *text,
                                        gint offset,
                                        AtkTextBoundary boundary_type,
                                        gint *start_offset,
                                        gint *end_offset)
{
	gchar *retval;

	pps_page_accessible_get_range_for_boundary (text, boundary_type, offset, start_offset, end_offset);
	retval = pps_page_accessible_get_substring (text, *start_offset, *end_offset);

	/* If newlines appear inside the text of a sentence (i.e. between the start and
	 * end offsets returned by pps_page_accessible_get_substring), it interferes with
	 * the prosody of text-to-speech based-solutions such as a screen reader because
	 * speech synthesizers tend to pause after the newline char as if it were the end
	 * of the sentence.
	 */
	if (boundary_type == ATK_TEXT_BOUNDARY_SENTENCE_START)
		g_strdelimit (retval, "\n", ' ');

	return retval;
}

static gint
pps_page_accessible_get_caret_offset (AtkText *text)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);

	if (self->priv->page == view->cursor_page && view->caret_enabled)
		return view->cursor_offset;

	return -1;
}

static gboolean
pps_page_accessible_set_caret_offset (AtkText *text, gint offset)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);

	pps_view_set_caret_cursor_position (view,
	                                    self->priv->page,
	                                    offset);

	return TRUE;
}

static gint
pps_page_accessible_get_character_count (AtkText *text)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	gint retval;

	retval = g_utf8_strlen (pps_page_cache_get_text (view->page_cache, self->priv->page), -1);

	return retval;
}

static gboolean
get_selection_bounds (PpsView *view,
                      PpsViewSelection *selection,
                      gint *start_offset,
                      gint *end_offset)
{
	cairo_rectangle_int_t rect;
	gint start, end;

	if (!selection->covered_region || cairo_region_is_empty (selection->covered_region))
		return FALSE;

	cairo_region_get_rectangle (selection->covered_region, 0, &rect);
	start = _pps_view_get_caret_cursor_offset_at_doc_point (view,
	                                                        selection->page,
	                                                        rect.x / view->scale,
	                                                        (rect.y + (rect.height / 2)) / view->scale);
	if (start == -1)
		return FALSE;

	cairo_region_get_rectangle (selection->covered_region,
	                            cairo_region_num_rectangles (selection->covered_region) - 1,
	                            &rect);
	end = _pps_view_get_caret_cursor_offset_at_doc_point (view,
	                                                      selection->page,
	                                                      (rect.x + rect.width) / view->scale,
	                                                      (rect.y + (rect.height / 2)) / view->scale);
	if (end == -1)
		return FALSE;

	*start_offset = start;
	*end_offset = end;

	return TRUE;
}

static gint
pps_page_accessible_get_n_selections (AtkText *text)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	gint n_selections = 0;
	GList *l;

	if (!PPS_IS_SELECTION (view->document) || !view->selection_info.selections)
		return 0;

	for (l = view->selection_info.selections; l != NULL; l = l->next) {
		PpsViewSelection *selection = (PpsViewSelection *) l->data;

		if (selection->page != self->priv->page)
			continue;

		n_selections = 1;
		break;
	}

	return n_selections;
}

static gchar *
pps_page_accessible_get_selection (AtkText *text,
                                   gint selection_num,
                                   gint *start_pos,
                                   gint *end_pos)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	gchar *selected_text = NULL;
	gchar *normalized_text = NULL;
	GList *l;

	*start_pos = -1;
	*end_pos = -1;

	if (selection_num != 0)
		return NULL;

	if (!PPS_IS_SELECTION (view->document) || !view->selection_info.selections)
		return NULL;

	for (l = view->selection_info.selections; l != NULL; l = l->next) {
		PpsViewSelection *selection = (PpsViewSelection *) l->data;
		gint start = 0, end = 0;

		if (selection->page != self->priv->page)
			continue;

		if (get_selection_bounds (view, selection, &start, &end) && start != end) {
			PpsPage *page;

			page = pps_document_get_page (view->document, selection->page);

			selected_text = pps_selection_get_selected_text (PPS_SELECTION (view->document),
			                                                 page,
			                                                 selection->style,
			                                                 &(selection->rect));

			g_object_unref (page);

			*start_pos = start;
			*end_pos = end;
		}

		break;
	}

	if (selected_text) {
		normalized_text = g_utf8_normalize (selected_text, -1, G_NORMALIZE_NFKC);
		g_free (selected_text);
	}

	return normalized_text;
}

static AtkAttributeSet *
add_attribute (AtkAttributeSet *attr_set,
               AtkTextAttribute attr_type,
               gchar *attr_value)
{
	AtkAttribute *attr = g_new (AtkAttribute, 1);

	attr->name = g_strdup (atk_text_attribute_get_name (attr_type));
	attr->value = attr_value;

	return g_slist_prepend (attr_set, attr);
}

static AtkAttributeSet *
get_run_attributes (PangoAttrList *attrs,
                    const gchar *text,
                    gint offset,
                    gint *start_offset,
                    gint *end_offset)
{
	AtkAttributeSet *atk_attr_set = NULL;
	PangoAttrString *pango_string;
	PangoAttrInt *pango_int;
	PangoAttrColor *pango_color;
	PangoAttrIterator *iter;
	gint i, start, end;
	gboolean has_attrs = FALSE;
	glong text_length;
	gchar *attr_value;

	text_length = g_utf8_strlen (text, -1);
	if (offset < 0 || offset >= text_length)
		return NULL;

	/* Check if there are attributes for the offset,
	 * and set the attributes range if positive */
	iter = pango_attr_list_get_iterator (attrs);
	i = g_utf8_offset_to_pointer (text, offset) - text;

	do {
		pango_attr_iterator_range (iter, &start, &end);
		if (i >= start && i < end) {
			*start_offset = g_utf8_pointer_to_offset (text, text + start);
			if (end == G_MAXINT) /* Last iterator */
				end = text_length;
			*end_offset = g_utf8_pointer_to_offset (text, text + end);
			has_attrs = TRUE;
		}
	} while (!has_attrs && pango_attr_iterator_next (iter));

	if (!has_attrs) {
		pango_attr_iterator_destroy (iter);
		return NULL;
	}

	/* Create the AtkAttributeSet from the Pango attributes */
	pango_string = (PangoAttrString *) pango_attr_iterator_get (iter, PANGO_ATTR_FAMILY);
	if (pango_string) {
		attr_value = g_strdup (pango_string->value);
		atk_attr_set = add_attribute (atk_attr_set, ATK_TEXT_ATTR_FAMILY_NAME, attr_value);
	}

	pango_int = (PangoAttrInt *) pango_attr_iterator_get (iter, PANGO_ATTR_SIZE);
	if (pango_int) {
		attr_value = g_strdup_printf ("%i", pango_int->value / PANGO_SCALE);
		atk_attr_set = add_attribute (atk_attr_set, ATK_TEXT_ATTR_SIZE, attr_value);
	}

	pango_int = (PangoAttrInt *) pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE);
	if (pango_int) {
		atk_attr_set = add_attribute (atk_attr_set,
		                              ATK_TEXT_ATTR_UNDERLINE,
		                              g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_UNDERLINE,
		                                                                      pango_int->value)));
	}

	pango_color = (PangoAttrColor *) pango_attr_iterator_get (iter, PANGO_ATTR_FOREGROUND);
	if (pango_color) {
		attr_value = g_strdup_printf ("%u,%u,%u",
		                              pango_color->color.red,
		                              pango_color->color.green,
		                              pango_color->color.blue);
		atk_attr_set = add_attribute (atk_attr_set, ATK_TEXT_ATTR_FG_COLOR, attr_value);
	}

	pango_attr_iterator_destroy (iter);

	return atk_attr_set;
}

static AtkAttributeSet *
pps_page_accessible_get_run_attributes (AtkText *text,
                                        gint offset,
                                        gint *start_offset,
                                        gint *end_offset)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	PangoAttrList *attrs;
	const gchar *page_text;

	if (offset < 0)
		return NULL;

	if (!view->page_cache)
		return NULL;

	page_text = pps_page_cache_get_text (view->page_cache, self->priv->page);
	if (!page_text)
		return NULL;

	attrs = pps_page_cache_get_text_attrs (view->page_cache, self->priv->page);
	if (!attrs)
		return NULL;

	return get_run_attributes (attrs, page_text, offset, start_offset, end_offset);
}

static AtkAttributeSet *
pps_page_accessible_get_default_attributes (AtkText *text)
{
	/* No default attributes */
	return NULL;
}

static void
pps_page_accessible_get_character_extents (AtkText *text,
                                           gint offset,
                                           gint *x,
                                           gint *y,
                                           gint *width,
                                           gint *height,
                                           AtkCoordType coords)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	GtkWidget *toplevel;
	PpsRectangle *areas = NULL;
	PpsRectangle *doc_rect;
	guint n_areas = 0;
	gint x_widget, y_widget;
	GdkRectangle view_rect;

	if (!view->page_cache)
		return;

	pps_page_cache_get_text_layout (view->page_cache, self->priv->page, &areas, &n_areas);
	if (!areas || offset >= n_areas)
		return;

	doc_rect = areas + offset;
	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, doc_rect, &view_rect);
	view_rect.x -= view->scroll_x;
	view_rect.y -= view->scroll_y;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	gtk_widget_translate_coordinates (GTK_WIDGET (view), toplevel, 0, 0, &x_widget, &y_widget);
	view_rect.x += x_widget;
	view_rect.y += y_widget;

	if (coords == ATK_XY_SCREEN) {
		gint x_window, y_window;

		gdk_window_get_origin (gtk_widget_get_window (toplevel), &x_window, &y_window);
		view_rect.x += x_window;
		view_rect.y += y_window;
	}

	*x = view_rect.x;
	*y = view_rect.y;
	*width = view_rect.width;
	*height = view_rect.height;
}

static gint
pps_page_accessible_get_offset_at_point (AtkText *text,
                                         gint x,
                                         gint y,
                                         AtkCoordType coords)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	GtkWidget *toplevel;
	PpsRectangle *areas = NULL;
	PpsRectangle *rect = NULL;
	guint n_areas = 0;
	guint i;
	gint x_widget, y_widget;
	gint offset = -1;
	gdouble view_x, view_y;
	PpsPoint point_on_page;

	if (!view->page_cache)
		return -1;

	pps_page_cache_get_text_layout (view->page_cache, self->priv->page, &areas, &n_areas);
	if (!areas)
		return -1;

	view_x = x;
	view_y = y;
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	gtk_widget_translate_coordinates (GTK_WIDGET (view), toplevel, 0, 0, &x_widget, &y_widget);
	view_x -= x_widget;
	view_y -= y_widget;

	if (coords == ATK_XY_SCREEN) {
		gint x_window, y_window;

		gdk_window_get_origin (gtk_widget_get_window (toplevel), &x_window, &y_window);
		view_x -= x_window;
		view_y -= y_window;
	}

	point_on_page = pps_view_get_point_on_page (view, self->priv->page,
	                                            view_x, view_y);

	for (i = 0; i < n_areas; i++) {
		rect = areas + i;
		if (point_on_page.x >= rect->x1 && point_on_page.x <= rect->x2 &&
		    point_on_page.y >= rect->y1 && point_on_page.y <= rect->y2)
			offset = i;
	}

	return offset;
}

/* ATK allows for multiple, non-contiguous selections within a single AtkText
 * object. Unless and until Papers supports this, selection numbers are ignored.
 */
static gboolean
pps_page_accessible_remove_selection (AtkText *text,
                                      gint selection_num)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);

	if (pps_view_has_selection (view)) {
		_pps_view_clear_selection (view);
		return TRUE;
	}

	return FALSE;
}

static gboolean
pps_page_accessible_set_selection (AtkText *text,
                                   gint selection_num,
                                   gint start_pos,
                                   gint end_pos)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	GdkRectangle start_rect, end_rect;
	gdouble start_x, start_y, end_x, end_y;

	pps_page_cache_get_text_layout (view->page_cache, self->priv->page, &areas, &n_areas);
	if (start_pos < 0 || end_pos >= n_areas)
		return FALSE;

	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, areas + start_pos, &start_rect);
	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, areas + end_pos - 1, &end_rect);
	start_x = start_rect.x;
	start_y = start_rect.y;
	end_x = end_rect.x + end_rect.width;
	end_y = end_rect.y + end_rect.height;
	_pps_view_set_selection (view, start_x, start_y, end_x, end_y);

	return TRUE;
}

static gboolean
pps_page_accessible_add_selection (AtkText *text,
                                   gint start_pos,
                                   gint end_pos)
{
	return pps_page_accessible_set_selection (text, 0, start_pos, end_pos);
}

#if ATK_CHECK_VERSION(2, 32, 0)
static gboolean
pps_page_accessible_scroll_substring_to (AtkText *text,
                                         gint start_pos,
                                         gint end_pos,
                                         AtkScrollType type)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	GdkRectangle start_rect, end_rect;
	gdouble start_x, start_y, end_x, end_y;
	gdouble hpage_size, vpage_size;
	PpsRectangle *areas = NULL;
	guint n_areas = 0;

	if (end_pos < start_pos)
		return FALSE;

	pps_page_cache_get_text_layout (view->page_cache, self->priv->page, &areas, &n_areas);
	if (start_pos < 0 || end_pos >= n_areas)
		return FALSE;

	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, areas + start_pos, &start_rect);
	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, areas + end_pos - 1, &end_rect);
	start_x = start_rect.x;
	start_y = start_rect.y;
	end_x = end_rect.x + end_rect.width;
	end_y = end_rect.y + end_rect.height;

	hpage_size = gtk_adjustment_get_page_size (view->hadjustment);
	vpage_size = gtk_adjustment_get_page_size (view->vadjustment);

	switch (type) {
	case ATK_SCROLL_TOP_LEFT:
		gtk_adjustment_clamp_page (view->hadjustment, start_x, start_x + hpage_size);
		gtk_adjustment_clamp_page (view->vadjustment, start_y, start_y + vpage_size);
		break;
	case ATK_SCROLL_BOTTOM_RIGHT:
		gtk_adjustment_clamp_page (view->hadjustment, end_x - hpage_size, end_x);
		gtk_adjustment_clamp_page (view->vadjustment, end_y - vpage_size, end_y);
		break;
	case ATK_SCROLL_TOP_EDGE:
		gtk_adjustment_clamp_page (view->vadjustment, start_y, start_y + vpage_size);
		break;
	case ATK_SCROLL_BOTTOM_EDGE:
		gtk_adjustment_clamp_page (view->vadjustment, end_y - vpage_size, end_y);
		break;
	case ATK_SCROLL_LEFT_EDGE:
		gtk_adjustment_clamp_page (view->hadjustment, start_x, start_x + hpage_size);
		break;
	case ATK_SCROLL_RIGHT_EDGE:
		gtk_adjustment_clamp_page (view->hadjustment, end_x - hpage_size, end_x);
		break;
	case ATK_SCROLL_ANYWHERE:
		_pps_view_ensure_rectangle_is_visible (view, &end_rect);
		_pps_view_ensure_rectangle_is_visible (view, &start_rect);
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

static gboolean
pps_page_accessible_scroll_substring_to_point (AtkText *text,
                                               gint start_pos,
                                               gint end_pos,
                                               AtkCoordType coords,
                                               gint x,
                                               gint y)
{
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (text);
	PpsView *view = pps_page_accessible_get_view (self);
	GdkRectangle start_rect;
	gdouble view_x, view_y, start_x, start_y;
	gdouble hpage_size, vpage_size;
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	GtkWidget *toplevel;
	gint x_widget, y_widget;
	PpsPoint point_on_page;

	if (end_pos < start_pos)
		return FALSE;

	pps_page_cache_get_text_layout (view->page_cache, self->priv->page, &areas, &n_areas);
	if (start_pos < 0 || end_pos >= n_areas)
		return FALSE;

	/* Assume that the API wants to place the top left of the substring at (x, y). */
	_pps_view_transform_doc_rect_to_view_rect (view, self->priv->page, areas + start_pos, &start_rect);
	start_x = start_rect.x;
	start_y = start_rect.y;

	hpage_size = gtk_adjustment_get_page_size (view->hadjustment);
	vpage_size = gtk_adjustment_get_page_size (view->vadjustment);

	view_x = x;
	view_y = y;
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	gtk_widget_translate_coordinates (GTK_WIDGET (view), toplevel, 0, 0, &x_widget, &y_widget);
	view_x -= x_widget;
	view_y -= y_widget;

	if (coords == ATK_XY_SCREEN) {
		gint x_window, y_window;

		gdk_window_get_origin (gtk_widget_get_window (toplevel), &x_window, &y_window);
		view_x -= x_window;
		view_y -= y_window;
	}

	point_on_page = pps_view_get_point_on_page (view, self->priv->page,
	                                            view_x, view_y);

	/* Calculate scrolling difference */
	start_x -= point_on_page.x;
	start_y -= point_on_page.y;

	gtk_adjustment_clamp_page (view->hadjustment, start_x, start_x + hpage_size);
	gtk_adjustment_clamp_page (view->vadjustment, start_y, start_y + vpage_size);

	return TRUE;
}
#endif

static void
pps_page_accessible_init (PpsPageAccessible *page)
{
	atk_object_set_role (ATK_OBJECT (page), ATK_ROLE_PAGE);

	page->priv = pps_page_accessible_get_instance_private (page);
}

static void
pps_page_accessible_text_iface_init (AtkTextIface *iface)
{
	iface->get_text = pps_page_accessible_get_text;
	iface->get_text_at_offset = pps_page_accessible_get_text_at_offset;
	iface->get_character_at_offset = pps_page_accessible_get_character_at_offset;
	iface->get_caret_offset = pps_page_accessible_get_caret_offset;
	iface->set_caret_offset = pps_page_accessible_set_caret_offset;
	iface->get_character_count = pps_page_accessible_get_character_count;
	iface->get_n_selections = pps_page_accessible_get_n_selections;
	iface->get_selection = pps_page_accessible_get_selection;
	iface->remove_selection = pps_page_accessible_remove_selection;
	iface->add_selection = pps_page_accessible_add_selection;
	iface->get_run_attributes = pps_page_accessible_get_run_attributes;
	iface->get_default_attributes = pps_page_accessible_get_default_attributes;
	iface->get_character_extents = pps_page_accessible_get_character_extents;
	iface->get_offset_at_point = pps_page_accessible_get_offset_at_point;
#if ATK_CHECK_VERSION(2, 32, 0)
	iface->scroll_substring_to = pps_page_accessible_scroll_substring_to;
	iface->scroll_substring_to_point = pps_page_accessible_scroll_substring_to_point;
#endif
}

static GHashTable *
pps_page_accessible_get_links (PpsPageAccessible *accessible)
{
	PpsPageAccessiblePrivate *priv = accessible->priv;

	if (priv->links)
		return priv->links;

	priv->links = g_hash_table_new_full (g_direct_hash,
	                                     g_direct_equal,
	                                     NULL,
	                                     (GDestroyNotify) g_object_unref);
	return priv->links;
}

static AtkHyperlink *
pps_page_accessible_get_link (AtkHypertext *hypertext,
                              gint link_index)
{
	GHashTable *links;
	PpsMappingList *link_mapping;
	gint n_links;
	PpsMapping *mapping;
	PpsLinkAccessible *atk_link;
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (hypertext);
	PpsView *view = pps_page_accessible_get_view (self);

	if (link_index < 0)
		return NULL;

	if (!PPS_IS_DOCUMENT_LINKS (view->document))
		return NULL;

	links = pps_page_accessible_get_links (PPS_PAGE_ACCESSIBLE (hypertext));

	atk_link = g_hash_table_lookup (links, GINT_TO_POINTER (link_index));
	if (atk_link)
		return atk_hyperlink_impl_get_hyperlink (ATK_HYPERLINK_IMPL (atk_link));

	link_mapping = pps_page_cache_get_link_mapping (view->page_cache, self->priv->page);
	if (!link_mapping)
		return NULL;

	n_links = pps_mapping_list_length (link_mapping);
	if (link_index > n_links - 1)
		return NULL;

	mapping = pps_mapping_list_nth (link_mapping, n_links - link_index - 1);
	atk_link = pps_link_accessible_new (PPS_PAGE_ACCESSIBLE (hypertext),
	                                    PPS_LINK (mapping->data),
	                                    &mapping->area);
	g_hash_table_insert (links, GINT_TO_POINTER (link_index), atk_link);

	return atk_hyperlink_impl_get_hyperlink (ATK_HYPERLINK_IMPL (atk_link));
}

static gint
pps_page_accessible_get_n_links (AtkHypertext *hypertext)
{
	PpsMappingList *link_mapping;
	PpsPageAccessible *self = PPS_PAGE_ACCESSIBLE (hypertext);
	PpsView *view = pps_page_accessible_get_view (self);

	if (!PPS_IS_DOCUMENT_LINKS (view->document))
		return 0;

	link_mapping = pps_page_cache_get_link_mapping (view->page_cache,
	                                                self->priv->page);

	return link_mapping ? pps_mapping_list_length (link_mapping) : 0;
}

static gint
pps_page_accessible_get_link_index (AtkHypertext *hypertext,
                                    gint offset)
{
	guint i;
	gint n_links = pps_page_accessible_get_n_links (hypertext);

	for (i = 0; i < n_links; i++) {
		AtkHyperlink *hyperlink;
		gint start_index, end_index;

		hyperlink = pps_page_accessible_get_link (hypertext, i);
		start_index = atk_hyperlink_get_start_index (hyperlink);
		end_index = atk_hyperlink_get_end_index (hyperlink);

		if (start_index <= offset && end_index >= offset)
			return i;
	}

	return -1;
}

static void
pps_page_accessible_hypertext_iface_init (AtkHypertextIface *iface)
{
	iface->get_link = pps_page_accessible_get_link;
	iface->get_n_links = pps_page_accessible_get_n_links;
	iface->get_link_index = pps_page_accessible_get_link_index;
}

static void
pps_page_accessible_get_extents (AtkComponent *atk_component,
                                 gint *x,
                                 gint *y,
                                 gint *width,
                                 gint *height,
                                 AtkCoordType coord_type)
{
	PpsPageAccessible *self;
	PpsView *view;
	GdkRectangle page_area;
	PpsRectangle doc_rect, atk_rect;

	self = PPS_PAGE_ACCESSIBLE (atk_component);
	view = pps_page_accessible_get_view (self);
	pps_view_get_page_extents (view, self->priv->page, &page_area);

	doc_rect.x1 = page_area.x;
	doc_rect.y1 = page_area.y;
	doc_rect.x2 = page_area.x + page_area.width;
	doc_rect.y2 = page_area.y + page_area.height;
	_transform_doc_rect_to_atk_rect (self->priv->view_accessible, self->priv->page, &doc_rect, &atk_rect, coord_type);

	*x = atk_rect.x1;
	*y = atk_rect.y1;
	*width = atk_rect.x2 - atk_rect.x1;
	*height = atk_rect.y2 - atk_rect.y1;
}

static void
pps_page_accessible_component_iface_init (AtkComponentIface *iface)
{
	iface->get_extents = pps_page_accessible_get_extents;
}

static void
page_cached_cb (PpsPageCache *cache,
                gint page,
                PpsPageAccessible *self)
{
	if (page == self->priv->page)
		pps_page_accessible_initialize_children (self);
}

PpsPageAccessible *
pps_page_accessible_new (PpsViewAccessible *view_accessible,
                         gint page)
{
	PpsPageAccessible *atk_page;
	PpsView *view;

	g_return_val_if_fail (PPS_IS_VIEW_ACCESSIBLE (view_accessible), NULL);
	g_return_val_if_fail (page >= 0, NULL);

	atk_page = g_object_new (PPS_TYPE_PAGE_ACCESSIBLE,
	                         "view-accessible", view_accessible,
	                         "page", page,
	                         NULL);

	view = pps_page_accessible_get_view (PPS_PAGE_ACCESSIBLE (atk_page));
	if (pps_page_cache_is_page_cached (view->page_cache, page))
		pps_page_accessible_initialize_children (PPS_PAGE_ACCESSIBLE (atk_page));
	else
		g_signal_connect (view->page_cache, "page-cached",
		                  G_CALLBACK (page_cached_cb),
		                  atk_page);

	return PPS_PAGE_ACCESSIBLE (atk_page);
}

AtkObject *
pps_page_accessible_get_accessible_for_mapping (PpsPageAccessible *page_accessible,
                                                PpsMapping *mapping)
{
	gint i;

	pps_page_accessible_initialize_children (page_accessible);
	if (!mapping || !page_accessible->priv->children)
		return NULL;

	for (i = 0; i < page_accessible->priv->children->len; i++) {
		AtkObject *child;

		child = g_ptr_array_index (page_accessible->priv->children, i);
		if (PPS_IS_FORM_FIELD_ACCESSIBLE (child) &&
		    pps_form_field_accessible_get_field (PPS_FORM_FIELD_ACCESSIBLE (child)) == mapping->data)
			return child;
	}

	return NULL;
}

void
pps_page_accessible_update_element_state (PpsPageAccessible *page_accessible,
                                          PpsMapping *mapping)
{
	AtkObject *child;

	child = pps_page_accessible_get_accessible_for_mapping (page_accessible, mapping);
	if (!child)
		return;

	if (PPS_IS_FORM_FIELD_ACCESSIBLE (child))
		pps_form_field_accessible_update_state (PPS_FORM_FIELD_ACCESSIBLE (child));
}
