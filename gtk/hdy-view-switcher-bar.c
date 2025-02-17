/*
 * Copyright (C) 2019 Zander Brown <zbrown@gnome.org>
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "gtkactionbar.h"
#include "gtkrevealer.h"
#include "gtkprivatetypebuiltins.h"
#include "hdy-view-switcher-bar-private.h"
#include "hdy-view-switcher-private.h"

/**
 * SECTION:hdy-view-switcher-bar
 * @short_description: A view switcher action bar.
 * @title: GtkHdyViewSwitcherBar
 * @See_also: #GtkHdyViewSwitcher, #GtkHdyViewSwitcherTitle
 *
 * An action bar letting you switch between multiple views offered by a
 * #GtkStack, via an #GtkHdyViewSwitcher. It is designed to be put at the bottom of
 * a window and to be revealed only on really narrow windows e.g. on mobile
 * phones. It can't be revealed if there are less than two pages.
 *
 * You can conveniently bind the #GtkHdyViewSwitcherBar:reveal property to
 * #GtkHdyViewSwitcherTitle:title-visible to automatically reveal the view switcher
 * bar when the title label is displayed in place of the view switcher.
 *
 * An example of the UI definition for a common use case:
 * |[
 * <object class="GtkWindow"/>
 *   <child type="titlebar">
 *     <object class="HdyHeaderBar">
 *       <property name="centering-policy">strict</property>
 *       <child type="title">
 *         <object class="GtkHdyViewSwitcherTitle"
 *                 id="view_switcher_title">
 *           <property name="stack">stack</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 *   <child>
 *     <object class="GtkBox">
 *       <child>
 *         <object class="GtkStack" id="stack"/>
 *       </child>
 *       <child>
 *         <object class="GtkHdyViewSwitcherBar">
 *           <property name="stack">stack</property>
 *           <property name="reveal"
 *                     bind-source="view_switcher_title"
 *                     bind-property="title-visible"
 *                     bind-flags="sync-create"/>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * #GtkHdyViewSwitcherBar has a single CSS node with name viewswitcherbar.
 *
 * Since: 0.0.10
 */

enum {
  PROP_0,
  PROP_POLICY,
  PROP_STACK,
  PROP_REVEAL,
  LAST_PROP,
};

struct _GtkHdyViewSwitcherBar
{
  GtkBin parent_instance;

  GtkActionBar *action_bar;
  GtkRevealer *revealer;
  GtkHdyViewSwitcher *view_switcher;

  GtkHdyViewSwitcherPolicy policy;
  gboolean reveal;
};

static GParamSpec *props[LAST_PROP];

G_DEFINE_TYPE (GtkHdyViewSwitcherBar, gtk_hdy_view_switcher_bar, GTK_TYPE_BIN)

static void
count_children_cb (GtkWidget *widget,
                   gint      *count)
{
  (*count)++;
}

static void
update_bar_revealed (GtkHdyViewSwitcherBar *self)
{
  GtkStack *stack = gtk_hdy_view_switcher_get_stack (self->view_switcher);
  gint count = 0;

  if (self->reveal && stack)
    gtk_container_foreach (GTK_CONTAINER (stack), (GtkCallback) count_children_cb, &count);

  gtk_revealer_set_reveal_child (self->revealer, count > 1);
}

static void
gtk_hdy_view_switcher_bar_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GtkHdyViewSwitcherBar *self = GTK_HDY_VIEW_SWITCHER_BAR (object);

  switch (prop_id) {
  case PROP_POLICY:
    g_value_set_enum (value, gtk_hdy_view_switcher_bar_get_policy (self));
    break;
  case PROP_STACK:
    g_value_set_object (value, gtk_hdy_view_switcher_bar_get_stack (self));
    break;
  case PROP_REVEAL:
    g_value_set_boolean (value, gtk_hdy_view_switcher_bar_get_reveal (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gtk_hdy_view_switcher_bar_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GtkHdyViewSwitcherBar *self = GTK_HDY_VIEW_SWITCHER_BAR (object);

  switch (prop_id) {
  case PROP_POLICY:
    gtk_hdy_view_switcher_bar_set_policy (self, g_value_get_enum (value));
    break;
  case PROP_STACK:
    gtk_hdy_view_switcher_bar_set_stack (self, g_value_get_object (value));
    break;
  case PROP_REVEAL:
    gtk_hdy_view_switcher_bar_set_reveal (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gtk_hdy_view_switcher_bar_class_init (GtkHdyViewSwitcherBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gtk_hdy_view_switcher_bar_get_property;
  object_class->set_property = gtk_hdy_view_switcher_bar_set_property;

  /**
   * GtkHdyViewSwitcherBar:policy:
   *
   * The #GtkHdyViewSwitcherPolicy the #GtkHdyViewSwitcher should use to determine
   * which mode to use.
   *
   * Since: 0.0.10
   */
  props[PROP_POLICY] =
    g_param_spec_enum ("policy",
                       _("Policy"),
                       _("The policy to determine the mode to use"),
                       GTK_TYPE_HDY_VIEW_SWITCHER_POLICY, GTK_HDY_VIEW_SWITCHER_POLICY_NARROW,
                       G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * GtkHdyViewSwitcherBar:stack:
   *
   * The #GtkStack the #GtkHdyViewSwitcher controls.
   *
   * Since: 0.0.10
   */
  props[PROP_STACK] =
    g_param_spec_object ("stack",
                         _("Stack"),
                         _("Stack"),
                         GTK_TYPE_STACK,
                         G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * GtkHdyViewSwitcherBar:reveal:
   *
   * Whether the bar should be revealed or hidden.
   *
   * Since: 0.0.10
   */
  props[PROP_REVEAL] =
    g_param_spec_boolean ("reveal",
                         _("Reveal"),
                         _("Whether the view switcher is revealed"),
                         FALSE,
                         G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "viewswitcherbar");

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gtk/libgtk/ui/hdy-view-switcher-bar.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkHdyViewSwitcherBar, action_bar);
  gtk_widget_class_bind_template_child (widget_class, GtkHdyViewSwitcherBar, view_switcher);
}

static void
gtk_hdy_view_switcher_bar_init (GtkHdyViewSwitcherBar *self)
{
  /* This must be initialized before the template so the embedded view switcher
   * can pick up the correct default value.
   */
  self->policy = GTK_HDY_VIEW_SWITCHER_POLICY_NARROW;

  g_type_ensure (GTK_TYPE_HDY_VIEW_SWITCHER);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->revealer = GTK_REVEALER (gtk_bin_get_child (GTK_BIN (self->action_bar)));
  update_bar_revealed (self);
  gtk_revealer_set_transition_type (self->revealer, GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
}

/**
 * gtk_hdy_view_switcher_bar_new:
 *
 * Creates a new #GtkHdyViewSwitcherBar widget.
 *
 * Returns: a new #GtkHdyViewSwitcherBar
 *
 * Since: 0.0.10
 */
GtkWidget *
gtk_hdy_view_switcher_bar_new (void)
{
  return g_object_new (GTK_TYPE_HDY_VIEW_SWITCHER_BAR, NULL);
}

/**
 * gtk_hdy_view_switcher_bar_get_policy:
 * @self: a #GtkHdyViewSwitcherBar
 *
 * Gets the policy of @self.
 *
 * Returns: the policy of @self
 *
 * Since: 0.0.10
 */
GtkHdyViewSwitcherPolicy
gtk_hdy_view_switcher_bar_get_policy (GtkHdyViewSwitcherBar *self)
{
  g_return_val_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self), GTK_HDY_VIEW_SWITCHER_POLICY_NARROW);

  return self->policy;
}

/**
 * gtk_hdy_view_switcher_bar_set_policy:
 * @self: a #GtkHdyViewSwitcherBar
 * @policy: the new policy
 *
 * Sets the policy of @self.
 *
 * Since: 0.0.10
 */
void
gtk_hdy_view_switcher_bar_set_policy (GtkHdyViewSwitcherBar    *self,
                                      GtkHdyViewSwitcherPolicy  policy)
{
  g_return_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self));

  if (self->policy == policy)
    return;

  self->policy = policy;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_POLICY]);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/**
 * gtk_hdy_view_switcher_bar_get_stack:
 * @self: a #GtkHdyViewSwitcherBar
 *
 * Get the #GtkStack being controlled by the #GtkHdyViewSwitcher.
 *
 * Returns: (nullable) (transfer none): the #GtkStack, or %NULL if none has been set
 *
 * Since: 0.0.10
 */
GtkStack *
gtk_hdy_view_switcher_bar_get_stack (GtkHdyViewSwitcherBar *self)
{
  g_return_val_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self), NULL);

  return gtk_hdy_view_switcher_get_stack (self->view_switcher);
}

/**
 * gtk_hdy_view_switcher_bar_set_stack:
 * @self: a #GtkHdyViewSwitcherBar
 * @stack: (nullable): a #GtkStack
 *
 * Sets the #GtkStack to control.
 *
 * Since: 0.0.10
 */
void
gtk_hdy_view_switcher_bar_set_stack (GtkHdyViewSwitcherBar *self,
                                     GtkStack              *stack)
{
  GtkStack *previous_stack;

  g_return_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self));
  g_return_if_fail (stack == NULL || GTK_IS_STACK (stack));

  previous_stack = gtk_hdy_view_switcher_get_stack (self->view_switcher);

  if (previous_stack == stack)
    return;

  if (previous_stack)
    g_signal_handlers_disconnect_by_func (previous_stack, G_CALLBACK (update_bar_revealed), self);

  gtk_hdy_view_switcher_set_stack (self->view_switcher, stack);

  if (stack) {
    g_signal_connect_swapped (stack, "add", G_CALLBACK (update_bar_revealed), self);
    g_signal_connect_swapped (stack, "remove", G_CALLBACK (update_bar_revealed), self);
  }

  update_bar_revealed (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STACK]);
}

/**
 * gtk_hdy_view_switcher_bar_get_reveal:
 * @self: a #GtkHdyViewSwitcherBar
 *
 * Gets whether @self should be revealed or not.
 *
 * Returns: %TRUE if @self is revealed, %FALSE if not.
 *
 * Since: 0.0.10
 */
gboolean
gtk_hdy_view_switcher_bar_get_reveal (GtkHdyViewSwitcherBar *self)
{
  g_return_val_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self), FALSE);

  return self->reveal;
}

/**
 * gtk_hdy_view_switcher_bar_set_reveal:
 * @self: a #GtkHdyViewSwitcherBar
 * @reveal: %TRUE to reveal @self
 *
 * Sets whether @self should be revealed or not.
 *
 * Since: 0.0.10
 */
void
gtk_hdy_view_switcher_bar_set_reveal (GtkHdyViewSwitcherBar *self,
                                      gboolean               reveal)
{
  g_return_if_fail (GTK_IS_HDY_VIEW_SWITCHER_BAR (self));

  reveal = !!reveal;

  if (self->reveal == reveal)
    return;

  self->reveal = reveal;
  update_bar_revealed (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVEAL]);
}
