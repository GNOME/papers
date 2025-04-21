// SPDX-FileCopyrightText: 2009 Carlos Garcia Campos <carlosgc@gnome.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <adwaita.h>
#include <papers-document.h>
#include <papers-view.h>

G_BEGIN_DECLS

#define PPS_TYPE_PREVIEWER_WINDOW (pps_previewer_window_get_type ())
G_DECLARE_FINAL_TYPE (PpsPreviewerWindow, pps_previewer_window, PPS, PREVIEWER_WINDOW, AdwApplicationWindow)

struct _PpsPreviewerWindow {
	AdwApplicationWindow base_instance;
};

PpsPreviewerWindow *pps_previewer_window_new (void);

void pps_previewer_window_set_job (PpsPreviewerWindow *window,
                                   PpsJob *job);
gboolean pps_previewer_window_set_print_settings (PpsPreviewerWindow *window,
                                                  const gchar *print_settings,
                                                  GError **error);
gboolean pps_previewer_window_set_print_settings_fd (PpsPreviewerWindow *window,
                                                     int fd,
                                                     GError **error);
void pps_previewer_window_set_source_file (PpsPreviewerWindow *window,
                                           const gchar *source_file);
gboolean pps_previewer_window_set_source_fd (PpsPreviewerWindow *window,
                                             int fd,
                                             GError **error);

G_END_DECLS
