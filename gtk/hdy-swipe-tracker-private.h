/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "hdy-navigation-direction-private.h"
#include "hdy-swipeable-private.h"

G_BEGIN_DECLS

#define GTK_HDY_SWIPE_BORDER 32

#define GTK_TYPE_HDY_SWIPE_TRACKER (gtk_hdy_swipe_tracker_get_type())

G_DECLARE_FINAL_TYPE (GtkHdySwipeTracker, gtk_hdy_swipe_tracker, GTK, HDY_SWIPE_TRACKER, GObject)

GtkHdySwipeTracker *gtk_hdy_swipe_tracker_new (GtkHdySwipeable *swipeable);

GtkHdySwipeable    *gtk_hdy_swipe_tracker_get_swipeable (GtkHdySwipeTracker *self);

gboolean         gtk_hdy_swipe_tracker_get_enabled (GtkHdySwipeTracker *self);
void             gtk_hdy_swipe_tracker_set_enabled (GtkHdySwipeTracker *self,
                                                    gboolean            enabled);

gboolean         gtk_hdy_swipe_tracker_get_reversed (GtkHdySwipeTracker *self);
void             gtk_hdy_swipe_tracker_set_reversed (GtkHdySwipeTracker *self,
                                                     gboolean            reversed);

gboolean         gtk_hdy_swipe_tracker_get_allow_mouse_drag (GtkHdySwipeTracker *self);
void             gtk_hdy_swipe_tracker_set_allow_mouse_drag (GtkHdySwipeTracker *self,
                                                             gboolean            allow_mouse_drag);

void             gtk_hdy_swipe_tracker_shift_position (GtkHdySwipeTracker *self,
                                                       gdouble             delta);

void gtk_hdy_swipe_tracker_emit_begin_swipe (GtkHdySwipeTracker        *self,
                                             GtkHdyNavigationDirection  direction,
                                             gboolean                   direct);
void gtk_hdy_swipe_tracker_emit_update_swipe (GtkHdySwipeTracker *self,
                                              gdouble             progress);
void gtk_hdy_swipe_tracker_emit_end_swipe (GtkHdySwipeTracker *self,
                                           gint64              duration,
                                           gdouble             to);

G_END_DECLS
