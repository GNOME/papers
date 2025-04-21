// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia
 * Author: Joanmarie Diggs <jdiggs@igalia.com>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-form-field.h"
#include "pps-page-accessible.h"

#define PPS_TYPE_FORM_FIELD_ACCESSIBLE (pps_form_field_accessible_get_type ())
#define PPS_FORM_FIELD_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_FORM_FIELD_ACCESSIBLE, PpsFormFieldAccessible))
#define PPS_IS_FORM_FIELD_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_FORM_FIELD_ACCESSIBLE))

typedef struct _PpsFormFieldAccessible PpsFormFieldAccessible;
typedef struct _PpsFormFieldAccessibleClass PpsFormFieldAccessibleClass;
typedef struct _PpsFormFieldAccessiblePrivate PpsFormFieldAccessiblePrivate;

struct _PpsFormFieldAccessible {
	AtkObject parent;
	PpsFormFieldAccessiblePrivate *priv;
};

struct _PpsFormFieldAccessibleClass {
	AtkObjectClass parent_class;
};

GType pps_form_field_accessible_get_type (void);
PpsFormFieldAccessible *pps_form_field_accessible_new (PpsPageAccessible *page,
                                                       PpsFormField *form_field,
                                                       PpsRectangle *area);
PpsFormField *pps_form_field_accessible_get_field (PpsFormFieldAccessible *accessible);
void pps_form_field_accessible_update_state (PpsFormFieldAccessible *accessible);
