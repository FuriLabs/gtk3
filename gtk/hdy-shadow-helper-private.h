/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkenums.h"
#include "gtkwidget.h"
#include "gtkwidgetpath.h"

G_BEGIN_DECLS

#define GTK_HDY_TYPE_SHADOW_HELPER (gtk_hdy_shadow_helper_get_type())

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkStyleContext, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkWidgetPath, gtk_widget_path_unref)

G_DECLARE_FINAL_TYPE (GtkHdyShadowHelper, gtk_hdy_shadow_helper, GTK, HDY_SHADOW_HELPER, GObject)

GtkHdyShadowHelper * gtk_hdy_shadow_helper_new (GtkWidget *widget);

void                 gtk_hdy_shadow_helper_clear_cache (GtkHdyShadowHelper *self);

void                 gtk_hdy_shadow_helper_draw_shadow (GtkHdyShadowHelper *self,
                                                        cairo_t            *cr,
                                                        gint                width,
                                                        gint                height,
                                                        gdouble             progress,
                                                        GtkPanDirection     direction);

G_END_DECLS
