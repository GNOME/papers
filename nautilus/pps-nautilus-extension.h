// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Red Hat, Inc
 *  Copyright (C) 2022 Qiu Wenbo <qiuwenbo@kylinos.com.cn>
 */

#pragma once

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <papers-document.h>

G_BEGIN_DECLS

typedef enum {
	TITLE_PROPERTY,
	URI_PROPERTY,
	SUBJECT_PROPERTY,
	AUTHOR_PROPERTY,
	KEYWORDS_PROPERTY,
	PRODUCER_PROPERTY,
	CREATOR_PROPERTY,
	CREATION_DATE_PROPERTY,
	MOD_DATE_PROPERTY,
	N_PAGES_PROPERTY,
	LINEARIZED_PROPERTY,
	FORMAT_PROPERTY,
	SECURITY_PROPERTY,
	CONTAINS_JS_PROPERTY,
	PAPER_SIZE_PROPERTY,
	FILE_SIZE_PROPERTY,
	N_PROPERTIES,
} Property;

typedef struct
{
	Property property;
	const char *label;
} PropertyInfo;

static const PropertyInfo properties_info[] = {
	{ TITLE_PROPERTY, N_ ("Title") },
	{ URI_PROPERTY, N_ ("Location") },
	{ SUBJECT_PROPERTY, N_ ("Subject") },
	{ AUTHOR_PROPERTY, N_ ("Author") },
	{ KEYWORDS_PROPERTY, N_ ("Keywords") },
	{ PRODUCER_PROPERTY, N_ ("Producer") },
	{ CREATOR_PROPERTY, N_ ("Creator") },
	{ CREATION_DATE_PROPERTY, N_ ("Created") },
	{ MOD_DATE_PROPERTY, N_ ("Modified") },
	{ N_PAGES_PROPERTY, N_ ("Number of Pages") },
	{ LINEARIZED_PROPERTY, N_ ("Optimized") },
	{ FORMAT_PROPERTY, N_ ("Format") },
	{ SECURITY_PROPERTY, N_ ("Security") },
	{ CONTAINS_JS_PROPERTY, N_ ("Contains Javascript") },
	{ PAPER_SIZE_PROPERTY, N_ ("Paper Size") },
	{ FILE_SIZE_PROPERTY, N_ ("Size") }
};

#define PPS_TYPE_DOCUMENT_PROPERTIES_MODEL_PROVIDER (pps_document_properties_model_provider_get_type ())

G_DECLARE_FINAL_TYPE (PpsDocumentPropertiesModelProvider,
                      pps_document_properties_model_provider,
                      PPS,
                      DOCUMENT_PROPERTIES_MODEL_PROVIDER,
                      GObject)

G_END_DECLS
