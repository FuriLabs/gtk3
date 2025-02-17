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

#include "gtkradiobutton.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_VIEW_SWITCHER_BUTTON (gtk_hdy_view_switcher_button_get_type())

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkRadioButton, g_object_unref)

G_DECLARE_FINAL_TYPE (GtkHdyViewSwitcherButton, gtk_hdy_view_switcher_button, GTK, HDY_VIEW_SWITCHER_BUTTON, GtkRadioButton)

GtkWidget   *gtk_hdy_view_switcher_button_new (void);

const gchar *gtk_hdy_view_switcher_button_get_icon_name (GtkHdyViewSwitcherButton *self);
void         gtk_hdy_view_switcher_button_set_icon_name (GtkHdyViewSwitcherButton *self,
                                                         const gchar              *icon_name);

GtkIconSize gtk_hdy_view_switcher_button_get_icon_size (GtkHdyViewSwitcherButton *self);
void        gtk_hdy_view_switcher_button_set_icon_size (GtkHdyViewSwitcherButton *self,
                                                        GtkIconSize               icon_size);

gboolean gtk_hdy_view_switcher_button_get_needs_attention (GtkHdyViewSwitcherButton *self);
void     gtk_hdy_view_switcher_button_set_needs_attention (GtkHdyViewSwitcherButton *self,
                                                           gboolean                  needs_attention);

const gchar *gtk_hdy_view_switcher_button_get_label (GtkHdyViewSwitcherButton *self);
void         gtk_hdy_view_switcher_button_set_label (GtkHdyViewSwitcherButton *self,
                                                     const gchar              *label);

void gtk_hdy_view_switcher_button_set_narrow_ellipsize (GtkHdyViewSwitcherButton *self,
                                                        PangoEllipsizeMode        mode);

void gtk_hdy_view_switcher_button_get_size (GtkHdyViewSwitcherButton *self,
                                            gint                     *h_min_width,
                                            gint                     *h_nat_width,
                                            gint                     *v_min_width,
                                            gint                     *v_nat_width);

G_END_DECLS
