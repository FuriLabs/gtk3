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

G_BEGIN_DECLS

#define GTK_TYPE_HDY_VIEW_SWITCHER (gtk_hdy_view_switcher_get_type())

typedef enum {
  GTK_HDY_VIEW_SWITCHER_POLICY_AUTO,
  GTK_HDY_VIEW_SWITCHER_POLICY_NARROW,
  GTK_HDY_VIEW_SWITCHER_POLICY_WIDE,
} GtkHdyViewSwitcherPolicy;

G_DECLARE_FINAL_TYPE (GtkHdyViewSwitcher, gtk_hdy_view_switcher, GTK, HDY_VIEW_SWITCHER, GtkBin)

GtkWidget *hdy_view_switcher_new (void);

GtkHdyViewSwitcherPolicy gtk_hdy_view_switcher_get_policy (GtkHdyViewSwitcher *self);
void                     gtk_hdy_view_switcher_set_policy (GtkHdyViewSwitcher       *self,
                                                           GtkHdyViewSwitcherPolicy  policy);

PangoEllipsizeMode gtk_hdy_view_switcher_get_narrow_ellipsize (GtkHdyViewSwitcher *self);
void               gtk_hdy_view_switcher_set_narrow_ellipsize (GtkHdyViewSwitcher *self,
                                                               PangoEllipsizeMode  mode);

GtkStack *gtk_hdy_view_switcher_get_stack (GtkHdyViewSwitcher *self);
void      gtk_hdy_view_switcher_set_stack (GtkHdyViewSwitcher *self,
                                           GtkStack           *stack);

G_END_DECLS
