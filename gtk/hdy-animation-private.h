/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkwidget.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_ANIMATION (gtk_hdy_animation_get_type())

typedef struct _GtkHdyAnimation GtkHdyAnimation;

typedef void   (*GtkHdyAnimationValueCallback) (gdouble  value,
                                                gpointer user_data);
typedef void   (*GtkHdyAnimationDoneCallback)  (gpointer user_data);
typedef double (*GtkHdyAnimationEasingFunc)    (gdouble  t);

GType            gtk_hdy_animation_get_type  (void) G_GNUC_CONST;

GtkHdyAnimation *gtk_hdy_animation_new       (GtkWidget                    *widget,
                                              gdouble                       from,
                                              gdouble                       to,
                                              gint64                        duration,
                                              GtkHdyAnimationEasingFunc     easing_func,
                                              GtkHdyAnimationValueCallback  value_cb,
                                              GtkHdyAnimationDoneCallback   done_cb,
                                              gpointer                      user_data);

GtkHdyAnimation *gtk_hdy_animation_ref       (GtkHdyAnimation *self);
void             gtk_hdy_animation_unref     (GtkHdyAnimation *self);

void             gtk_hdy_animation_start     (GtkHdyAnimation *self);
void             gtk_hdy_animation_stop      (GtkHdyAnimation *self);

gdouble          gtk_hdy_animation_get_value (GtkHdyAnimation *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkHdyAnimation, gtk_hdy_animation_unref)

gboolean gtk_hdy_get_enable_animations (GtkWidget *widget);

gdouble gtk_hdy_lerp (gdouble a, gdouble b, gdouble t);

gdouble gtk_hdy_ease_out_cubic (gdouble t);

G_END_DECLS
