// SPDX-License-Identifier: GPL-3.0-or-later
/* pps-colors.c
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
 */

#include "pps-colors.h"

#include <adwaita.h>

void
get_accent_color (GdkRGBA *background_color, GdkRGBA *foreground_color)
{
	if (background_color) {
		AdwStyleManager *style_manager = adw_style_manager_get_default ();
		AdwAccentColor accent = adw_style_manager_get_accent_color (style_manager);
		adw_accent_color_to_rgba (accent, background_color);
	}

	if (foreground_color) {
		foreground_color->red = 1.0;
		foreground_color->green = 1.0;
		foreground_color->blue = 1.0;
	}
}
