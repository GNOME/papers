// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

void pps_snapshot_append_page_texture (GtkSnapshot *snapshot,
                                       GdkTexture *texture,
                                       const graphene_rect_t *area,
                                       gboolean inverted);

G_END_DECLS
