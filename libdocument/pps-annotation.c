// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
 */

#include "config.h"

#include "pps-annotation.h"
#include "pps-document-misc.h"
#include "pps-document-type-builtins.h"

/* PpsAnnotation*/
typedef struct
{
	PpsAnnotationType type;
	PpsPage *page;

	gchar *contents;
	gchar *name;
	gchar *modified;
	GdkRGBA rgba;
	gboolean hidden;
	PpsRectangle area;
	gdouble border_width;

	GValue last_property_set;
} PpsAnnotationPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PpsAnnotation, pps_annotation, G_TYPE_OBJECT);
#define GET_ANNOT_PRIVATE(o) pps_annotation_get_instance_private (o)

/* PpsAnnotationMarkup*/
typedef struct
{
	gchar *label;
	gdouble opacity;
	gboolean can_have_popup;
	gboolean has_popup;
	gboolean popup_is_open;
	PpsRectangle rectangle;
} PpsAnnotationMarkupPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PpsAnnotationMarkup, pps_annotation_markup, PPS_TYPE_ANNOTATION);
#define GET_ANNOT_MARKUP_PRIVATE(o) pps_annotation_markup_get_instance_private (o)

/* PpsAnnotationText*/
typedef struct
{
	gboolean is_open : 1;
	PpsAnnotationTextIcon icon;
} PpsAnnotationTextPrivate;

struct _PpsAnnotationText {
	PpsAnnotation parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationText,
                            pps_annotation_text,
                            PPS_TYPE_ANNOTATION_MARKUP);
#define GET_ANNOT_TEXT_PRIVATE(o) pps_annotation_text_get_instance_private (o)

/* PpsAnnotationStamp */
typedef struct {
	gdouble width;
	cairo_surface_t *surface;
} PpsAnnotationStampPrivate;

struct _PpsAnnotationStamp {
	PpsAnnotation parent;
};

G_DEFINE_TYPE_WITH_CODE (PpsAnnotationStamp,
                         pps_annotation_stamp,
                         PPS_TYPE_ANNOTATION_MARKUP,
                         G_ADD_PRIVATE (PpsAnnotationStamp));
#define GET_ANNOT_STAMP_PRIVATE(o) pps_annotation_stamp_get_instance_private (o)

/* PpsAnnotationFreeText*/
typedef struct {
	PangoFontDescription *font_desc;
	GdkRGBA font_rgba;
} PpsAnnotationFreeTextPrivate;

struct _PpsAnnotationFreeText {
	PpsAnnotation parent;
};

G_DEFINE_TYPE_WITH_CODE (PpsAnnotationFreeText,
                         pps_annotation_free_text,
                         PPS_TYPE_ANNOTATION_MARKUP,
                         G_ADD_PRIVATE (PpsAnnotationFreeText));
#define GET_ANNOT_FREE_TEXT_PRIVATE(o) pps_annotation_free_text_get_instance_private (o)

/* PpsAnnotationAttachment */
typedef struct
{
	PpsAttachment *attachment;
} PpsAnnotationAttachmentPrivate;

struct _PpsAnnotationAttachment {
	PpsAnnotation parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationAttachment,
                            pps_annotation_attachment,
                            PPS_TYPE_ANNOTATION_MARKUP);
#define GET_ANNOT_ATTACH_PRIVATE(o) pps_annotation_attachment_get_instance_private (o)

/* PpsAnnotationTextMarkup */
typedef struct
{
	PpsAnnotationTextMarkupType type;
} PpsAnnotationTextMarkupPrivate;

struct _PpsAnnotationTextMarkup {
	PpsAnnotation parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationTextMarkup,
                            pps_annotation_text_markup,
                            PPS_TYPE_ANNOTATION_MARKUP);
#define GET_ANNOT_TEXT_MARKUP_PRIVATE(o) pps_annotation_text_markup_get_instance_private (o)

/* PpsAnnotation */
enum {
	PROP_PAGE = 1,
	PROP_CONTENTS,
	PROP_NAME,
	PROP_MODIFIED,
	PROP_RGBA,
	PROP_AREA,
	PROP_HIDDEN,
	PROP_BORDER_WIDTH,
	PROP_LAST,
};
static GParamSpec *properties[PROP_LAST];

/* PpsAnnotationMarkup */
enum {
	PROP_MARKUP_LABEL = 1,
	PROP_MARKUP_OPACITY,
	PROP_MARKUP_CAN_HAVE_POPUP,
	PROP_MARKUP_HAS_POPUP,
	PROP_MARKUP_RECTANGLE,
	PROP_MARKUP_POPUP_IS_OPEN,
	PROP_MARKUP_LAST
};
static GParamSpec *markup_properties[PROP_MARKUP_LAST];

/* PpsAnnotationText */
enum {
	PROP_TEXT_ICON = 1,
	PROP_TEXT_IS_OPEN,
	PROP_TEXT_LAST,
};
static GParamSpec *text_properties[PROP_TEXT_LAST];

/* PpsAnnotationFreeText */
enum {
	PROP_FREE_TEXT_0 = 1,
	PROP_FREE_TEXT_FONT_DESC,
	PROP_FREE_TEXT_FONT_RGBA,
	PROP_FREE_TEXT_LAST,
};
static GParamSpec *free_text_properties[PROP_FREE_TEXT_LAST];

/* PpsAnnotationAttachment */
enum {
	PROP_ATTACHMENT_ATTACHMENT = 1,
	PROP_ATTACHMENT_LAST,
};
static GParamSpec *attachment_properties[PROP_ATTACHMENT_LAST];

/* PpsAnnotationTextMarkup */
enum {
	PROP_TEXT_MARKUP_TYPE = 1,
	PROP_TEXT_MARKUP_LAST,
};
static GParamSpec *text_markup_properties[PROP_TEXT_MARKUP_LAST];

/* PpsAnnotation */
static void
pps_annotation_finalize (GObject *object)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (object));

	g_clear_object (&priv->page);
	g_clear_pointer (&priv->contents, g_free);
	g_clear_pointer (&priv->name, g_free);
	g_clear_pointer (&priv->modified, g_free);

	G_OBJECT_CLASS (pps_annotation_parent_class)->finalize (object);
}

static void
pps_annotation_init (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	priv->type = PPS_ANNOTATION_TYPE_UNKNOWN;
	priv->contents = g_strdup ("");
	priv->area.x1 = -1;
	priv->area.y1 = -1;
	priv->area.x2 = -1;
	priv->area.y2 = -1;
	g_value_init (&priv->last_property_set, G_TYPE_INT);
}

static void
pps_annotation_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
	PpsAnnotation *annot = PPS_ANNOTATION (object);
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	switch (prop_id) {
	case PROP_PAGE:
		priv->page = g_value_dup_object (value);
		break;
	case PROP_CONTENTS:
		pps_annotation_set_contents (annot, g_value_get_string (value));
		break;
	case PROP_NAME:
		pps_annotation_set_name (annot, g_value_get_string (value));
		break;
	case PROP_MODIFIED:
		pps_annotation_set_modified (annot, g_value_get_string (value));
		break;
	case PROP_RGBA:
		pps_annotation_set_rgba (annot, g_value_get_boxed (value));
		break;
	case PROP_AREA:
		pps_annotation_set_area (annot, g_value_get_boxed (value));
		break;
	case PROP_HIDDEN:
		pps_annotation_set_hidden (annot, g_value_get_boolean (value));
		break;
	case PROP_BORDER_WIDTH:
		pps_annotation_set_border_width (annot, g_value_get_double (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

void
pps_annotation_get_value_last_property (PpsAnnotation *annot, GValue *value)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	GType src_type = G_VALUE_TYPE (&priv->last_property_set);
	GType dst_type = G_VALUE_TYPE (value);

	if (!g_value_type_compatible (src_type, dst_type)) {
		g_value_unset (value);
		g_value_init (value, src_type);
	}

	g_value_copy (&priv->last_property_set, value);
}

static void
pps_annotation_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
	PpsAnnotation *annot = PPS_ANNOTATION (object);
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	switch (prop_id) {
	case PROP_CONTENTS:
		g_value_set_string (value, pps_annotation_get_contents (annot));
		break;
	case PROP_NAME:
		g_value_set_string (value, pps_annotation_get_name (annot));
		break;
	case PROP_MODIFIED:
		g_value_set_string (value, pps_annotation_get_modified (annot));
		break;
	case PROP_RGBA:
		g_value_set_boxed (value, &priv->rgba);
		break;
	case PROP_AREA:
		g_value_set_boxed (value, &priv->area);
		break;
	case PROP_HIDDEN:
		g_value_set_boolean (value, priv->hidden);
		break;
	case PROP_BORDER_WIDTH:
		g_value_set_double (value, priv->border_width);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_dispose (GObject *object)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (object));

	g_value_reset (&priv->last_property_set);

	G_OBJECT_CLASS (pps_annotation_parent_class)->dispose (object);
}

static void
pps_annotation_class_init (PpsAnnotationClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->finalize = pps_annotation_finalize;
	g_object_class->set_property = pps_annotation_set_property;
	g_object_class->get_property = pps_annotation_get_property;
	g_object_class->dispose = pps_annotation_dispose;

	properties[PROP_PAGE] =
	    g_param_spec_object ("page",
	                         "Page",
	                         "The page wehere the annotation is",
	                         PPS_TYPE_PAGE,
	                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
	                             G_PARAM_STATIC_STRINGS);
	properties[PROP_CONTENTS] =
	    g_param_spec_string ("contents",
	                         "Contents",
	                         "The annotation contents",
	                         NULL,
	                         G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);
	properties[PROP_NAME] =
	    g_param_spec_string ("name",
	                         "Name",
	                         "The annotation unique name",
	                         NULL,
	                         G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);
	properties[PROP_MODIFIED] =
	    g_param_spec_string ("modified",
	                         "Modified",
	                         "Last modified date as string",
	                         NULL,
	                         G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);
	properties[PROP_RGBA] =
	    g_param_spec_boxed ("rgba", NULL, NULL,
	                        GDK_TYPE_RGBA,
	                        G_PARAM_READWRITE |
	                            G_PARAM_STATIC_STRINGS);
	properties[PROP_AREA] =
	    g_param_spec_boxed ("area",
	                        "Area",
	                        "The area of the page where the annotation is placed",
	                        PPS_TYPE_RECTANGLE,
	                        G_PARAM_READWRITE |
	                            G_PARAM_STATIC_STRINGS);
	properties[PROP_HIDDEN] =
	    g_param_spec_boolean ("hidden",
	                          "Hidden Flag",
	                          "Whether the annotation is hidden or not",
	                          FALSE,
	                          G_PARAM_READWRITE |
	                              G_PARAM_STATIC_STRINGS);
	properties[PROP_BORDER_WIDTH] =
	    g_param_spec_double ("border-width",
	                         "Border Width",
	                         "The annotation border width",
	                         0., G_MAXDOUBLE, 0.,
	                         G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);

	g_object_class_install_property (g_object_class,
	                                 PROP_PAGE,
	                                 properties[PROP_PAGE]);
	g_object_class_install_property (g_object_class,
	                                 PROP_CONTENTS,
	                                 properties[PROP_CONTENTS]);
	g_object_class_install_property (g_object_class,
	                                 PROP_NAME,
	                                 properties[PROP_NAME]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MODIFIED,
	                                 properties[PROP_MODIFIED]);

	/**
	 * PpsAnnotation:rgba:
	 *
	 * The colour of the annotation as a #GdkRGBA.
	 *
	 */
	g_object_class_install_property (g_object_class,
	                                 PROP_RGBA,
	                                 properties[PROP_RGBA]);

	/**
	 * PpsAnnotation:area:
	 *
	 * The area of the page where the annotation is placed.
	 *
	 * Since 3.18
	 */
	g_object_class_install_property (g_object_class,
	                                 PROP_AREA,
	                                 properties[PROP_AREA]);

	/**
	 * PpsAnnotation:hidden:
	 *
	 * A flag to hide an annotation from the view.
	 *
	 * Since: 48.0
	 */
	g_object_class_install_property (g_object_class,
	                                 PROP_HIDDEN,
	                                 properties[PROP_HIDDEN]);

	/**
	 * PpsAnnotation:border-width:
	 *
	 * The border width of the annotation. This is only partially implemented, as there is no way
	 * to set the color. Thus, it may only be used for padding at this moment.
	 *
	 * Since: 48.0
	 */
	g_object_class_install_property (g_object_class,
	                                 PROP_BORDER_WIDTH,
	                                 properties[PROP_BORDER_WIDTH]);
}

PpsAnnotationType
pps_annotation_get_annotation_type (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), 0);

	return priv->type;
}

/**
 * pps_annotation_get_page:
 * @annot: an #PpsAnnotation
 *
 * Get the page where @annot appears.
 *
 * Returns: (transfer none): the #PpsPage where @annot appears
 */
PpsPage *
pps_annotation_get_page (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), NULL);

	return priv->page;
}

/**
 * pps_annotation_get_page_index:
 * @annot: an #PpsAnnotation
 *
 * Get the index of the page where @annot appears. Note that the index
 * is 0 based.
 *
 * Returns: the page index.
 */
guint
pps_annotation_get_page_index (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), 0);

	return priv->page->index;
}

/**
 * pps_annotation_equal:
 * @annot: an #PpsAnnotation
 * @other: another #PpsAnnotation
 *
 * Compare @annot and @other.
 *
 * Returns: %TRUE if @annot is equal to @other, %FALSE otherwise
 */
gboolean
pps_annotation_equal (PpsAnnotation *annot,
                      PpsAnnotation *other)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);
	g_return_val_if_fail (PPS_IS_ANNOTATION (other), FALSE);

	return (annot == other ||
	        g_strcmp0 (priv->name, pps_annotation_get_name (other)) == 0);
}

#define SAVE_PROPERTY(self, prop, type, value_set, val)                          \
	PpsAnnotationPrivate *priv_ = GET_ANNOT_PRIVATE (PPS_ANNOTATION (self)); \
	g_value_unset (&priv_->last_property_set);                               \
	g_value_init (&priv_->last_property_set, type);                          \
	value_set (&priv_->last_property_set, val);

/**
 * pps_annotation_get_contents:
 * @annot: an #PpsAnnotation
 *
 * Get the contents of @annot. The contents of
 * @annot is the text that is displayed in the annotation, or an
 * alternate description of the annotation's content for non-text annotations
 *
 * Returns: a string with the contents of the annotation or
 * %NULL if @annot has no contents.
 */
const gchar *
pps_annotation_get_contents (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), NULL);

	return priv->contents;
}

/**
 * pps_annotation_set_contents:
 * @annot: an #PpsAnnotation
 *
 * Set the contents of @annot. You can monitor
 * changes in the annotation's  contents by connecting to
 * notify::contents signal of @annot.
 *
 * Returns: %TRUE if the contents have been changed, %FALSE otherwise.
 */
gboolean
pps_annotation_set_contents (PpsAnnotation *annot,
                             const gchar *contents)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);

	if (g_strcmp0 (priv->contents, contents) == 0)
		return FALSE;

	SAVE_PROPERTY (annot, "contents", G_TYPE_STRING, g_value_set_string, priv->contents)

	if (priv->contents)
		g_free (priv->contents);
	priv->contents = contents ? g_strdup (contents) : NULL;

	g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_CONTENTS]);

	return TRUE;
}

/**
 * pps_annotation_get_name:
 * @annot: an #PpsAnnotation
 *
 * Get the name of @annot. The name of the annotation is a string
 * that uniquely indenftifies @annot amongs all the annotations
 * in the same page.
 *
 * Returns: the string with the annotation's name.
 */
const gchar *
pps_annotation_get_name (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), NULL);

	return priv->name;
}

/**
 * pps_annotation_set_name:
 * @annot: an #PpsAnnotation
 *
 * Set the name of @annot.
 * You can monitor changes of the annotation name by connecting
 * to the notify::name signal on @annot.
 *
 * Returns: %TRUE when the name has been changed, %FALSE otherwise.
 */
gboolean
pps_annotation_set_name (PpsAnnotation *annot,
                         const gchar *name)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);

	if (g_strcmp0 (priv->name, name) == 0)
		return FALSE;

	if (priv->name)
		g_free (priv->name);
	priv->name = name ? g_strdup (name) : NULL;

	g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_NAME]);

	return TRUE;
}

/**
 * pps_annotation_get_modified:
 * @annot: an #PpsAnnotation
 *
 * Get the last modification date of @annot.
 *
 * Returns: A string containing the last modification date.
 */
const gchar *
pps_annotation_get_modified (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), NULL);

	return priv->modified;
}

/**
 * pps_annotation_set_modified:
 * @annot: an #PpsAnnotation
 * @modified: string with the last modification date.
 *
 * Set the last modification date of @annot to @modified. To
 * set the last modification date using a #time_t, use
 * pps_annotation_set_modified_from_time_t() instead. You can monitor
 * changes to the last modification date by connecting to the
 * notify::modified signal on @annot.
 *
 * Returns: %TRUE if the last modification date has been updated, %FALSE otherwise.
 */
gboolean
pps_annotation_set_modified (PpsAnnotation *annot,
                             const gchar *modified)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);

	if (g_strcmp0 (priv->modified, modified) == 0)
		return FALSE;

	if (priv->modified)
		g_free (priv->modified);
	priv->modified = modified ? g_strdup (modified) : NULL;

	g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_MODIFIED]);

	return TRUE;
}

/**
 * pps_annotation_set_modified_from_time_t:
 * @annot: an #PpsAnnotation
 * @utime: a #time_t
 *
 * Set the last modification date of @annot to @utime.  You can
 * monitor changes to the last modification date by connecting to the
 * notify::modified sinal on @annot.
 * For the time-format used, see pps_document_misc_format_datetime().
 *
 * Returns: %TRUE if the last modified date has been updated, %FALSE otherwise.
 */
gboolean
pps_annotation_set_modified_from_time_t (PpsAnnotation *annot,
                                         time_t utime)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	gchar *modified;
	g_autoptr (GDateTime) dt = g_date_time_new_from_unix_local ((gint64) utime);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);

	modified = pps_document_misc_format_datetime (dt);

	if (g_strcmp0 (priv->modified, modified) == 0) {
		g_free (modified);
		return FALSE;
	}

	if (priv->modified)
		g_free (priv->modified);

	priv->modified = modified;
	g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_MODIFIED]);

	return TRUE;
}

/**
 * pps_annotation_get_rgba:
 * @annot: an #PpsAnnotation
 * @rgba: (out): a #GdkRGBA to be filled with the annotation color
 *
 * Gets the color of @annot.
 *
 */
void
pps_annotation_get_rgba (PpsAnnotation *annot,
                         GdkRGBA *rgba)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_if_fail (PPS_IS_ANNOTATION (annot));
	g_return_if_fail (rgba != NULL);

	*rgba = priv->rgba;
}

/**
 * pps_annotation_set_rgba:
 * @annot: an #Ppsannotation
 * @rgba: a #GdkRGBA
 *
 * Set the color of the annotation to @rgba.
 *
 * Returns: %TRUE if the color has been changed, %FALSE otherwise
 */
gboolean
pps_annotation_set_rgba (PpsAnnotation *annot,
                         const GdkRGBA *rgba)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);
	g_return_val_if_fail (rgba != NULL, FALSE);

	if (gdk_rgba_equal (rgba, &priv->rgba))
		return FALSE;

	SAVE_PROPERTY (annot, "rgba", GDK_TYPE_RGBA, g_value_set_boxed, &priv->rgba)

	priv->rgba = *rgba;
	g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_RGBA]);

	return TRUE;
}

/**
 * pps_annotation_get_area:
 * @annot: an #PpsAnnotation
 * @area: (out): a #PpsRectangle to be filled with the annotation area
 *
 * Gets the area of @annot.
 */
void
pps_annotation_get_area (PpsAnnotation *annot,
                         PpsRectangle *area)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);

	g_return_if_fail (PPS_IS_ANNOTATION (annot));
	g_return_if_fail (area != NULL);

	*area = priv->area;
}

/**
 * pps_annotation_set_area:
 * @annot: an #Ppsannotation
 * @area: a #PpsRectangle
 *
 * Set the area of the annotation to @area.
 *
 * Returns: %TRUE if the area has been changed, %FALSE otherwise
 */
gboolean
pps_annotation_set_area (PpsAnnotation *annot,
                         const PpsRectangle *area)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	gboolean was_initial;

	g_return_val_if_fail (PPS_IS_ANNOTATION (annot), FALSE);
	g_return_val_if_fail (area != NULL, FALSE);

	if (pps_rect_cmp ((PpsRectangle *) area, &priv->area) == 0)
		return FALSE;

	SAVE_PROPERTY (annot, "area", PPS_TYPE_RECTANGLE, g_value_set_boxed, &priv->area)

	was_initial = priv->area.x1 == -1 && priv->area.x2 == -1 && priv->area.y1 == -1 && priv->area.y2 == -1;
	priv->area = *area;
	if (!was_initial)
		g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_AREA]);

	return TRUE;
}

/**
 * pps_annotation_set_hidden:
 * @annot: a #PpsAnnotation
 * @hidden: a boolean
 *
 * Set whether the annotation is hidden or not.
 *
 * Returns: %TRUE if the visibility of the annotation has been changed, %FALSE otherwise
 *
 * Since: 48.0
 */
gboolean
pps_annotation_set_hidden (PpsAnnotation *annot, const gboolean hidden)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	if (priv->hidden != hidden) {
		priv->hidden = hidden;
		g_object_notify_by_pspec (G_OBJECT (annot), properties[PROP_HIDDEN]);
		return TRUE;
	}
	return FALSE;
}

/**
 * pps_annotation_get_hidden:
 * @annot: a #PpsAnnotation
 *
 * Gets the hidden flag of @annot, i.e. whether it is hidden or not.
 *
 * Since: 48.0
 */
gboolean
pps_annotation_get_hidden (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	return priv->hidden;
}

/**
 * pps_annotation_set_border_width:
 * @annot: a #PpsAnnotation
 * @width: double
 *
 * Set the area of the annotation to @area.
 *
 * Returns: %TRUE if the border width has been changed, %FALSE otherwise
 *
 * Since: 48.0
 */
gboolean
pps_annotation_set_border_width (PpsAnnotation *annot, const gdouble width)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	if (priv->border_width != width) {
		priv->border_width = width;
		g_object_notify_by_pspec (G_OBJECT (annot),
		                          properties[PROP_BORDER_WIDTH]);
		return TRUE;
	}
	return FALSE;
}

/**
 * pps_annotation_get_border_width:
 * @annot: a #PpsAnnotation
 *
 * Gets the border width of @annot.
 *
 * Since: 48.0
 */
gdouble
pps_annotation_get_border_width (PpsAnnotation *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (annot);
	return priv->border_width;
}

/* PpsAnnotationMarkup */
static void
pps_annotation_markup_dispose (GObject *object)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (object));

	g_free (priv->label);

	G_OBJECT_CLASS (pps_annotation_markup_parent_class)->dispose (object);
}

static void
pps_annotation_markup_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
	PpsAnnotationMarkup *self = PPS_ANNOTATION_MARKUP (object);

	switch (prop_id) {
	case PROP_MARKUP_LABEL:
		pps_annotation_markup_set_label (self, g_value_get_string (value));
		break;
	case PROP_MARKUP_OPACITY:
		pps_annotation_markup_set_opacity (self, g_value_get_double (value));
		break;
	case PROP_MARKUP_HAS_POPUP:
		pps_annotation_markup_set_has_popup (self, g_value_get_boolean (value));
		break;
	case PROP_MARKUP_RECTANGLE:
		pps_annotation_markup_set_rectangle (self, g_value_get_boxed (value));
		break;
	case PROP_MARKUP_POPUP_IS_OPEN:
		pps_annotation_markup_set_popup_is_open (self, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_markup_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
	PpsAnnotationMarkup *self = PPS_ANNOTATION_MARKUP (object);
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	switch (prop_id) {
	case PROP_MARKUP_LABEL:
		g_value_set_string (value, priv->label);
		break;
	case PROP_MARKUP_OPACITY:
		g_value_set_double (value, priv->opacity);
		break;
	case PROP_MARKUP_CAN_HAVE_POPUP:
		g_value_set_boolean (value, priv->can_have_popup);
		break;
	case PROP_MARKUP_HAS_POPUP:
		g_value_set_boolean (value, priv->has_popup);
		break;
	case PROP_MARKUP_RECTANGLE:
		g_value_set_boxed (value, &priv->rectangle);
		break;
	case PROP_MARKUP_POPUP_IS_OPEN:
		g_value_set_boolean (value, priv->popup_is_open);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_markup_init (PpsAnnotationMarkup *self)
{
}

static void
pps_annotation_markup_class_init (PpsAnnotationMarkupClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->dispose = pps_annotation_markup_dispose;
	g_object_class->set_property = pps_annotation_markup_set_property;
	g_object_class->get_property = pps_annotation_markup_get_property;

	markup_properties[PROP_MARKUP_LABEL] =
	    g_param_spec_string ("label",
	                         "Label",
	                         "Label of the markup annotation",
	                         NULL,
	                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	markup_properties[PROP_MARKUP_OPACITY] =
	    g_param_spec_double ("opacity",
	                         "Opacity",
	                         "Opacity of the markup annotation",
	                         0,
	                         G_MAXDOUBLE,
	                         1.,
	                         G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);
	markup_properties[PROP_MARKUP_CAN_HAVE_POPUP] =
	    g_param_spec_boolean ("can-have-popup",
	                          "Can have popup",
	                          "Whether it is allowed to have a popup "
	                          "window for this type of markup annotation",
	                          FALSE,
	                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	markup_properties[PROP_MARKUP_HAS_POPUP] =
	    g_param_spec_boolean ("has-popup",
	                          "Has popup",
	                          "Whether the markup annotation has "
	                          "a popup window associated",
	                          TRUE,
	                          G_PARAM_READWRITE |
	                              G_PARAM_STATIC_STRINGS);
	markup_properties[PROP_MARKUP_RECTANGLE] =
	    g_param_spec_boxed ("rectangle",
	                        "Rectangle",
	                        "The Rectangle of the popup associated "
	                        "to the markup annotation",
	                        PPS_TYPE_RECTANGLE,
	                        G_PARAM_READWRITE |
	                            G_PARAM_STATIC_STRINGS);
	markup_properties[PROP_MARKUP_POPUP_IS_OPEN] =
	    g_param_spec_boolean ("popup-is-open",
	                          "PopupIsOpen",
	                          "Whether the popup associated to "
	                          "the markup annotation is open",
	                          FALSE,
	                          G_PARAM_READWRITE |
	                              G_PARAM_STATIC_STRINGS);

	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_LABEL,
	                                 markup_properties[PROP_MARKUP_LABEL]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_OPACITY,
	                                 markup_properties[PROP_MARKUP_OPACITY]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_CAN_HAVE_POPUP,
	                                 markup_properties[PROP_MARKUP_CAN_HAVE_POPUP]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_HAS_POPUP,
	                                 markup_properties[PROP_MARKUP_HAS_POPUP]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_RECTANGLE,
	                                 markup_properties[PROP_MARKUP_RECTANGLE]);
	g_object_class_install_property (g_object_class,
	                                 PROP_MARKUP_POPUP_IS_OPEN,
	                                 markup_properties[PROP_MARKUP_POPUP_IS_OPEN]);
}

const gchar *
pps_annotation_markup_get_label (PpsAnnotationMarkup *self)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), NULL);

	return priv->label;
}

gboolean
pps_annotation_markup_set_label (PpsAnnotationMarkup *self,
                                 const gchar *label)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);
	g_return_val_if_fail (label != NULL, FALSE);

	if (g_strcmp0 (priv->label, label) == 0)
		return FALSE;

	SAVE_PROPERTY (self, "label", G_TYPE_STRING, g_value_set_string, priv->label)

	g_free (priv->label);
	priv->label = g_strdup (label);

	g_object_notify_by_pspec (G_OBJECT (self),
	                          markup_properties[PROP_MARKUP_LABEL]);

	return TRUE;
}

gdouble
pps_annotation_markup_get_opacity (PpsAnnotationMarkup *self)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), 1.0);

	return priv->opacity;
}

gboolean
pps_annotation_markup_set_opacity (PpsAnnotationMarkup *self,
                                   gdouble opacity)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	if (priv->opacity == opacity)
		return FALSE;

	SAVE_PROPERTY (self, "opacity", G_TYPE_DOUBLE, g_value_set_double, priv->opacity)

	priv->opacity = opacity;

	g_object_notify_by_pspec (G_OBJECT (self),
	                          markup_properties[PROP_MARKUP_OPACITY]);

	return TRUE;
}

gboolean
pps_annotation_markup_can_have_popup (PpsAnnotationMarkup *self)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	return priv->can_have_popup;
}

gboolean
pps_annotation_markup_has_popup (PpsAnnotationMarkup *self)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	return priv->has_popup;
}

gboolean
pps_annotation_markup_set_has_popup (PpsAnnotationMarkup *self,
                                     gboolean has_popup)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	if (priv->has_popup == has_popup)
		return FALSE;

	priv->has_popup = has_popup;

	g_object_notify_by_pspec (G_OBJECT (self),
	                          markup_properties[PROP_MARKUP_HAS_POPUP]);

	return TRUE;
}

void
pps_annotation_markup_get_rectangle (PpsAnnotationMarkup *self,
                                     PpsRectangle *pps_rect)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_if_fail (PPS_IS_ANNOTATION_MARKUP (self));
	g_return_if_fail (pps_rect != NULL);

	*pps_rect = priv->rectangle;
}

gboolean
pps_annotation_markup_set_rectangle (PpsAnnotationMarkup *self,
                                     const PpsRectangle *pps_rect)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);
	g_return_val_if_fail (pps_rect != NULL, FALSE);

	if (priv->rectangle.x1 == pps_rect->x1 &&
	    priv->rectangle.y1 == pps_rect->y1 &&
	    priv->rectangle.x2 == pps_rect->x2 &&
	    priv->rectangle.y2 == pps_rect->y2)
		return FALSE;

	priv->rectangle = *pps_rect;

	g_object_notify_by_pspec (G_OBJECT (self),
	                          markup_properties[PROP_MARKUP_RECTANGLE]);

	return TRUE;
}

gboolean
pps_annotation_markup_get_popup_is_open (PpsAnnotationMarkup *self)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	return priv->popup_is_open;
}

gboolean
pps_annotation_markup_set_popup_is_open (PpsAnnotationMarkup *self,
                                         gboolean is_open)
{
	PpsAnnotationMarkupPrivate *priv = GET_ANNOT_MARKUP_PRIVATE (self);

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (self), FALSE);

	if (priv->popup_is_open == is_open)
		return FALSE;

	SAVE_PROPERTY (self, "popup-is-open", G_TYPE_BOOLEAN, g_value_set_boolean, priv->popup_is_open)

	priv->popup_is_open = is_open;

	g_object_notify_by_pspec (G_OBJECT (self),
	                          markup_properties[PROP_MARKUP_POPUP_IS_OPEN]);

	return TRUE;
}

/* PpsAnnotationText */
static void
pps_annotation_text_init (PpsAnnotationText *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (annot));
	PpsAnnotationMarkupPrivate *markup_priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (annot));

	priv->type = PPS_ANNOTATION_TYPE_TEXT;
	markup_priv->can_have_popup = TRUE;
}

static void
pps_annotation_text_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	PpsAnnotationText *annot = PPS_ANNOTATION_TEXT (object);

	switch (prop_id) {
	case PROP_TEXT_ICON:
		pps_annotation_text_set_icon (annot, g_value_get_enum (value));
		break;
	case PROP_TEXT_IS_OPEN:
		pps_annotation_text_set_is_open (annot, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_text_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	PpsAnnotationText *annot = PPS_ANNOTATION_TEXT (object);
	PpsAnnotationTextPrivate *priv = GET_ANNOT_TEXT_PRIVATE (annot);

	switch (prop_id) {
	case PROP_TEXT_ICON:
		g_value_set_enum (value, priv->icon);
		break;
	case PROP_TEXT_IS_OPEN:
		g_value_set_boolean (value, priv->is_open);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_text_class_init (PpsAnnotationTextClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = pps_annotation_text_set_property;
	g_object_class->get_property = pps_annotation_text_get_property;

	text_properties[PROP_TEXT_ICON] =
	    g_param_spec_enum ("icon",
	                       "Icon",
	                       "The icon fo the text annotation",
	                       PPS_TYPE_ANNOTATION_TEXT_ICON,
	                       PPS_ANNOTATION_TEXT_ICON_NOTE,
	                       G_PARAM_READWRITE |
	                           G_PARAM_STATIC_STRINGS);
	text_properties[PROP_TEXT_IS_OPEN] =
	    g_param_spec_boolean ("is-open",
	                          "IsOpen",
	                          "Whether text annot is initially open",
	                          FALSE,
	                          G_PARAM_READWRITE |
	                              G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_ICON,
	                                 text_properties[PROP_TEXT_ICON]);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_IS_OPEN,
	                                 text_properties[PROP_TEXT_IS_OPEN]);
}

PpsAnnotation *
pps_annotation_text_new (PpsPage *page)
{
	return PPS_ANNOTATION (g_object_new (PPS_TYPE_ANNOTATION_TEXT,
	                                     "page", page,
	                                     NULL));
}

PpsAnnotationTextIcon
pps_annotation_text_get_icon (PpsAnnotationText *text)
{
	PpsAnnotationTextPrivate *priv = GET_ANNOT_TEXT_PRIVATE (text);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT (text), 0);

	return priv->icon;
}

gboolean
pps_annotation_text_set_icon (PpsAnnotationText *text,
                              PpsAnnotationTextIcon icon)
{
	PpsAnnotationTextPrivate *priv = GET_ANNOT_TEXT_PRIVATE (text);
	PpsAnnotation *self = PPS_ANNOTATION (text);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT (text), FALSE);

	if (priv->icon == icon)
		return FALSE;

	SAVE_PROPERTY (self, "icon", PPS_TYPE_ANNOTATION_TEXT_ICON, g_value_set_enum, priv->icon)

	priv->icon = icon;

	g_object_notify_by_pspec (G_OBJECT (text),
	                          text_properties[PROP_TEXT_ICON]);

	return TRUE;
}

gboolean
pps_annotation_text_get_is_open (PpsAnnotationText *text)
{
	PpsAnnotationTextPrivate *priv = GET_ANNOT_TEXT_PRIVATE (text);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT (text), FALSE);

	return priv->is_open;
}

gboolean
pps_annotation_text_set_is_open (PpsAnnotationText *text,
                                 gboolean is_open)
{
	PpsAnnotationTextPrivate *priv = GET_ANNOT_TEXT_PRIVATE (text);
	PpsAnnotation *self = PPS_ANNOTATION (text);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT (text), FALSE);

	if (priv->is_open == is_open)
		return FALSE;

	SAVE_PROPERTY (self, "is-open", G_TYPE_BOOLEAN, g_value_set_boolean, priv->is_open)
	priv->is_open = is_open;

	g_object_notify_by_pspec (G_OBJECT (text),
	                          text_properties[PROP_TEXT_IS_OPEN]);

	return TRUE;
}

/* PpsAnnotationStamp */
static void
pps_annotation_stamp_init (PpsAnnotationStamp *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (annot));
	PpsAnnotationMarkupPrivate *markup_priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (annot));

	priv->type = PPS_ANNOTATION_TYPE_STAMP;
	markup_priv->can_have_popup = TRUE;
}

static void
pps_annotation_stamp_dispose (GObject *object)
{
	PpsAnnotationStampPrivate *priv = GET_ANNOT_STAMP_PRIVATE (PPS_ANNOTATION_STAMP (object));

	g_clear_pointer (&priv->surface, cairo_surface_destroy);

	G_OBJECT_CLASS (pps_annotation_stamp_parent_class)->dispose (object);
}

static void
pps_annotation_stamp_class_init (PpsAnnotationStampClass *klass)
{
	G_OBJECT_CLASS (klass)->dispose = pps_annotation_stamp_dispose;
}

/**
 * pps_annotation_stamp_set_surface:
 * @stamp: an #PpsAnnotationStamp
 * @surface: a #cairo_surface_t
 *
 * Set the custom cairo surface of the stamp.
 *
 * Since: 48.0
 */
void
pps_annotation_stamp_set_surface (PpsAnnotationStamp *stamp, cairo_surface_t *surface)
{
	PpsAnnotationStampPrivate *priv = GET_ANNOT_STAMP_PRIVATE (stamp);
	priv->surface = cairo_surface_reference (surface);
}

/**
 * pps_annotation_stamp_get_surface:
 * @stamp: an #PpsAnnotationStamp
 *
 * Set the custom cairo surface of the stamp.
 *
 * Returns: (transfer none): the custom cairo surface of the stamp, if it exists.
 * Since: 48.0
 */
cairo_surface_t *
pps_annotation_stamp_get_surface (PpsAnnotationStamp *stamp)
{
	PpsAnnotationStampPrivate *priv = GET_ANNOT_STAMP_PRIVATE (stamp);
	return priv->surface;
}

/**
 * pps_annotation_stamp_new:
 * @page: a #PpsPage
 *
 * Creates a new stamp annotation. ATM only the custom image type via
 * #pps_annotation_stamp_set_surface is implemented, other stamps (draft, etc.)
 * are not.
 *
 * Since: 48.0
 */
PpsAnnotation *
pps_annotation_stamp_new (PpsPage *page)
{
	return PPS_ANNOTATION (g_object_new (PPS_TYPE_ANNOTATION_STAMP,
	                                     "page", page,
	                                     NULL));
}

/* PpsAnnotationFreeText */
static void
pps_annotation_free_text_init (PpsAnnotationFreeText *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (annot));
	PpsAnnotationMarkupPrivate *markup_priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (annot));

	priv->type = PPS_ANNOTATION_TYPE_FREE_TEXT;
	markup_priv->can_have_popup = FALSE;
}

static void
pps_annotation_free_text_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
	PpsAnnotationFreeText *annot = PPS_ANNOTATION_FREE_TEXT (object);

	switch (prop_id) {
	case PROP_FREE_TEXT_FONT_DESC:
		pps_annotation_free_text_set_font_description (annot, g_value_get_boxed (value));
		break;
	case PROP_FREE_TEXT_FONT_RGBA:
		pps_annotation_free_text_set_font_rgba (annot, g_value_get_boxed (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_free_text_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (object));

	switch (prop_id) {
	case PROP_FREE_TEXT_FONT_RGBA:
		g_value_set_boxed (value, &priv->font_rgba);
		break;
	case PROP_FREE_TEXT_FONT_DESC:
		g_value_set_boxed (value, &priv->font_desc);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_free_text_dispose (GObject *object)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (object));

	g_clear_pointer (&priv->font_desc, pango_font_description_free);

	G_OBJECT_CLASS (pps_annotation_free_text_parent_class)->dispose (object);
}

static void
pps_annotation_free_text_class_init (PpsAnnotationFreeTextClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
	g_object_class->set_property = pps_annotation_free_text_set_property;
	g_object_class->get_property = pps_annotation_free_text_get_property;
	g_object_class->dispose = pps_annotation_free_text_dispose;

	free_text_properties[PROP_FREE_TEXT_FONT_DESC] =
	    g_param_spec_boxed ("font-desc", NULL, NULL,
	                        PANGO_TYPE_FONT_DESCRIPTION,
	                        G_PARAM_READWRITE |
	                            G_PARAM_STATIC_STRINGS);
	free_text_properties[PROP_FREE_TEXT_FONT_RGBA] =
	    g_param_spec_boxed ("font-rgba", NULL, NULL,
	                        GDK_TYPE_RGBA,
	                        G_PARAM_READWRITE |
	                            G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_FREE_TEXT_FONT_DESC,
	                                 free_text_properties[PROP_FREE_TEXT_FONT_DESC]);
	g_object_class_install_property (g_object_class,
	                                 PROP_FREE_TEXT_FONT_RGBA,
	                                 free_text_properties[PROP_FREE_TEXT_FONT_RGBA]);
}

/**
 * pps_annotation_free_text_new:
 * @page: a #PpsPage
 *
 * Creates a new free text annotation in the page. Font shall be set afterwards.
 *
 * Returns: a new free text annotation
 *
 * Since: 48.0
 */
PpsAnnotation *
pps_annotation_free_text_new (PpsPage *page)
{
	return PPS_ANNOTATION (g_object_new (PPS_TYPE_ANNOTATION_FREE_TEXT, "page", page, NULL));
}

/**
 * pps_annotation_free_text_set_font_description:
 * @annot: an #PpsAnnotationFreeText
 * @font_desc: a #PangoFontDescription
 *
 * Set the font of the free text annotation to annotation to @font_desc.
 *
 * Returns: %TRUE if the font description has been changed, %FALSE otherwise
 *
 * Since: 48.0
 */
gboolean
pps_annotation_free_text_set_font_description (PpsAnnotationFreeText *annot,
                                               const PangoFontDescription *font_desc)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (annot));
	if (priv->font_desc && pango_font_description_equal (priv->font_desc, font_desc)) {
		return FALSE;
	}
	SAVE_PROPERTY (annot, "font-desc", PANGO_TYPE_FONT_DESCRIPTION, g_value_set_boxed, priv->font_desc)
	g_clear_pointer (&priv->font_desc, pango_font_description_free);
	priv->font_desc = pango_font_description_copy (font_desc);

	g_object_notify_by_pspec (G_OBJECT (annot),
	                          free_text_properties[PROP_FREE_TEXT_FONT_DESC]);
	return TRUE;
}

/**
 * pps_annotation_free_text_get_font_description:
 * @annot: an #PpsAnnotationFreeText
 *
 * Returns a copy of the font descption used by the annotation.
 *
 * Returns: (transfer full): the font description used to display the annotation.
 *
 * Since: 48.0
 */
PangoFontDescription *
pps_annotation_free_text_get_font_description (PpsAnnotationFreeText *annot)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (annot));

	return pango_font_description_copy (priv->font_desc);
}

/**
 * pps_annotation_free_text_set_font_rgba:
 * @annot: an #PpsAnnotationFreeText
 * @rgba: a #GdkRGBA
 *
 * Set the text color of the annotation to @rgba.
 *
 * Returns: %TRUE if the color has been changed, %FALSE otherwise
 *
 * Since: 48.0
 */
gboolean
pps_annotation_free_text_set_font_rgba (PpsAnnotationFreeText *annot, const GdkRGBA *rgba)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (annot));

	if (gdk_rgba_equal (&(priv->font_rgba), rgba)) {
		return FALSE;
	}

	SAVE_PROPERTY (annot, "font-rgba", GDK_TYPE_RGBA, g_value_set_boxed, &priv->font_rgba)

	priv->font_rgba = *rgba;

	g_object_notify_by_pspec (G_OBJECT (annot),
	                          free_text_properties[PROP_FREE_TEXT_FONT_RGBA]);

	return TRUE;
}

/**
 * pps_annotation_free_text_get_font_rgba:
 * @annot: an #PpsAnnotationFreeText
 *
 * Gets the text color of @annot.
 *
 * Returns: (transfer full): the font RGBA, must be freed by the caller
 *
 * Since: 48.0
 */
GdkRGBA *
pps_annotation_free_text_get_font_rgba (PpsAnnotationFreeText *annot)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (annot));

	return gdk_rgba_copy (&priv->font_rgba);
}

/**
 * pps_annotation_free_text_auto_resize:
 * @annot: an #PpsAnnotationFreeText
 * @ctx: a valid #PangoContext
 *
 * Resize the annotation so as all the text fits in its rect, according to Pango metrics.
 * This should typically be called every time the content is changed unless the free text
 * annotation is supposed to be fixed width for instance.
 *
 * Since: 48.0
 */
void
pps_annotation_free_text_auto_resize (PpsAnnotationFreeText *annot, PangoContext *ctx)
{
	PpsAnnotationFreeTextPrivate *priv = GET_ANNOT_FREE_TEXT_PRIVATE (PPS_ANNOTATION_FREE_TEXT (annot));

	gint width, height, border_width;
	PpsRectangle rect;

	/* The text is going to be measured with a pango layout with the same settings as the annot. */
	g_autoptr (PangoLayout) layout = pango_layout_new (ctx);
	gint font_size = pango_font_description_get_size (priv->font_desc);
	g_autoptr (PangoAttrList) list = pango_attr_list_new ();
	// font_size is in px, convert it to pt with /0.75
	PangoAttribute *attr = pango_attr_line_height_new_absolute (font_size / 0.75);

	const gchar *contents = pps_annotation_get_contents (PPS_ANNOTATION (annot));
	pango_layout_set_text (layout, contents, -1);

	pango_attr_list_change (list, attr);
	pango_layout_set_attributes (layout, list);
	pango_layout_set_font_description (layout, priv->font_desc);

	pango_layout_get_size (layout, &width, &height);

	/* Updating the annot size based on the current position. */
	pps_annotation_get_area (PPS_ANNOTATION (annot), &rect);
	/* poppler adds a padding of the same extent as the border width, so the actual width/height must
	be increased by 4*border_width
	+ 2 is necessary because poppler does not layout characters exactly as pango unfortunately */
	border_width = pps_annotation_get_border_width (PPS_ANNOTATION (annot));
	rect.x2 = rect.x1 + width / PANGO_SCALE * 0.75 + 4 * border_width + 2;
	/* We add 1/4 of a line below so descending characters are shown.*/
	rect.y2 = rect.y1 + height / PANGO_SCALE * 0.75 + 4 * border_width + 0.25 * font_size / PANGO_SCALE + 2;
	pps_annotation_set_area (PPS_ANNOTATION (annot), &rect);
}

/* PpsAnnotationAttachment */
static void
pps_annotation_attachment_finalize (GObject *object)
{
	PpsAnnotationAttachment *annot = PPS_ANNOTATION_ATTACHMENT (object);
	PpsAnnotationAttachmentPrivate *priv = GET_ANNOT_ATTACH_PRIVATE (annot);

	g_clear_object (&priv->attachment);

	G_OBJECT_CLASS (pps_annotation_attachment_parent_class)->finalize (object);
}

static void
pps_annotation_attachment_init (PpsAnnotationAttachment *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (annot));
	PpsAnnotationMarkupPrivate *markup_priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (annot));

	priv->type = PPS_ANNOTATION_TYPE_ATTACHMENT;
	markup_priv->can_have_popup = TRUE;
}

static void
pps_annotation_attachment_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
	PpsAnnotationAttachment *annot = PPS_ANNOTATION_ATTACHMENT (object);

	switch (prop_id) {
	case PROP_ATTACHMENT_ATTACHMENT:
		pps_annotation_attachment_set_attachment (annot, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_attachment_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
	PpsAnnotationAttachment *annot = PPS_ANNOTATION_ATTACHMENT (object);
	PpsAnnotationAttachmentPrivate *priv = GET_ANNOT_ATTACH_PRIVATE (annot);

	switch (prop_id) {
	case PROP_ATTACHMENT_ATTACHMENT:
		g_value_set_object (value, priv->attachment);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_attachment_class_init (PpsAnnotationAttachmentClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = pps_annotation_attachment_set_property;
	g_object_class->get_property = pps_annotation_attachment_get_property;
	g_object_class->finalize = pps_annotation_attachment_finalize;

	attachment_properties[PROP_ATTACHMENT_ATTACHMENT] =
	    g_param_spec_object ("attachment",
	                         "Attachment",
	                         "The attachment of the annotation",
	                         PPS_TYPE_ATTACHMENT,
	                         G_PARAM_CONSTRUCT |
	                             G_PARAM_READWRITE |
	                             G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_ATTACHMENT_ATTACHMENT,
	                                 attachment_properties[PROP_ATTACHMENT_ATTACHMENT]);
}

PpsAnnotation *
pps_annotation_attachment_new (PpsPage *page,
                               PpsAttachment *attachment)
{
	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), NULL);

	return PPS_ANNOTATION (g_object_new (PPS_TYPE_ANNOTATION_ATTACHMENT,
	                                     "page", page,
	                                     "attachment", attachment,
	                                     NULL));
}

/**
 * pps_annotation_attachment_get_attachment:
 * @annot: an #PpsAnnotationAttachment
 *
 * Returns: (transfer none): an #PpsAttachment
 */
PpsAttachment *
pps_annotation_attachment_get_attachment (PpsAnnotationAttachment *annot)
{
	PpsAnnotationAttachmentPrivate *priv = GET_ANNOT_ATTACH_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION_ATTACHMENT (annot), NULL);

	return priv->attachment;
}

gboolean
pps_annotation_attachment_set_attachment (PpsAnnotationAttachment *annot,
                                          PpsAttachment *attachment)
{
	PpsAnnotationAttachmentPrivate *priv = GET_ANNOT_ATTACH_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION_ATTACHMENT (annot), FALSE);

	if (g_set_object (&priv->attachment, attachment)) {
		g_object_notify_by_pspec (G_OBJECT (annot),
		                          attachment_properties[PROP_ATTACHMENT_ATTACHMENT]);
		return TRUE;
	}

	return FALSE;
}

/* PpsAnnotationTextMarkup */
static void
pps_annotation_text_markup_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
	PpsAnnotationTextMarkup *annot = PPS_ANNOTATION_TEXT_MARKUP (object);
	PpsAnnotationTextMarkupPrivate *priv = GET_ANNOT_TEXT_MARKUP_PRIVATE (annot);

	switch (prop_id) {
	case PROP_TEXT_MARKUP_TYPE:
		g_value_set_enum (value, priv->type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_text_markup_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
	PpsAnnotationTextMarkup *annot = PPS_ANNOTATION_TEXT_MARKUP (object);

	switch (prop_id) {
	case PROP_TEXT_MARKUP_TYPE:
		pps_annotation_text_markup_set_markup_type (annot, g_value_get_enum (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_text_markup_init (PpsAnnotationTextMarkup *annot)
{
	PpsAnnotationPrivate *priv = GET_ANNOT_PRIVATE (PPS_ANNOTATION (annot));
	PpsAnnotationMarkupPrivate *markup_priv = GET_ANNOT_MARKUP_PRIVATE (PPS_ANNOTATION_MARKUP (annot));

	priv->type = PPS_ANNOTATION_TYPE_TEXT_MARKUP;
	markup_priv->can_have_popup = TRUE;
}

static void
pps_annotation_text_markup_class_init (PpsAnnotationTextMarkupClass *class)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (class);

	g_object_class->get_property = pps_annotation_text_markup_get_property;
	g_object_class->set_property = pps_annotation_text_markup_set_property;

	text_markup_properties[PROP_TEXT_MARKUP_TYPE] =
	    g_param_spec_enum ("type",
	                       "Type",
	                       "The text markup annotation type",
	                       PPS_TYPE_ANNOTATION_TEXT_MARKUP_TYPE,
	                       PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT,
	                       G_PARAM_READWRITE |
	                           G_PARAM_CONSTRUCT |
	                           G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (g_object_class,
	                                 PROP_TEXT_MARKUP_TYPE,
	                                 text_markup_properties[PROP_TEXT_MARKUP_TYPE]);
}

PpsAnnotation *
pps_annotation_text_markup_new (PpsPage *page, PpsAnnotationTextMarkupType markup_type)
{
	PpsAnnotation *annot = PPS_ANNOTATION (g_object_new (PPS_TYPE_ANNOTATION_TEXT_MARKUP,
	                                                     "page", page,
	                                                     "type", markup_type,
	                                                     NULL));
	return annot;
}

PpsAnnotation *
pps_annotation_text_markup_highlight_new (PpsPage *page)
{
	return pps_annotation_text_markup_new (page, PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT);
}

PpsAnnotation *
pps_annotation_text_markup_strike_out_new (PpsPage *page)
{
	return pps_annotation_text_markup_new (page, PPS_ANNOTATION_TEXT_MARKUP_STRIKE_OUT);
}

PpsAnnotation *
pps_annotation_text_markup_underline_new (PpsPage *page)
{
	return pps_annotation_text_markup_new (page, PPS_ANNOTATION_TEXT_MARKUP_UNDERLINE);
}

PpsAnnotation *
pps_annotation_text_markup_squiggly_new (PpsPage *page)
{
	return pps_annotation_text_markup_new (page, PPS_ANNOTATION_TEXT_MARKUP_SQUIGGLY);
}

PpsAnnotationTextMarkupType
pps_annotation_text_markup_get_markup_type (PpsAnnotationTextMarkup *annot)
{
	PpsAnnotationTextMarkupPrivate *priv = GET_ANNOT_TEXT_MARKUP_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT_MARKUP (annot),
	                      PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT);

	return priv->type;
}

gboolean
pps_annotation_text_markup_set_markup_type (PpsAnnotationTextMarkup *annot,
                                            PpsAnnotationTextMarkupType markup_type)
{
	PpsAnnotationTextMarkupPrivate *priv = GET_ANNOT_TEXT_MARKUP_PRIVATE (annot);

	g_return_val_if_fail (PPS_IS_ANNOTATION_TEXT_MARKUP (annot), FALSE);

	if (priv->type == markup_type)
		return FALSE;

	SAVE_PROPERTY (annot, "type", PPS_TYPE_ANNOTATION_TEXT_MARKUP_TYPE, g_value_set_enum, priv->type)

	priv->type = markup_type;
	g_object_notify_by_pspec (G_OBJECT (annot),
	                          text_markup_properties[PROP_TEXT_MARKUP_TYPE]);

	return TRUE;
}
