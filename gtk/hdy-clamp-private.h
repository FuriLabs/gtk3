/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkbin.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_CLAMP (gtk_hdy_clamp_get_type())

G_DECLARE_FINAL_TYPE (GtkHdyClamp, gtk_hdy_clamp, GTK, HDY_CLAMP, GtkBin)

GtkWidget *gtk_hdy_clamp_new (void);

gint gtk_hdy_clamp_get_maximum_size (GtkHdyClamp *self);
void gtk_hdy_clamp_set_maximum_size (GtkHdyClamp *self,
                                     gint         maximum_size);

gint gtk_hdy_clamp_get_tightening_threshold (GtkHdyClamp *self);
void gtk_hdy_clamp_set_tightening_threshold (GtkHdyClamp *self,
                                             gint         tightening_threshold);

G_END_DECLS
