// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-model.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-annotation-model.h"

struct _PpsAnnotationModel {
	GObject base;
	PpsAnnotationTool tool;
	gdouble radius[3];
	GdkRGBA color[3];
	PangoFontDescription *font_desc;
	gboolean eraser_objects;
};

enum {
	PROP_0,
	PROP_TOOL,
	PROP_ERASER_RADIUS,
	PROP_PEN_RADIUS,
	PROP_HIGHLIGHT_RADIUS,
	PROP_PEN_COLOR,
	PROP_HIGHLIGHT_COLOR,
	PROP_TEXT_COLOR,
	PROP_TEXT_FONT,
	PROP_TEXT_FONT_SIZE,
	PROP_ERASER_OBJECTS,
	PROP_ACTIVE_TOOL_STR,
	PROP_ANNOT_MODEL_LAST
};
static GParamSpec *model_properties[PROP_ANNOT_MODEL_LAST];

G_DEFINE_TYPE (PpsAnnotationModel, pps_annotation_model, G_TYPE_OBJECT)

/**
 * pps_annotation_model_set_tool:
 * @model: a #PpsAnnotationModel
 * @tool: the #PpsAnnotationTool to set
 *
 * Sets the current tool of the annotation model.
 *
 * Since: 48.0
 */
void
pps_annotation_model_set_tool (PpsAnnotationModel *model,
                               PpsAnnotationTool tool)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->tool = tool;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_TOOL]);
	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_ACTIVE_TOOL_STR]);
}

/**
 * pps_annotation_model_get_tool:
 * @model: a #PpsAnnotationModel
 *
 * Gets the current tool of the annotation model.
 *
 * Returns: the current #PpsAnnotationTool
 *
 * Since: 48.0
 */
PpsAnnotationTool
pps_annotation_model_get_tool (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), TOOL_PENCIL);

	return model->tool;
}

void
pps_annotation_model_set_eraser_radius (PpsAnnotationModel *model,
                                        gdouble radius)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->radius[TOOL_ERASER] = radius;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_ERASER_RADIUS]);
}

gdouble
pps_annotation_model_get_eraser_radius (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), 1.0);

	return model->radius[TOOL_ERASER];
}

gboolean
pps_annotation_model_get_eraser_objects (PpsAnnotationModel *model)
{
	return model->eraser_objects;
}

void
pps_annotation_model_set_highlight_radius (PpsAnnotationModel *model,
                                           gdouble radius)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->radius[TOOL_HIGHLIGHT] = radius;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_HIGHLIGHT_RADIUS]);
}

gdouble
pps_annotation_model_get_highlight_radius (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), 1.0);

	return model->radius[TOOL_HIGHLIGHT];
}
void
pps_annotation_model_set_pen_radius (PpsAnnotationModel *model,
                                     gdouble radius)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->radius[TOOL_PENCIL] = radius;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_PEN_RADIUS]);
}

gdouble
pps_annotation_model_get_pen_radius (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), 1.0);

	return model->radius[TOOL_PENCIL];
}

void
pps_annotation_model_set_pen_color (PpsAnnotationModel *model,
                                    const GdkRGBA *pen_color)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->color[TOOL_PENCIL] = *pen_color;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_PEN_COLOR]);
}

GdkRGBA *
pps_annotation_model_get_pen_color (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), NULL);

	return &model->color[TOOL_PENCIL];
}

void
pps_annotation_model_set_highlight_color (PpsAnnotationModel *model,
                                          const GdkRGBA *color)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->color[TOOL_HIGHLIGHT] = *color;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_HIGHLIGHT_COLOR]);
}

GdkRGBA *
pps_annotation_model_get_highlight_color (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), NULL);

	return &model->color[TOOL_HIGHLIGHT];
}

void
pps_annotation_model_set_text_color (PpsAnnotationModel *model,
                                     const GdkRGBA *color)
{
	g_return_if_fail (PPS_IS_ANNOTATION_MODEL (model));

	model->color[2] = *color;

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_TEXT_COLOR]);
}

GdkRGBA *
pps_annotation_model_get_text_color (PpsAnnotationModel *model)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_MODEL (model), NULL);

	return &model->color[2];
}

void
pps_annotation_model_set_font_desc (PpsAnnotationModel *model,
                                    const PangoFontDescription *font_desc)
{
	if (model->font_desc == font_desc ||
	    (model->font_desc && font_desc &&
	     pango_font_description_equal (model->font_desc, font_desc)))
		return;

	if (model->font_desc)
		pango_font_description_free (model->font_desc);

	model->font_desc = pango_font_description_copy (font_desc);

	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_TEXT_FONT]);
}

PangoFontDescription *
pps_annotation_model_get_font_desc (PpsAnnotationModel *model)
{
	return model->font_desc;
}

void
pps_annotation_model_set_font_size (PpsAnnotationModel *model,
                                    gdouble font_size)
{
	g_autoptr (PangoFontDescription) new_font_desc = pango_font_description_copy (model->font_desc);
	pango_font_description_set_size (new_font_desc, font_size * PANGO_SCALE);
	pps_annotation_model_set_font_desc (model, new_font_desc);
	g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_TEXT_FONT_SIZE]);
}

static void
pps_annotation_model_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
	PpsAnnotationModel *model = PPS_ANNOTATION_MODEL (object);

	switch (prop_id) {
	case PROP_TOOL:
		pps_annotation_model_set_tool (model, g_value_get_int (value));
		break;
	case PROP_ACTIVE_TOOL_STR: {
		const gchar *tool = g_value_get_string (value);
		if (!g_strcmp0 (tool, "pencil")) {
			pps_annotation_model_set_tool (model, TOOL_PENCIL);
		} else if (!g_strcmp0 (tool, "text")) {
			pps_annotation_model_set_tool (model, TOOL_TEXT);
		} else if (!g_strcmp0 (tool, "highlighter")) {
			pps_annotation_model_set_tool (model, TOOL_HIGHLIGHT);
		} else if (!g_strcmp0 (tool, "eraser")) {
			pps_annotation_model_set_tool (model, TOOL_ERASER);
		} else {
			g_critical ("unkonwn tool");
		}
		break;
	}
	case PROP_ERASER_RADIUS:
		pps_annotation_model_set_eraser_radius (model, g_value_get_double (value));
		break;
	case PROP_HIGHLIGHT_RADIUS:
		pps_annotation_model_set_highlight_radius (model, g_value_get_double (value));
		break;
	case PROP_PEN_RADIUS:
		pps_annotation_model_set_pen_radius (model, g_value_get_double (value));
		break;
	case PROP_PEN_COLOR:
		pps_annotation_model_set_pen_color (model, g_value_get_boxed (value));
		break;
	case PROP_TEXT_COLOR:
		pps_annotation_model_set_text_color (model, g_value_get_boxed (value));
		break;
	case PROP_HIGHLIGHT_COLOR:
		pps_annotation_model_set_highlight_color (model, g_value_get_boxed (value));
		break;
	case PROP_TEXT_FONT: {
		int current_font_size = pango_font_description_get_size (model->font_desc);
		PangoFontDescription *desc = g_value_get_boxed (value);
		pango_font_description_set_size (desc, current_font_size);
		pps_annotation_model_set_font_desc (model, desc);
		g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_TEXT_FONT]);
		break;
	}
	case PROP_TEXT_FONT_SIZE: {
		pps_annotation_model_set_font_size (model, g_value_get_double (value));
		break;
	}
	case PROP_ERASER_OBJECTS:
		model->eraser_objects = g_value_get_boolean (value);
		g_object_notify_by_pspec (G_OBJECT (model), model_properties[PROP_ERASER_OBJECTS]);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_model_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
	PpsAnnotationModel *model = PPS_ANNOTATION_MODEL (object);

	switch (prop_id) {
	case PROP_TOOL:
		g_value_set_int (value, model->tool);
		break;
	case PROP_ACTIVE_TOOL_STR:
		switch (model->tool) {
		case TOOL_PENCIL:
			g_value_set_string (value, "pencil");
			break;
		case TOOL_TEXT:
			g_value_set_string (value, "text");
			break;
		case TOOL_HIGHLIGHT:
			g_value_set_string (value, "highlighter");
			break;
		case TOOL_ERASER:
			g_value_set_string (value, "eraser");
			break;
		default:
			g_critical ("unknown tool");
			break;
		}
		break;
	case PROP_ERASER_RADIUS:
		g_value_set_double (value, model->radius[TOOL_ERASER]);
		break;
	case PROP_PEN_RADIUS:
		g_value_set_double (value, model->radius[TOOL_PENCIL]);
		break;
	case PROP_HIGHLIGHT_RADIUS:
		g_value_set_double (value, model->radius[TOOL_HIGHLIGHT]);
		break;
	case PROP_PEN_COLOR:
		g_value_set_boxed (value, &model->color[TOOL_PENCIL]);
		break;
	case PROP_TEXT_COLOR:
		g_value_set_boxed (value, &model->color[2]);
		break;
	case PROP_HIGHLIGHT_COLOR:
		g_value_set_boxed (value, &model->color[TOOL_HIGHLIGHT]);
		break;
	case PROP_TEXT_FONT:
		g_value_set_boxed (value, model->font_desc);
		break;
	case PROP_TEXT_FONT_SIZE:
		g_value_set_double (value, (double) pango_font_description_get_size (model->font_desc) / PANGO_SCALE);
		break;
	case PROP_ERASER_OBJECTS:
		g_value_set_boolean (value, model->eraser_objects);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_model_class_init (PpsAnnotationModelClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->get_property = pps_annotation_model_get_property;
	g_object_class->set_property = pps_annotation_model_set_property;

	model_properties[PROP_TOOL] = g_param_spec_int ("tool",
	                                                "Tool",
	                                                "Current tool",
	                                                TOOL_PENCIL, TOOL_MAX, TOOL_PENCIL,
	                                                G_PARAM_READWRITE |
	                                                    G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TOOL,
	                                 model_properties[PROP_TOOL]);
	model_properties[PROP_ACTIVE_TOOL_STR] = g_param_spec_string ("active-tool-str",
	                                                              "Tool",
	                                                              "Current tool",
	                                                              "pencil",
	                                                              G_PARAM_READWRITE |
	                                                                  G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_ACTIVE_TOOL_STR,
	                                 model_properties[PROP_ACTIVE_TOOL_STR]);

	model_properties[PROP_ERASER_RADIUS] = g_param_spec_double ("eraser-radius",
	                                                            "Eraser Radius",
	                                                            "Current eraser radius",
	                                                            1.0, 100., 5.0,
	                                                            G_PARAM_READWRITE |
	                                                                G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_ERASER_RADIUS,
	                                 model_properties[PROP_ERASER_RADIUS]);

	model_properties[PROP_ERASER_OBJECTS] = g_param_spec_boolean ("eraser-objects",
	                                                              "Eraser Objects",
	                                                              "Whether the eraser should erase objects or ink",
	                                                              TRUE,
	                                                              G_PARAM_READWRITE |
	                                                                  G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_ERASER_OBJECTS,
	                                 model_properties[PROP_ERASER_OBJECTS]);

	model_properties[PROP_HIGHLIGHT_RADIUS] = g_param_spec_double ("highlight-radius",
	                                                               "Highlight Radius",
	                                                               "Current highlight radius",
	                                                               1.0, 100., 5.0,
	                                                               G_PARAM_READWRITE |
	                                                                   G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_HIGHLIGHT_RADIUS,
	                                 model_properties[PROP_HIGHLIGHT_RADIUS]);

	model_properties[PROP_PEN_RADIUS] = g_param_spec_double ("pen-radius",
	                                                         "Pen Radius",
	                                                         "Current pen radius",
	                                                         1.0, 100., 1.0,
	                                                         G_PARAM_READWRITE |
	                                                             G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_PEN_RADIUS,
	                                 model_properties[PROP_PEN_RADIUS]);

	model_properties[PROP_PEN_COLOR] = g_param_spec_boxed ("pen-color",
	                                                       "Pen Color",
	                                                       "Current pen color",
	                                                       GDK_TYPE_RGBA,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_PEN_COLOR,
	                                 model_properties[PROP_PEN_COLOR]);

	model_properties[PROP_HIGHLIGHT_COLOR] = g_param_spec_boxed ("highlight-color",
	                                                             "Highlighter Color",
	                                                             "Current highlighter color",
	                                                             GDK_TYPE_RGBA,
	                                                             G_PARAM_READWRITE |
	                                                                 G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_HIGHLIGHT_COLOR,
	                                 model_properties[PROP_HIGHLIGHT_COLOR]);

	model_properties[PROP_TEXT_COLOR] = g_param_spec_boxed ("text-color",
	                                                        "Text Color",
	                                                        "Current text color",
	                                                        GDK_TYPE_RGBA,
	                                                        G_PARAM_READWRITE |
	                                                            G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_COLOR,
	                                 model_properties[PROP_TEXT_COLOR]);

	model_properties[PROP_TEXT_FONT] = g_param_spec_boxed ("text-font",
	                                                       "Font for new free text annots",
	                                                       "Current font",
	                                                       PANGO_TYPE_FONT_DESCRIPTION,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_FONT,
	                                 model_properties[PROP_TEXT_FONT]);
	model_properties[PROP_TEXT_FONT_SIZE] = g_param_spec_double ("text-font-size",
	                                                             "Font size for new free text annots",
	                                                             "Current font size",
	                                                             1., 50., 8.,
	                                                             G_PARAM_READWRITE |
	                                                                 G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_FONT_SIZE,
	                                 model_properties[PROP_TEXT_FONT_SIZE]);
}

static void
pps_annotation_model_init (PpsAnnotationModel *model)
{
	model->radius[TOOL_ERASER] = 5.0;
	model->radius[TOOL_HIGHLIGHT] = 5.0;
	model->radius[TOOL_PENCIL] = 1.0;
	model->font_desc = pango_font_description_from_string ("Sans Regular 12");
	model->eraser_objects = TRUE;
	gdk_rgba_parse (&model->color[TOOL_PENCIL], "#3584e4");
	gdk_rgba_parse (&model->color[TOOL_HIGHLIGHT], "#f5c211");
	gdk_rgba_parse (&model->color[2], "#3584e4");
}

PpsAnnotationModel *
pps_annotation_model_new (void)
{
	return g_object_new (PPS_TYPE_ANNOTATION_MODEL, NULL);
}
