/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia
 * Author: Joanmarie Diggs <jdiggs@igalia.com>
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

#include <config.h>

#include "pps-form-field-accessible.h"
#include "pps-view-private.h"

struct _PpsFormFieldAccessiblePrivate {
	PpsPageAccessible *page;
	PpsFormField *form_field;
	PpsRectangle area;

	gchar *name;
	gint start_index;
	gint end_index;

	AtkStateSet *saved_states;
};

static void pps_form_field_accessible_component_iface_init (AtkComponentIface *iface);

G_DEFINE_TYPE_WITH_CODE (PpsFormFieldAccessible, pps_form_field_accessible, ATK_TYPE_OBJECT, G_ADD_PRIVATE (PpsFormFieldAccessible) G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, pps_form_field_accessible_component_iface_init))

static void
pps_form_field_accessible_get_extents (AtkComponent *atk_component,
                                       gint *x,
                                       gint *y,
                                       gint *width,
                                       gint *height,
                                       AtkCoordType coord_type)
{
	PpsFormFieldAccessible *self;
	PpsViewAccessible *view_accessible;
	gint page;
	PpsRectangle atk_rect;

	self = PPS_FORM_FIELD_ACCESSIBLE (atk_component);
	view_accessible = pps_page_accessible_get_view_accessible (self->priv->page);
	page = pps_page_accessible_get_page (self->priv->page);
	_transform_doc_rect_to_atk_rect (view_accessible, page, &self->priv->area, &atk_rect, coord_type);
	*x = atk_rect.x1;
	*y = atk_rect.y1;
	*width = atk_rect.x2 - atk_rect.x1;
	*height = atk_rect.y2 - atk_rect.y1;
}

static gboolean
pps_form_field_accessible_grab_focus (AtkComponent *atk_component)
{
	PpsFormFieldAccessible *self;
	PpsView *view;

	self = PPS_FORM_FIELD_ACCESSIBLE (atk_component);
	view = pps_page_accessible_get_view (self->priv->page);
	_pps_view_focus_form_field (view, self->priv->form_field);

	return TRUE;
}

static void
pps_form_field_accessible_component_iface_init (AtkComponentIface *iface)
{
	iface->get_extents = pps_form_field_accessible_get_extents;
	iface->grab_focus = pps_form_field_accessible_grab_focus;
}

static gboolean
get_indices_in_parent (AtkObject *atk_object,
                       gint *start,
                       gint *end)
{
	PpsFormFieldAccessiblePrivate *priv;
	PpsView *view;
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	gint last_zero_sized_index = -1;
	gint i;

	priv = PPS_FORM_FIELD_ACCESSIBLE (atk_object)->priv;
	if (priv->start_index != -1 && priv->end_index != -1) {
		*start = priv->start_index;
		*end = priv->end_index;
		return TRUE;
	}

	view = pps_page_accessible_get_view (priv->page);
	if (!view->page_cache)
		return FALSE;

	pps_page_cache_get_text_layout (view->page_cache,
	                                pps_page_accessible_get_page (priv->page),
	                                &areas, &n_areas);
	if (!areas)
		return FALSE;

	for (i = 0; i < n_areas; i++) {
		PpsRectangle *rect = areas + i;
		gdouble c_x, c_y;

		c_x = rect->x1 + (rect->x2 - rect->x1) / 2.;
		c_y = rect->y1 + (rect->y2 - rect->y1) / 2.;

		if (c_x >= priv->area.x1 && c_x <= priv->area.x2 &&
		    c_y >= priv->area.y1 && c_y <= priv->area.y2) {
			priv->start_index = i;
			break;
		}
	}

	if (priv->start_index == -1)
		return FALSE;

	for (i = priv->start_index + 1; i < n_areas; i++) {
		PpsRectangle *rect = areas + i;
		gdouble c_x, c_y;

		/* A zero-sized text rect suggests a line break. If it is within the text of the
		 * field, we want to preserve it; if it is the character immediately after, we
		 * do not. We won't know which it is until we find the first text rect that is
		 * outside of the area occupied by the field.
		 */
		if (rect->y1 == rect->y2) {
			last_zero_sized_index = i;
			continue;
		}

		c_x = rect->x1 + (rect->x2 - rect->x1) / 2.;
		c_y = rect->y1 + (rect->y2 - rect->y1) / 2.;

		if (c_x < priv->area.x1 || c_x > priv->area.x2 ||
		    c_y < priv->area.y1 || c_y > priv->area.y2) {
			priv->end_index = last_zero_sized_index + 1 == i ? i - 1 : i;
			break;
		}
	}

	if (priv->end_index == -1)
		return FALSE;

	*start = priv->start_index;
	*end = priv->end_index;
	return TRUE;
}

static gchar *
get_text_under_element (AtkObject *atk_object)
{
	gint start = -1;
	gint end = -1;

	if (get_indices_in_parent (atk_object, &start, &end) && start != end)
		return atk_text_get_text (ATK_TEXT (atk_object_get_parent (atk_object)), start, end);

	return NULL;
}

static const gchar *
pps_form_field_accessible_get_name (AtkObject *atk_object)
{
	PpsFormFieldAccessiblePrivate *priv;

	priv = PPS_FORM_FIELD_ACCESSIBLE (atk_object)->priv;
	if (priv->name)
		return priv->name;

	if (PPS_IS_FORM_FIELD_BUTTON (priv->form_field)) {
		PpsFormFieldButton *button = PPS_FORM_FIELD_BUTTON (priv->form_field);

		if (button->type == PPS_FORM_FIELD_BUTTON_PUSH)
			priv->name = get_text_under_element (atk_object);
	}

	return priv->name;
}

static AtkObject *
pps_form_field_accessible_get_parent (AtkObject *atk_object)
{
	PpsFormFieldAccessiblePrivate *priv = PPS_FORM_FIELD_ACCESSIBLE (atk_object)->priv;

	return ATK_OBJECT (priv->page);
}

static AtkRole
pps_form_field_accessible_get_role (AtkObject *atk_object)
{
	PpsFormField *pps_form_field;

	pps_form_field = PPS_FORM_FIELD_ACCESSIBLE (atk_object)->priv->form_field;
	if (PPS_IS_FORM_FIELD_BUTTON (pps_form_field)) {
		PpsFormFieldButton *field_button = PPS_FORM_FIELD_BUTTON (pps_form_field);

		switch (field_button->type) {
		case PPS_FORM_FIELD_BUTTON_CHECK:
			return ATK_ROLE_CHECK_BOX;
		case PPS_FORM_FIELD_BUTTON_RADIO:
			return ATK_ROLE_RADIO_BUTTON;
		case PPS_FORM_FIELD_BUTTON_PUSH:
			return ATK_ROLE_PUSH_BUTTON;
		default:
			return ATK_ROLE_UNKNOWN;
		}
	}

	if (PPS_IS_FORM_FIELD_CHOICE (pps_form_field)) {
		PpsFormFieldChoice *field_choice = PPS_FORM_FIELD_CHOICE (pps_form_field);

		switch (field_choice->type) {
		case PPS_FORM_FIELD_CHOICE_COMBO:
			return ATK_ROLE_COMBO_BOX;
		case PPS_FORM_FIELD_CHOICE_LIST:
			return ATK_ROLE_LIST_BOX;
		default:
			return ATK_ROLE_UNKNOWN;
		}
	}

	if (PPS_IS_FORM_FIELD_TEXT (pps_form_field)) {
		PpsFormFieldText *field_text = PPS_FORM_FIELD_TEXT (pps_form_field);

		if (field_text->is_password)
			return ATK_ROLE_PASSWORD_TEXT;
		else if (field_text->type == PPS_FORM_FIELD_TEXT_MULTILINE)
			return ATK_ROLE_TEXT;
		else
			return ATK_ROLE_ENTRY;
	}

	return ATK_ROLE_UNKNOWN;
}

static AtkStateSet *
pps_form_field_accessible_ref_state_set (AtkObject *atk_object)
{
	AtkStateSet *state_set;
	AtkStateSet *copy_set;
	AtkStateSet *page_accessible_state_set;
	PpsFormFieldAccessible *self;
	PpsFormField *pps_form_field;
	PpsViewAccessible *view_accessible;
	gint page;

	self = PPS_FORM_FIELD_ACCESSIBLE (atk_object);
	state_set = ATK_OBJECT_CLASS (pps_form_field_accessible_parent_class)->ref_state_set (atk_object);
	atk_state_set_clear_states (state_set);

	page_accessible_state_set = atk_object_ref_state_set (ATK_OBJECT (self->priv->page));
	copy_set = atk_state_set_or_sets (state_set, page_accessible_state_set);

	view_accessible = pps_page_accessible_get_view_accessible (self->priv->page);
	page = pps_page_accessible_get_page (self->priv->page);
	if (!pps_view_accessible_is_doc_rect_showing (view_accessible, page, &self->priv->area))
		atk_state_set_remove_state (copy_set, ATK_STATE_SHOWING);

	pps_form_field = PPS_FORM_FIELD_ACCESSIBLE (atk_object)->priv->form_field;
	if (PPS_IS_FORM_FIELD_BUTTON (pps_form_field)) {
		PpsFormFieldButton *field_button = PPS_FORM_FIELD_BUTTON (pps_form_field);

		if (field_button->state) {
			if (field_button->type == PPS_FORM_FIELD_BUTTON_PUSH)
				atk_state_set_add_state (copy_set, ATK_STATE_PRESSED);
			else
				atk_state_set_add_state (copy_set, ATK_STATE_CHECKED);
		}
	}

	else if (PPS_IS_FORM_FIELD_CHOICE (pps_form_field)) {
		PpsFormFieldChoice *field_choice = PPS_FORM_FIELD_CHOICE (pps_form_field);

		if (field_choice->is_editable && !pps_form_field->is_read_only)
			atk_state_set_add_state (copy_set, ATK_STATE_EDITABLE);
		if (field_choice->multi_select)
			atk_state_set_add_state (copy_set, ATK_STATE_MULTISELECTABLE);
	}

	else if (PPS_IS_FORM_FIELD_TEXT (pps_form_field)) {
		PpsFormFieldText *field_text = PPS_FORM_FIELD_TEXT (pps_form_field);

		if (!pps_form_field->is_read_only)
			atk_state_set_add_state (copy_set, ATK_STATE_EDITABLE);
		if (field_text->type == PPS_FORM_FIELD_TEXT_NORMAL)
			atk_state_set_add_state (copy_set, ATK_STATE_SINGLE_LINE);
		else if (field_text->type == PPS_FORM_FIELD_TEXT_MULTILINE)
			atk_state_set_add_state (copy_set, ATK_STATE_MULTI_LINE);
	}

	g_object_unref (state_set);
	g_object_unref (page_accessible_state_set);

	return copy_set;
}

void
pps_form_field_accessible_update_state (PpsFormFieldAccessible *accessible)
{
	AtkObject *atk_object;
	AtkStateSet *states;
	AtkStateSet *changed_states;
	gint i;

	atk_object = ATK_OBJECT (accessible);
	states = pps_form_field_accessible_ref_state_set (atk_object);
	changed_states = atk_state_set_xor_sets (accessible->priv->saved_states, states);
	if (changed_states && !atk_state_set_is_empty (accessible->priv->saved_states)) {
		for (i = 0; i < ATK_STATE_LAST_DEFINED; i++) {
			if (atk_state_set_contains_state (changed_states, i))
				atk_object_notify_state_change (atk_object, i, atk_state_set_contains_state (states, i));
		}
	}

	g_object_unref (accessible->priv->saved_states);

	atk_state_set_clear_states (changed_states);
	accessible->priv->saved_states = atk_state_set_or_sets (changed_states, states);

	g_object_unref (changed_states);
	g_object_unref (states);
}

static void
pps_form_field_accessible_finalize (GObject *object)
{
	PpsFormFieldAccessiblePrivate *priv = PPS_FORM_FIELD_ACCESSIBLE (object)->priv;

	g_object_unref (priv->form_field);
	g_free (priv->name);
	g_object_unref (priv->saved_states);

	G_OBJECT_CLASS (pps_form_field_accessible_parent_class)->finalize (object);
}

static void
pps_form_field_accessible_class_init (PpsFormFieldAccessibleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	object_class->finalize = pps_form_field_accessible_finalize;
	atk_class->get_name = pps_form_field_accessible_get_name;
	atk_class->get_parent = pps_form_field_accessible_get_parent;
	atk_class->get_role = pps_form_field_accessible_get_role;
	atk_class->ref_state_set = pps_form_field_accessible_ref_state_set;
}

static void
pps_form_field_accessible_init (PpsFormFieldAccessible *accessible)
{
	accessible->priv = pps_form_field_accessible_get_instance_private (accessible);
	accessible->priv->start_index = -1;
	accessible->priv->end_index = -1;
}

PpsFormFieldAccessible *
pps_form_field_accessible_new (PpsPageAccessible *page,
                               PpsFormField *form_field,
                               PpsRectangle *area)
{
	PpsFormFieldAccessible *atk_form_field;

	atk_form_field = g_object_new (PPS_TYPE_FORM_FIELD_ACCESSIBLE, NULL);
	atk_form_field->priv->page = page;
	atk_form_field->priv->form_field = g_object_ref (form_field);
	atk_form_field->priv->area = *area;
	atk_form_field->priv->saved_states = atk_state_set_new ();
	pps_form_field_accessible_update_state (atk_form_field);

	return PPS_FORM_FIELD_ACCESSIBLE (atk_form_field);
}

PpsFormField *
pps_form_field_accessible_get_field (PpsFormFieldAccessible *accessible)
{
	return accessible->priv->form_field;
}
