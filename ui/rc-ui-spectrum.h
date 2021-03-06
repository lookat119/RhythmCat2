/*
 * RhythmCat UI Spectrum Widget Header Declaration
 *
 * rc-ui-spectrum.h
 * This file is part of RhythmCat Music Player (GTK+ Version)
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
 *
 * RhythmCat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * RhythmCat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RhythmCat; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef HAVE_RC_UI_SPECTRUM_H
#define HAVE_RC_UI_SPECTRUM_H

#include <math.h>
#include <rclib.h>
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RC_UI_SPECTRUM_WIDGET_TYPE (rc_ui_spectrum_widget_get_type())
#define RC_UI_SPECTRUM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
    RC_UI_SPECTRUM_WIDGET_TYPE, RCUiSpectrumWidget))

#define RC_UI_SPECTRUM_WIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_SPECTRUM_WIDGET_TYPE, \
    RCUiSpectrumWidgetClass))
#define IS_RC_UI_SPECTRUM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_SPECTRUM_WIDGET_TYPE))
#define IS_RC_UI_SPECTRUM_WIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_SPECTRUM_WIDGET_TYPE))

/**
 * RCUiSpectrumStyle:
 * @RC_UI_SPECTRUM_STYLE_NONE: no visualizer
 * @RC_UI_SPECTRUM_STYLE_WAVE_MONO: wavescope style visualizer
 *     (mono channel)
 * @RC_UI_SPECTRUM_STYLE_WAVE_MULTI: wavescope style visualizer
 *     (multi-channel)
 * @RC_UI_SPECTRUM_STYLE_SPECTRUM: spectrum style visualizer
 * 
 * The visualizer style of the spectrum widget.
 */

typedef enum {
    RC_UI_SPECTRUM_STYLE_NONE,
    RC_UI_SPECTRUM_STYLE_WAVE_MONO,
    RC_UI_SPECTRUM_STYLE_WAVE_MULTI,
    RC_UI_SPECTRUM_STYLE_SPECTRUM,
}RCUiSpectrumStyle;

typedef struct _RCUiSpectrumWidget RCUiSpectrumWidget;
typedef struct _RCUiSpectrumWidgetClass RCUiSpectrumWidgetClass;
typedef struct _RCUiSpectrumWidgetPrivate RCUiSpectrumWidgetPrivate;
    
/**
 * RCUiSpectrumWidget:
 *
 * The structure used in object.
 */

struct _RCUiSpectrumWidget {
    /*< private >*/
    GtkWidget widget;
    RCUiSpectrumWidgetPrivate *priv;
};

/**
 * RCUiSpectrumWidgetClass:
 *
 * The class data.
 */

struct _RCUiSpectrumWidgetClass {
    /*< private >*/
    GtkWidgetClass parent_class;
};


/*< private >*/
GType rc_ui_spectrum_widget_get_type();

/*< public >*/
GtkWidget *rc_ui_spectrum_widget_new();
void rc_ui_spectrum_widget_set_fps(RCUiSpectrumWidget *spectrum, guint fps);
void rc_ui_spectrum_widget_set_style(RCUiSpectrumWidget *spectrum,
    RCUiSpectrumStyle style);
guint rc_ui_spectrum_widget_get_fps(RCUiSpectrumWidget *spectrum);
RCUiSpectrumStyle rc_ui_spectrum_widget_get_style(
    RCUiSpectrumWidget *spectrum);
void rc_ui_spectrum_widget_clean(RCUiSpectrumWidget *spectrum);

G_END_DECLS

#endif

