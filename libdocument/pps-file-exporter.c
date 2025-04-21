// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Martin Kretzschmar
 *
 *  Author:
 *    Martin Kretzschmar <martink@gnome.org>
 */

#include "pps-file-exporter.h"
#include "pps-document.h"
#include <config.h>

G_DEFINE_INTERFACE (PpsFileExporter, pps_file_exporter, 0)

static void
pps_file_exporter_default_init (PpsFileExporterInterface *klass)
{
}

void
pps_file_exporter_begin (PpsFileExporter *exporter,
                         PpsFileExporterContext *fc)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	iface->begin (exporter, fc);
}

void
pps_file_exporter_begin_page (PpsFileExporter *exporter)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	if (iface->begin_page)
		iface->begin_page (exporter);
}

void
pps_file_exporter_do_page (PpsFileExporter *exporter,
                           PpsRenderContext *rc)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	iface->do_page (exporter, rc);
}

void
pps_file_exporter_end_page (PpsFileExporter *exporter)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	if (iface->end_page)
		iface->end_page (exporter);
}

void
pps_file_exporter_end (PpsFileExporter *exporter)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	iface->end (exporter);
}

PpsFileExporterCapabilities
pps_file_exporter_get_capabilities (PpsFileExporter *exporter)
{
	PpsFileExporterInterface *iface = PPS_FILE_EXPORTER_GET_IFACE (exporter);

	return iface->get_capabilities (exporter);
}
