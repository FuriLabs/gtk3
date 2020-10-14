/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  GTK_HDY_NAVIGATION_DIRECTION_BACK,
  GTK_HDY_NAVIGATION_DIRECTION_FORWARD,
} GtkHdyNavigationDirection;

G_END_DECLS
