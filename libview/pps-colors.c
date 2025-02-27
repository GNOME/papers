/* pps-colors.c
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
 *
 * Papers is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
