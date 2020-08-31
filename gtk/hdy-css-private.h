/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkwidget.h"

G_BEGIN_DECLS

void gtk_hdy_css_measure (GtkWidget      *widget,
                          GtkOrientation  orientation,
                          gint           *minimum,
                          gint           *natural);

void gtk_hdy_css_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation);

G_END_DECLS
