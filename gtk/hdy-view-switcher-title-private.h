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

#include "hdy-view-switcher-private.h"

G_BEGIN_DECLS

#define GTK_TYPE_HDY_VIEW_SWITCHER_TITLE (gtk_hdy_view_switcher_title_get_type())

G_DECLARE_FINAL_TYPE (GtkHdyViewSwitcherTitle, gtk_hdy_view_switcher_title, GTK, HDY_VIEW_SWITCHER_TITLE, GtkBin)

GtkHdyViewSwitcherTitle *gtk_hdy_view_switcher_title_new (void);

GtkHdyViewSwitcherPolicy gtk_hdy_view_switcher_title_get_policy (GtkHdyViewSwitcherTitle *self);
void                     gtk_hdy_view_switcher_title_set_policy (GtkHdyViewSwitcherTitle  *self,
                                                                 GtkHdyViewSwitcherPolicy  policy);

GtkStack *gtk_hdy_view_switcher_title_get_stack (GtkHdyViewSwitcherTitle *self);
void      gtk_hdy_view_switcher_title_set_stack (GtkHdyViewSwitcherTitle *self,
                                                 GtkStack                *stack);

const gchar *gtk_hdy_view_switcher_title_get_title (GtkHdyViewSwitcherTitle *self);
void         gtk_hdy_view_switcher_title_set_title (GtkHdyViewSwitcherTitle *self,
                                                    const gchar             *title);

const gchar *gtk_hdy_view_switcher_title_get_subtitle (GtkHdyViewSwitcherTitle *self);
void         gtk_hdy_view_switcher_title_set_subtitle (GtkHdyViewSwitcherTitle *self,
                                                       const gchar             *subtitle);

gboolean gtk_hdy_view_switcher_title_get_view_switcher_enabled (GtkHdyViewSwitcherTitle *self);
void     gtk_hdy_view_switcher_title_set_view_switcher_enabled (GtkHdyViewSwitcherTitle *self,
                                                                gboolean                 enabled);

gboolean gtk_hdy_view_switcher_title_get_title_visible (GtkHdyViewSwitcherTitle *self);

G_END_DECLS
