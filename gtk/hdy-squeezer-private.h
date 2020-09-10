/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkcontainer.h"
#include "gtkenums.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_SQUEEZER (gtk_hdy_squeezer_get_type ())

G_DECLARE_FINAL_TYPE (GtkHdySqueezer, gtk_hdy_squeezer, GTK, HDY_SQUEEZER, GtkContainer)

typedef enum {
  GTK_HDY_SQUEEZER_TRANSITION_TYPE_NONE,
  GTK_HDY_SQUEEZER_TRANSITION_TYPE_CROSSFADE,
} GtkHdySqueezerTransitionType;

GtkWidget *gtk_hdy_squeezer_new (void);

gboolean gtk_hdy_squeezer_get_homogeneous (GtkHdySqueezer *self);
void     gtk_hdy_squeezer_set_homogeneous (GtkHdySqueezer *self,
                                           gboolean        homogeneous);

guint gtk_hdy_squeezer_get_transition_duration (GtkHdySqueezer *self);
void  gtk_hdy_squeezer_set_transition_duration (GtkHdySqueezer *self,
                                                guint           duration);

GtkHdySqueezerTransitionType gtk_hdy_squeezer_get_transition_type (GtkHdySqueezer *self);
void                         gtk_hdy_squeezer_set_transition_type (GtkHdySqueezer               *self,
                                                                   GtkHdySqueezerTransitionType  transition);

gboolean gtk_hdy_squeezer_get_transition_running (GtkHdySqueezer *self);

gboolean gtk_hdy_squeezer_get_interpolate_size (GtkHdySqueezer *self);
void     gtk_hdy_squeezer_set_interpolate_size (GtkHdySqueezer *self,
                                                gboolean        interpolate_size);

GtkWidget *gtk_hdy_squeezer_get_visible_child (GtkHdySqueezer *self);

gboolean gtk_hdy_squeezer_get_child_enabled (GtkHdySqueezer *self,
                                             GtkWidget      *child);
void     gtk_hdy_squeezer_set_child_enabled (GtkHdySqueezer *self,
                                             GtkWidget      *child,
                                             gboolean        enabled);

gfloat gtk_hdy_squeezer_get_xalign (GtkHdySqueezer *self);
void   gtk_hdy_squeezer_set_xalign (GtkHdySqueezer *self,
                                    gfloat          xalign);

gfloat gtk_hdy_squeezer_get_yalign (GtkHdySqueezer *self);
void   gtk_hdy_squeezer_set_yalign (GtkHdySqueezer *self,
                                    gfloat          yalign);

G_END_DECLS
