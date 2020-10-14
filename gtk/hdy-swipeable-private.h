/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include "gtkwidget.h"
#include "hdy-navigation-direction-private.h"

G_BEGIN_DECLS

typedef struct _GtkHdySwipeTracker GtkHdySwipeTracker;

#define GTK_TYPE_HDY_SWIPEABLE (gtk_hdy_swipeable_get_type ())

G_DECLARE_INTERFACE (GtkHdySwipeable, gtk_hdy_swipeable, GTK, HDY_SWIPEABLE, GtkWidget)

/**
 * GtkHdySwipeableInterface:
 * @parent: The parent interface.
 * @switch_child: Switches visible child.
 * @get_swipe_tracker: Gets the swipe tracker.
 * @get_distance: Gets the swipe distance.
 * @get_snap_points: Gets the snap points
 * @get_progress: Gets the current progress.
 * @get_cancel_progress: Gets the cancel progress.
 * @get_swipe_area: Gets the swipeable rectangle.
 *
 * An interface for swipeable widgets.
 *
 * Since: 1.0
 **/
struct _GtkHdySwipeableInterface
{
  GTypeInterface parent;

  void (*switch_child) (GtkHdySwipeable *self,
                        guint            index,
                        gint64           duration);

  GtkHdySwipeTracker * (*get_swipe_tracker)   (GtkHdySwipeable *self);
  gdouble              (*get_distance)        (GtkHdySwipeable *self);
  gdouble *            (*get_snap_points)     (GtkHdySwipeable *self,
                                               gint            *n_snap_points);
  gdouble              (*get_progress)        (GtkHdySwipeable *self);
  gdouble              (*get_cancel_progress) (GtkHdySwipeable *self);
  void                 (*get_swipe_area)      (GtkHdySwipeable           *self,
                                               GtkHdyNavigationDirection  navigation_direction,
                                               gboolean                   is_drag,
                                               GdkRectangle              *rect);

  /*< private >*/
  gpointer padding[4];
};

void gtk_hdy_swipeable_switch_child (GtkHdySwipeable *self,
                                     guint            index,
                                     gint64           duration);

void gtk_hdy_swipeable_emit_child_switched (GtkHdySwipeable *self,
                                            guint            index,
                                            gint64           duration);

GtkHdySwipeTracker *gtk_hdy_swipeable_get_swipe_tracker (GtkHdySwipeable *self);
gdouble             gtk_hdy_swipeable_get_distance        (GtkHdySwipeable *self);
gdouble            *gtk_hdy_swipeable_get_snap_points     (GtkHdySwipeable *self,
                                                           gint            *n_snap_points);
gdouble             gtk_hdy_swipeable_get_progress        (GtkHdySwipeable *self);
gdouble             gtk_hdy_swipeable_get_cancel_progress (GtkHdySwipeable *self);
void                gtk_hdy_swipeable_get_swipe_area      (GtkHdySwipeable           *self,
                                                           GtkHdyNavigationDirection  navigation_direction,
                                                           gboolean                   is_drag,
                                                           GdkRectangle              *rect);

G_END_DECLS
