/*
 *  Copyright (C) 2000-2003 Marco Pesenti Gritti
 *  Copyright © 2021 Christian Persch
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#if !defined (__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-macros.h"

G_BEGIN_DECLS

typedef struct _PpsDocumentInfo    PpsDocumentInfo;
typedef struct _PpsDocumentLicense PpsDocumentLicense;

#define PPS_TYPE_DOCUMENT_INFO (pps_document_info_get_type())

typedef enum
{
	PPS_DOCUMENT_LAYOUT_SINGLE_PAGE,
	PPS_DOCUMENT_LAYOUT_ONE_COLUMN,
	PPS_DOCUMENT_LAYOUT_TWO_COLUMN_LEFT,
	PPS_DOCUMENT_LAYOUT_TWO_COLUMN_RIGHT,
	PPS_DOCUMENT_LAYOUT_TWO_PAGE_LEFT,
	PPS_DOCUMENT_LAYOUT_TWO_PAGE_RIGHT
} PpsDocumentLayout;

typedef enum
{
	PPS_DOCUMENT_MODE_NONE,
	PPS_DOCUMENT_MODE_USE_OC,
	PPS_DOCUMENT_MODE_USE_THUMBS,
	PPS_DOCUMENT_MODE_FULL_SCREEN,
	PPS_DOCUMENT_MODE_USE_ATTACHMENTS,
	PPS_DOCUMENT_MODE_PRESENTATION = PPS_DOCUMENT_MODE_FULL_SCREEN /* Will these be different? */
} PpsDocumentMode;

typedef enum
{
	PPS_DOCUMENT_CONTAINS_JS_UNKNOWN,
	PPS_DOCUMENT_CONTAINS_JS_NO,
	PPS_DOCUMENT_CONTAINS_JS_YES
} PpsDocumentContainsJS;

typedef enum
{
	PPS_DOCUMENT_UI_HINT_HIDE_TOOLBAR = 1 << 0,
	PPS_DOCUMENT_UI_HINT_HIDE_MENUBAR = 1 << 1,
	PPS_DOCUMENT_UI_HINT_HIDE_WINDOWUI = 1 << 2,
	PPS_DOCUMENT_UI_HINT_FIT_WINDOW = 1 << 3,
	PPS_DOCUMENT_UI_HINT_CENTER_WINDOW = 1 << 4,
	PPS_DOCUMENT_UI_HINT_DISPLAY_DOC_TITLE = 1 << 5,
	PPS_DOCUMENT_UI_HINT_DIRECTION_RTL = 1 << 6
} PpsDocumentUIHints;

/* This define is needed because glib-mkenums chokes with multiple lines */
#define _PERMISSIONS_FULL (PPS_DOCUMENT_PERMISSIONS_OK_TO_PRINT  \
			 | PPS_DOCUMENT_PERMISSIONS_OK_TO_MODIFY \
			 | PPS_DOCUMENT_PERMISSIONS_OK_TO_COPY   \
			 | PPS_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES)

typedef enum
{
	PPS_DOCUMENT_PERMISSIONS_OK_TO_PRINT = 1 << 0,
	PPS_DOCUMENT_PERMISSIONS_OK_TO_MODIFY = 1 << 1,
	PPS_DOCUMENT_PERMISSIONS_OK_TO_COPY = 1 << 2,
	PPS_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES = 1 << 3,
	PPS_DOCUMENT_PERMISSIONS_FULL = _PERMISSIONS_FULL
} PpsDocumentPermissions;

typedef enum
{
	PPS_DOCUMENT_INFO_TITLE = 1 << 0,
	PPS_DOCUMENT_INFO_FORMAT = 1 << 1,
	PPS_DOCUMENT_INFO_AUTHOR = 1 << 2,
	PPS_DOCUMENT_INFO_SUBJECT = 1 << 3,
	PPS_DOCUMENT_INFO_KEYWORDS = 1 << 4,
	PPS_DOCUMENT_INFO_LAYOUT = 1 << 5,
	PPS_DOCUMENT_INFO_CREATOR = 1 << 6,
	PPS_DOCUMENT_INFO_PRODUCER = 1 << 7,
	PPS_DOCUMENT_INFO_CREATION_DATETIME = 1 << 8,
	PPS_DOCUMENT_INFO_MOD_DATETIME = 1 << 9,
	PPS_DOCUMENT_INFO_LINEARIZED = 1 << 10,
	PPS_DOCUMENT_INFO_START_MODE = 1 << 11,
	PPS_DOCUMENT_INFO_UI_HINTS = 1 << 12,
	PPS_DOCUMENT_INFO_PERMISSIONS = 1 << 13,
	PPS_DOCUMENT_INFO_N_PAGES = 1 << 14,
	PPS_DOCUMENT_INFO_SECURITY = 1 << 15,
	PPS_DOCUMENT_INFO_PAPER_SIZE = 1 << 16,
	PPS_DOCUMENT_INFO_LICENSE = 1 << 17,
	PPS_DOCUMENT_INFO_CONTAINS_JS = 1 << 18
} PpsDocumentInfoFields;

struct _PpsDocumentInfo
{
	char *title;
	char *format; /* eg, "pdf-1.5" */
	char *author;
	char *subject;
	char *keywords;
	char *creator;
	char *producer;
	char *linearized;
	char *security;
	GDateTime *creation_datetime;
	GDateTime *modified_datetime;
	PpsDocumentLayout layout;
	PpsDocumentMode mode;
	guint ui_hints;
	guint permissions;
	int   n_pages;
	double paper_height;
	double paper_width;
	PpsDocumentLicense *license;
	PpsDocumentContainsJS contains_js; /* wheter it contains any javascript */

	/* Mask of all the valid fields */
	guint fields_mask;
};

PPS_PUBLIC
GType           pps_document_info_get_type (void) G_GNUC_CONST;
PPS_PUBLIC
PpsDocumentInfo* pps_document_info_new      (void);
PPS_PUBLIC
PpsDocumentInfo *pps_document_info_copy     (PpsDocumentInfo *info);
PPS_PUBLIC
void            pps_document_info_free     (PpsDocumentInfo *info);
PPS_PUBLIC
GDateTime      *pps_document_info_get_created_datetime   (const PpsDocumentInfo *info);
PPS_PUBLIC
GDateTime      *pps_document_info_get_modified_datetime  (const PpsDocumentInfo *info);

PPS_PRIVATE
void            pps_document_info_take_created_datetime  (PpsDocumentInfo *info,
                                                         GDateTime      *datetime);
PPS_PRIVATE
void            pps_document_info_take_modified_datetime (PpsDocumentInfo *info,
                                                         GDateTime      *datetime);
PPS_PRIVATE
gboolean        pps_document_info_set_from_xmp           (PpsDocumentInfo *info,
                                                         const char     *xmp,
                                                         gssize          size);

/* PpsDocumentLicense */
#define PPS_TYPE_DOCUMENT_LICENSE (pps_document_license_get_type())
struct _PpsDocumentLicense {
	gchar *text;
	gchar *uri;
	gchar *web_statement;
};
PPS_PUBLIC
GType              pps_document_license_get_type          (void) G_GNUC_CONST;
PPS_PUBLIC
PpsDocumentLicense *pps_document_license_new               (void);
PPS_PUBLIC
PpsDocumentLicense *pps_document_license_copy              (PpsDocumentLicense *license);
PPS_PUBLIC
void               pps_document_license_free              (PpsDocumentLicense *license);
PPS_PUBLIC
const gchar       *pps_document_license_get_text          (PpsDocumentLicense *license);
PPS_PUBLIC
const gchar       *pps_document_license_get_uri           (PpsDocumentLicense *license);
PPS_PUBLIC
const gchar       *pps_document_license_get_web_statement (PpsDocumentLicense *license);

G_END_DECLS
