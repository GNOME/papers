/* pps-attachment-context.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Markus Göllnitz  <camelcasenick@bewares.it>
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

#include <libdocument/pps-macros.h>
#if !defined (__PPS_PAPERS_VIEW_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>

#include <libview/context/pps-document-model.h>
#include <papers-document.h>

G_BEGIN_DECLS

#define PPS_TYPE_ATTACHMENT_CONTEXT    (pps_attachment_context_get_type())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAttachmentContext, pps_attachment_context, PPS, ATTACHMENT_CONTEXT, GObject)

struct _PpsAttachmentContext {
        GObject parent_instance;
};

struct _PpsAttachmentContextClass {
	GObjectClass parent_class;
};

#define PPS_ATTACHMENT_CONTEXT_ERROR pps_attachment_context_error_quark ()

typedef enum {
	PPS_ATTACHMENT_CONTEXT_ERROR_NOT_IMPLEMENTED,
	PPS_ATTACHMENT_CONTEXT_ERROR_EMPTY_INPUT,
} PpsAttachmentContextError;

PPS_PUBLIC
GQuark                pps_attachment_context_error_quark             (void);
PPS_PUBLIC
PpsAttachmentContext *pps_attachment_context_new                     (PpsDocumentModel *model);
PPS_PUBLIC
GListModel           *pps_attachment_context_get_model               (PpsAttachmentContext *context);
PPS_PUBLIC
void                  pps_attachment_context_save_attachments_async  (PpsAttachmentContext *context,
								      GListModel           *attachments,
								      GtkWindow            *parent,
								      GCancellable         *cancellable,
								      GAsyncReadyCallback   callback,
								      gpointer              user_data);
PPS_PUBLIC
gboolean              pps_attachment_context_save_attachments_finish (PpsAttachmentContext *context,
								      GAsyncResult         *result,
								      GError              **error);

G_END_DECLS
