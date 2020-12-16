/*
 * Copyright (C) 2020 Felix Häcker <haeckerfelix@gnome.org>
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

#define GTK_TYPE_HDY_FLAP (gtk_hdy_flap_get_type ())

G_DECLARE_FINAL_TYPE (GtkHdyFlap, gtk_hdy_flap, GTK, HDY_FLAP, GtkContainer)

typedef enum {
  GTK_HDY_FLAP_FOLD_POLICY_NEVER,
  GTK_HDY_FLAP_FOLD_POLICY_ALWAYS,
  GTK_HDY_FLAP_FOLD_POLICY_AUTO,
} GtkHdyFlapFoldPolicy;

typedef enum {
  GTK_HDY_FLAP_TRANSITION_TYPE_OVER,
  GTK_HDY_FLAP_TRANSITION_TYPE_UNDER,
  GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE,
} GtkHdyFlapTransitionType;

GtkWidget *gtk_hdy_flap_new (void);

GtkWidget *gtk_hdy_flap_get_content (GtkHdyFlap *self);
void       gtk_hdy_flap_set_content (GtkHdyFlap *self,
                                     GtkWidget  *content);

GtkWidget *gtk_hdy_flap_get_flap (GtkHdyFlap *self);
void       gtk_hdy_flap_set_flap (GtkHdyFlap *self,
                                  GtkWidget  *flap);

GtkWidget *gtk_hdy_flap_get_separator (GtkHdyFlap *self);
void       gtk_hdy_flap_set_separator (GtkHdyFlap *self,
                                       GtkWidget  *separator);

GtkPackType gtk_hdy_flap_get_flap_position (GtkHdyFlap  *self);
void        gtk_hdy_flap_set_flap_position (GtkHdyFlap  *self,
                                            GtkPackType  position);

gboolean gtk_hdy_flap_get_reveal_flap (GtkHdyFlap *self);
void     gtk_hdy_flap_set_reveal_flap (GtkHdyFlap *self,
                                       gboolean    reveal_flap);

guint gtk_hdy_flap_get_reveal_duration (GtkHdyFlap *self);
void  gtk_hdy_flap_set_reveal_duration (GtkHdyFlap *self,
                                        guint       duration);

gdouble gtk_hdy_flap_get_reveal_progress (GtkHdyFlap *self);

GtkHdyFlapFoldPolicy gtk_hdy_flap_get_fold_policy (GtkHdyFlap           *self);
void                 gtk_hdy_flap_set_fold_policy (GtkHdyFlap           *self,
                                                   GtkHdyFlapFoldPolicy  policy);

guint gtk_hdy_flap_get_fold_duration (GtkHdyFlap *self);
void  gtk_hdy_flap_set_fold_duration (GtkHdyFlap *self,
                                      guint       duration);

gboolean gtk_hdy_flap_get_folded (GtkHdyFlap *self);

gboolean gtk_hdy_flap_get_locked (GtkHdyFlap *self);
void     gtk_hdy_flap_set_locked (GtkHdyFlap *self,
                                  gboolean    locked);

GtkHdyFlapTransitionType gtk_hdy_flap_get_transition_type (GtkHdyFlap               *self);
void                     gtk_hdy_flap_set_transition_type (GtkHdyFlap               *self,
                                                           GtkHdyFlapTransitionType  transition_type);

gboolean gtk_hdy_flap_get_modal (GtkHdyFlap *self);
void     gtk_hdy_flap_set_modal (GtkHdyFlap *self,
                                 gboolean    modal);

gboolean gtk_hdy_flap_get_swipe_to_open (GtkHdyFlap *self);
void     gtk_hdy_flap_set_swipe_to_open (GtkHdyFlap *self,
                                         gboolean    swipe_to_open);

gboolean gtk_hdy_flap_get_swipe_to_close (GtkHdyFlap *self);
void     gtk_hdy_flap_set_swipe_to_close (GtkHdyFlap *self,
                                          gboolean    swipe_to_close);

G_END_DECLS
