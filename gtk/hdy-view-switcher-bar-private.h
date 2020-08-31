/*
 * Copyright (C) 2019 Zander Brown <zbrown@gnome.org>
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkbin.h"
#include "gtkstack.h"
#include "hdy-view-switcher-private.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_VIEW_SWITCHER_BAR (gtk_hdy_view_switcher_bar_get_type())

G_DECLARE_FINAL_TYPE (GtkHdyViewSwitcherBar, gtk_hdy_view_switcher_bar, GTK, HDY_VIEW_SWITCHER_BAR, GtkBin)

GtkWidget *gtk_hdy_view_switcher_bar_new (void);

GtkHdyViewSwitcherPolicy gtk_hdy_view_switcher_bar_get_policy (GtkHdyViewSwitcherBar *self);
void                     gtk_hdy_view_switcher_bar_set_policy (GtkHdyViewSwitcherBar    *self,
                                                               GtkHdyViewSwitcherPolicy  policy);

GtkStack *gtk_hdy_view_switcher_bar_get_stack (GtkHdyViewSwitcherBar *self);
void      gtk_hdy_view_switcher_bar_set_stack (GtkHdyViewSwitcherBar *self,
                                               GtkStack              *stack);

gboolean gtk_hdy_view_switcher_bar_get_reveal (GtkHdyViewSwitcherBar *self);
void     gtk_hdy_view_switcher_bar_set_reveal (GtkHdyViewSwitcherBar *self,
                                               gboolean               reveal);

G_END_DECLS
