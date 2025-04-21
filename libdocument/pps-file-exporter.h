// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Martin Kretzschmar
 *
 *  Author:
 *    Martin Kretzschmar <martink@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-macros.h"
#include "pps-render-context.h"

G_BEGIN_DECLS

typedef enum {
	PPS_FILE_FORMAT_UNKNOWN,
	PPS_FILE_FORMAT_PS,
	PPS_FILE_FORMAT_PDF
} PpsFileExporterFormat;

typedef enum {
	PPS_FILE_EXPORTER_CAN_PAGE_SET = 1 << 0,
	PPS_FILE_EXPORTER_CAN_COPIES = 1 << 1,
	PPS_FILE_EXPORTER_CAN_COLLATE = 1 << 2,
	PPS_FILE_EXPORTER_CAN_REVERSE = 1 << 3,
	PPS_FILE_EXPORTER_CAN_SCALE = 1 << 4,
	PPS_FILE_EXPORTER_CAN_GENERATE_PDF = 1 << 5,
	PPS_FILE_EXPORTER_CAN_GENERATE_PS = 1 << 6,
	PPS_FILE_EXPORTER_CAN_PREVIEW = 1 << 7,
	PPS_FILE_EXPORTER_CAN_NUMBER_UP = 1 << 8
} PpsFileExporterCapabilities;

typedef struct _PpsFileExporterContext PpsFileExporterContext;

struct _PpsFileExporterContext {
	PpsFileExporterFormat format;
	const gchar *filename;
	gint first_page;
	gint last_page;
	gdouble paper_width;
	gdouble paper_height;
	gboolean duplex;
	gint pages_per_sheet;
};

#define PPS_TYPE_FILE_EXPORTER (pps_file_exporter_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsFileExporter, pps_file_exporter, PPS, FILE_EXPORTER, GObject)

struct _PpsFileExporterInterface {
	GTypeInterface base_iface;

	/* Methods  */
	void (*begin) (PpsFileExporter *exporter,
	               PpsFileExporterContext *fc);
	void (*begin_page) (PpsFileExporter *exporter);
	void (*do_page) (PpsFileExporter *exporter,
	                 PpsRenderContext *rc);
	void (*end_page) (PpsFileExporter *exporter);
	void (*end) (PpsFileExporter *exporter);
	PpsFileExporterCapabilities (*get_capabilities) (PpsFileExporter *exporter);
};

PPS_PUBLIC
void pps_file_exporter_begin (PpsFileExporter *exporter,
                              PpsFileExporterContext *fc);
PPS_PUBLIC
void pps_file_exporter_begin_page (PpsFileExporter *exporter);
PPS_PUBLIC
void pps_file_exporter_do_page (PpsFileExporter *exporter,
                                PpsRenderContext *rc);
PPS_PUBLIC
void pps_file_exporter_end_page (PpsFileExporter *exporter);
PPS_PUBLIC
void pps_file_exporter_end (PpsFileExporter *exporter);
PPS_PUBLIC
PpsFileExporterCapabilities pps_file_exporter_get_capabilities (PpsFileExporter *exporter);

G_END_DECLS
