/*
 * Copyright (C) 2020 Felix Häcker <haeckerfelix@gnome.org>
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-flap-private.h"

#include <math.h>

#include "gtkbuildable.h"
#include "gtkgesturemultipress.h"
#include "gtkeventcontrollerkey.h"
#include "gtkorientable.h"
#include "gtkstylecontext.h"
#include "gtksizerequest.h"
#include "gtkprivatetypebuiltins.h"
#include "gtktypebuiltins.h"
#include "hdy-animation-private.h"
#include "hdy-shadow-helper-private.h"
#include "hdy-swipeable-private.h"
#include "hdy-swipe-tracker-private.h"

/**
 * SECTION:hdy-flap
 * @short_description: An adaptive container acting like a box or an overlay.
 * @Title: GtkHdyFlap
 *
 * The #GtkHdyFlap widget can display its children like a #GtkBox does or like a
 * #GtkOverlay does, according to the #GtkHdyFlap:fold-policy value.
 *
 * #GtkHdyFlap has at most three children: #GtkHdyFlap:content, #GtkHdyFlap:flap and
 * #GtkHdyFlap:separator. Content is the primary child, flap is displayed next to
 * it when unfolded, or overlays it when folded. Flap can be shown or hidden by
 * changing the #GtkHdyFlap:reveal-flap value, as well as via swipe gestures if
 * #GtkHdyFlap:swipe-to-open and/or #GtkHdyFlap:swipe-to-close are set to %TRUE.
 *
 * Optionally, a separator can be provided, which would be displayed between
 * the content and the flap when there's no shadow to separate them, depending
 * on the transition type.
 *
 * #GtkHdyFlap:flap is transparent by default; add the .background style class to
 * it if this is unwanted.
 *
 * If #GtkHdyFlap:modal is set to %TRUE, content becomes completely inaccessible
 * when the flap is revealed when folded.
 *
 * The position of the flap and separator children relative to the content is
 * determined by orientation, as well as  #GtkHdyFlap:flap-position value.
 *
 * Folding the flap will automatically hide the flap widget, and unfolding it
 * will automatically reveal it. If this behavior is not desired, the
 * #GtkHdyFlap:locked property can be used to override it.
 *
 * Common use cases include sidebars, header bars that need to be able to
 * overlap the window content (for example, in fullscreen mode) and bottom
 * sheets.
 *
 * # GtkHdyFlap as GtkBuildable
 *
 * The #GtkHdyFlap implementation of the #GtkBuildable interface supports setting
 * the flap child by specifying “flap” as the “type” attribute of a
 * &lt;child&gt; element, and separator by specifying “separator”. Specifying
 * “content” child type or omitting it results in setting the content child.
 *
 * # CSS nodes
 *
 * #GtkHdyFlap has a single CSS node with name flap. The node will get the style
 * classes .folded when it is folded, and .unfolded when it's not.
 *
 * Since: 1.1
 */

/**
 * GtkHdyFlapFoldPolicy:
 * @GTK_HDY_FLAP_FOLD_POLICY_NEVER: Disable folding, the flap cannot reach narrow
 *   sizes.
 * @GTK_HDY_FLAP_FOLD_POLICY_ALWAYS: Keep the flap always folded.
 * @GTK_HDY_FLAP_FOLD_POLICY_AUTO: Fold and unfold the flap based on available
 *   space.
 *
 * These enumeration values describe the possible folding behavior in a #GtkHdyFlap
 * widget.
 *
 * Since: 1.1
 */

/**
 * GtkHdyFlapTransitionType:
 * @GTK_HDY_FLAP_TRANSITION_TYPE_OVER: The flap slides over the content, which is
 *   dimmed. When folded, only the flap can be swiped.
 * @GTK_HDY_FLAP_TRANSITION_TYPE_UNDER: The content slides over the flap. Only the
 *   content can be swiped.
 * @GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE: The flap slides offscreen when hidden,
 *   neither the flap nor content overlap each other. Both widgets can be
 *   swiped.
 *
 * These enumeration values describe the possible transitions between children
 * in a #GtkHdyFlap widget, as well as which areas can be swiped via
 * #GtkHdyFlap:swipe-to-open and #GtkHdyFlap:swipe-to-close.
 *
 * New values may be added to this enum over time.
 *
 * Since: 1.1
 */

typedef struct {
  GtkWidget *widget;
  GdkWindow *window;
  GtkAllocation allocation;
} ChildInfo;

struct _GtkHdyFlap
{
  GtkContainer parent_instance;

  ChildInfo content;
  ChildInfo flap;
  ChildInfo separator;

  GtkHdyFlapFoldPolicy fold_policy;
  GtkHdyFlapTransitionType transition_type;
  GtkPackType flap_position;
  gboolean reveal_flap;
  gboolean locked;
  gboolean folded;

  guint fold_duration;
  gdouble fold_progress;
  GtkHdyAnimation *fold_animation;

  guint reveal_duration;
  gdouble reveal_progress;
  GtkHdyAnimation *reveal_animation;

  gboolean schedule_fold;

  GtkOrientation orientation;

  GtkHdyShadowHelper *shadow_helper;

  gboolean swipe_to_open;
  gboolean swipe_to_close;
  GtkHdySwipeTracker *tracker;
  gboolean swipe_active;

  gboolean modal;
  GtkGesture *click_gesture;
  GtkEventController *key_controller;
};

static void gtk_hdy_flap_buildable_init (GtkBuildableIface *iface);
static void gtk_hdy_flap_swipeable_init (GtkHdySwipeableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkHdyFlap, gtk_hdy_flap, GTK_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, gtk_hdy_flap_buildable_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_HDY_SWIPEABLE, gtk_hdy_flap_swipeable_init))

enum {
  PROP_0,
  PROP_CONTENT,
  PROP_FLAP,
  PROP_SEPARATOR,
  PROP_FLAP_POSITION,
  PROP_REVEAL_FLAP,
  PROP_REVEAL_DURATION,
  PROP_REVEAL_PROGRESS,
  PROP_FOLD_POLICY,
  PROP_FOLD_DURATION,
  PROP_FOLDED,
  PROP_LOCKED,
  PROP_TRANSITION_TYPE,
  PROP_MODAL,
  PROP_SWIPE_TO_OPEN,
  PROP_SWIPE_TO_CLOSE,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_ORIENTATION,
};

static GParamSpec *props[LAST_PROP];

static void
update_swipe_tracker (GtkHdyFlap *self)
{
  gboolean reverse = self->flap_position == GTK_PACK_START;

  if (!self->tracker)
    return;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL &&
      gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL)
    reverse = !reverse;

  gtk_hdy_swipe_tracker_set_enabled (self->tracker, self->flap.widget &&
                                     (self->swipe_to_open || self->swipe_to_close));
  gtk_hdy_swipe_tracker_set_reversed (self->tracker, reverse);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->tracker),
                                  self->orientation);
}

static void
set_orientation (GtkHdyFlap     *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

  gtk_widget_queue_resize (GTK_WIDGET (self));
  update_swipe_tracker (self);

  g_object_notify (G_OBJECT (self), "orientation");
}

static void
update_child_visibility (GtkHdyFlap *self)
{
  gboolean visible = self->reveal_progress > 0;

  if (self->flap.widget)
    gtk_widget_set_child_visible (self->flap.widget, visible);

  if (self->separator.widget)
    gtk_widget_set_child_visible (self->separator.widget, visible);

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  if (self->flap.widget) {
    if (visible)
      gdk_window_show (self->flap.window);
    else
      gdk_window_hide (self->flap.window);
  }

  if (self->separator.widget) {
    if (visible)
      gdk_window_show (self->separator.window);
    else
      gdk_window_hide (self->separator.window);
  }

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
set_reveal_progress (GtkHdyFlap *self,
                     gdouble     progress)
{
  self->reveal_progress = progress;

  update_child_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVEAL_PROGRESS]);
}

static void
fold_animation_value_cb (gdouble     value,
                         GtkHdyFlap *self)
{
  self->fold_progress = value;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
fold_animation_done_cb (GtkHdyFlap *self)
{
  g_clear_pointer (&self->fold_animation, gtk_hdy_animation_unref);
}

static void
animate_fold (GtkHdyFlap *self)
{
  if (self->fold_animation)
    gtk_hdy_animation_stop (self->fold_animation);

  self->fold_animation =
    gtk_hdy_animation_new (GTK_WIDGET (self),
                           self->fold_progress,
                           self->folded ? 1 : 0,
                           /* When the flap is completely hidden, we can skip animation */
                           (self->reveal_progress > 0) ? self->fold_duration : 0,
                           gtk_hdy_ease_out_cubic,
                           (GtkHdyAnimationValueCallback) fold_animation_value_cb,
                           (GtkHdyAnimationDoneCallback) fold_animation_done_cb,
                           self);

  gtk_hdy_animation_start (self->fold_animation);
}

static void
reveal_animation_value_cb (gdouble     value,
                           GtkHdyFlap *self)
{
  set_reveal_progress (self, value);
}

static void
reveal_animation_done_cb (GtkHdyFlap *self)
{
  g_clear_pointer (&self->reveal_animation, gtk_hdy_animation_unref);

  if (self->reveal_progress <= 0 ||
      self->transition_type == GTK_HDY_FLAP_TRANSITION_TYPE_UNDER)
    gtk_hdy_shadow_helper_clear_cache (self->shadow_helper);

  if (self->schedule_fold) {
    self->schedule_fold = FALSE;

    animate_fold (self);
  }

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
animate_reveal (GtkHdyFlap *self,
                gdouble     to,
                gint64      duration)
{
  if (self->reveal_animation)
    gtk_hdy_animation_stop (self->reveal_animation);

  self->reveal_animation =
    gtk_hdy_animation_new (GTK_WIDGET (self),
                           self->reveal_progress,
                           to,
                           duration,
                           gtk_hdy_ease_out_cubic,
                           (GtkHdyAnimationValueCallback) reveal_animation_value_cb,
                           (GtkHdyAnimationDoneCallback) reveal_animation_done_cb,
                           self);

  gtk_hdy_animation_start (self->reveal_animation);
}

static void
set_reveal_flap (GtkHdyFlap *self,
                 gboolean    reveal_flap,
                 guint64     duration,
                 gboolean    emit_child_switched)
{
  reveal_flap = !!reveal_flap;

  if (self->reveal_flap == reveal_flap)
    return;

  self->reveal_flap = reveal_flap;

  if (!self->swipe_active) {
    animate_reveal (self, reveal_flap ? 1 : 0, duration);

    if (emit_child_switched)
      gtk_hdy_swipeable_emit_child_switched (GTK_HDY_SWIPEABLE (self), reveal_flap ? 1 : 0, duration);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVEAL_FLAP]);
}

static void
set_folded (GtkHdyFlap *self,
            gboolean    folded)
{
  GtkStyleContext *context;

  folded = !!folded;

  if (self->folded == folded)
    return;

  self->folded = folded;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

   /* When unlocked, folding should also hide flap. We don't want two concurrent
    * animations in this case, instead only animate reveal and schedule a fold
    * after it finishes, which will be skipped because the flap is fuly hidden.
    * Meanwhile if it's unfolding, animate folding immediately. */
  if (!self->locked && folded)
    self->schedule_fold = TRUE;
  else
    animate_fold (self);

  if (!self->locked)
    set_reveal_flap (self, !self->folded, self->fold_duration, TRUE);

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  if (folded) {
    gtk_style_context_add_class (context, "folded");
    gtk_style_context_remove_class (context, "unfolded");
  } else {
    gtk_style_context_remove_class (context, "folded");
    gtk_style_context_add_class (context, "unfolded");
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOLDED]);
}

static inline GtkPackType
get_start_or_end (GtkHdyFlap *self)
{
  GtkTextDirection direction = gtk_widget_get_direction (GTK_WIDGET (self));
  gboolean is_rtl = direction == GTK_TEXT_DIR_RTL;
  gboolean is_horiz = self->orientation == GTK_ORIENTATION_HORIZONTAL;

  return (is_rtl && is_horiz) ? GTK_PACK_END : GTK_PACK_START;
}

static void
begin_swipe_cb (GtkHdySwipeTracker        *tracker,
                GtkHdyNavigationDirection  direction,
                gboolean                   direct,
                GtkHdyFlap                *self)
{
  if (self->reveal_progress <= 0 && !self->swipe_to_open)
    return;

  if (self->reveal_progress >= 1 && !self->swipe_to_close)
    return;

  if (self->reveal_animation)
    gtk_hdy_animation_stop (self->reveal_animation);

  self->swipe_active = TRUE;
}

static void
update_swipe_cb (GtkHdySwipeTracker *tracker,
                 gdouble             progress,
                 GtkHdyFlap         *self)
{
  if (!self->swipe_active)
    return;

  set_reveal_progress (self, progress);
}

static void
end_swipe_cb (GtkHdySwipeTracker *tracker,
              gint64              duration,
              gdouble             to,
              GtkHdyFlap         *self)
{
  if (!self->swipe_active)
    return;

  self->swipe_active = FALSE;

  if ((to > 0) == self->reveal_flap)
    animate_reveal (self, to, duration);
  else
    set_reveal_flap (self, to > 0, duration, FALSE);
}

static void
released_cb (GtkGestureMultiPress *gesture,
             gint                  n_press,
             gdouble               x,
             gdouble               y,
             GtkHdyFlap           *self)
{
  if (self->reveal_progress <= 0 || self->fold_progress <= 0) {
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  if (x >= self->flap.allocation.x &&
      x <= self->flap.allocation.x + self->flap.allocation.width &&
      y >= self->flap.allocation.y &&
      y <= self->flap.allocation.y + self->flap.allocation.height) {
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  gtk_hdy_flap_set_reveal_flap (self, FALSE);
}

static gboolean
key_pressed_cb (GtkEventControllerKey *controller,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        modifiers,
                GtkHdyFlap            *self)
{
  if (keyval == GDK_KEY_Escape &&
      self->reveal_progress > 0 &&
      self->fold_progress > 0) {
    gtk_hdy_flap_set_reveal_flap (self, FALSE);

    return GDK_EVENT_STOP;
  }

  return GDK_EVENT_PROPAGATE;
}

static void
register_window (GtkHdyFlap *self,
                 ChildInfo  *info)
{
  GdkWindowAttr attributes = { 0 };
  GdkWindowAttributesType attributes_mask;

  if (!info->widget)
    return;

  attributes.x = info->allocation.x;
  attributes.y = info->allocation.y;
  attributes.width = info->allocation.width;
  attributes.height = info->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (info->widget);
  attributes.event_mask = gtk_widget_get_events (info->widget);
  attributes_mask = (GDK_WA_X | GDK_WA_Y) | GDK_WA_VISUAL;

  attributes.event_mask = gtk_widget_get_events (GTK_WIDGET (self)) |
                          gtk_widget_get_events (info->widget);

  info->window = gdk_window_new (gtk_widget_get_window (GTK_WIDGET (self)),
                                 &attributes, attributes_mask);
  gtk_widget_register_window (GTK_WIDGET (self), info->window);

  gtk_widget_set_parent_window (info->widget, info->window);

  gdk_window_show (info->window);
}

static void
unregister_window (GtkHdyFlap *self,
                   ChildInfo  *info)
{
  if (!info->window)
    return;

  gtk_widget_unregister_window (GTK_WIDGET (self), info->window);
  gdk_window_destroy (info->window);
  info->window = NULL;
}

static gboolean
transition_is_content_above_flap (GtkHdyFlap *self)
{
  switch (self->transition_type) {
  case GTK_HDY_FLAP_TRANSITION_TYPE_OVER:
    return FALSE;

  case GTK_HDY_FLAP_TRANSITION_TYPE_UNDER:
  case GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE:
    return TRUE;

  default:
    g_assert_not_reached ();
  }
}

static gboolean
transition_should_clip (GtkHdyFlap *self)
{
  switch (self->transition_type) {
  case GTK_HDY_FLAP_TRANSITION_TYPE_OVER:
  case GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE:
    return FALSE;

  case GTK_HDY_FLAP_TRANSITION_TYPE_UNDER:
    return TRUE;

  default:
    g_assert_not_reached ();
  }
}

static gdouble
transition_get_content_motion_factor (GtkHdyFlap *self)
{
  switch (self->transition_type) {
  case GTK_HDY_FLAP_TRANSITION_TYPE_OVER:
    return 0;

  case GTK_HDY_FLAP_TRANSITION_TYPE_UNDER:
  case GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE:
    return 1;

  default:
    g_assert_not_reached ();
  }
}

static gdouble
transition_get_flap_motion_factor (GtkHdyFlap *self)
{
  switch (self->transition_type) {
  case GTK_HDY_FLAP_TRANSITION_TYPE_OVER:
  case GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE:
    return 1;

  case GTK_HDY_FLAP_TRANSITION_TYPE_UNDER:
    return 0;

  default:
    g_assert_not_reached ();
  }
}

static void
restack_windows (GtkHdyFlap *self)
{
  gboolean content_above_flap = transition_is_content_above_flap (self);

  if (!content_above_flap) {
    if (self->content.window)
      gdk_window_raise (self->content.window);

    if (self->separator.window)
      gdk_window_raise (self->separator.window);
  }

  if (self->flap.window)
    gdk_window_raise (self->flap.window);

  if (content_above_flap) {
    if (self->separator.window)
      gdk_window_raise (self->separator.window);

    if (self->content.window)
      gdk_window_raise (self->content.window);
  }
}

static void
add_child (GtkHdyFlap *self,
           ChildInfo  *info)
{
  if (gtk_widget_get_realized (GTK_WIDGET (self))) {
    register_window (self, info);
    restack_windows (self);
  }

  gtk_widget_set_parent (info->widget, GTK_WIDGET (self));
}

static void
remove_child (GtkHdyFlap *self,
              ChildInfo  *info)
{
  if (gtk_widget_get_realized (GTK_WIDGET (self)))
    unregister_window (self, info);

  gtk_widget_unparent (info->widget);
}

static inline void
get_preferred_size (GtkWidget      *widget,
                    GtkOrientation  orientation,
                    gint           *min,
                    gint           *nat)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_get_preferred_width (widget, min, nat);
  else
    gtk_widget_get_preferred_height (widget, min, nat);
}

static void
compute_sizes (GtkHdyFlap    *self,
               GtkAllocation *alloc,
               gboolean       folded,
               gboolean       revealed,
               gint          *flap_size,
               gint          *content_size,
               gint          *separator_size)
{
  gboolean flap_expand, content_expand;
  gint total, extra;
  gint flap_nat, content_nat;

  if (!self->flap.widget && !self->content.widget)
    return;

  if (self->separator.widget)
    get_preferred_size (self->separator.widget, self->orientation, separator_size, NULL);
  else
    *separator_size = 0;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    total = alloc->width;
  else
    total = alloc->height;

  if (!self->flap.widget) {
    *content_size = total;
    *flap_size = 0;

    return;
  }

  if (!self->content.widget) {
    *content_size = 0;
    *flap_size = total;

    return;
  }

  get_preferred_size (self->flap.widget, self->orientation, flap_size, &flap_nat);
  get_preferred_size (self->content.widget, self->orientation, content_size, &content_nat);

  flap_expand = gtk_widget_compute_expand (self->flap.widget, self->orientation);
  content_expand = gtk_widget_compute_expand (self->content.widget, self->orientation);

  if (folded) {
    *content_size = total;

    if (flap_expand) {
      *flap_size = total;
    } else {
      get_preferred_size (self->flap.widget, self->orientation, NULL, flap_size);
      *flap_size = MIN (*flap_size, total);
    }

    return;
  }

  if (revealed)
    total -= *separator_size;

  if (flap_expand && content_expand) {
    *flap_size = MAX (total / 2, *flap_size);

    if (!revealed)
      *content_size = total;
    else
      *content_size = total - *flap_size;

    return;
  }

  extra = total - *content_size - *flap_size;

  if (flap_expand) {
    *flap_size += extra;
    extra = 0;

    if (!revealed)
      *content_size = total;

    return;
  }

  if (content_expand) {
    *content_size += extra;
    extra = 0;
  }

  if (extra > 0) {
    GtkRequestedSize sizes[2];

    sizes[0].data = self->flap.widget;
    sizes[0].minimum_size = *flap_size;
    sizes[0].natural_size = flap_nat;

    sizes[1].data = self->content.widget;
    sizes[1].minimum_size = *content_size;
    sizes[1].natural_size = content_nat;

    extra = gtk_distribute_natural_allocation (extra, 2, sizes);

    *flap_size = sizes[0].minimum_size;
    *content_size = sizes[1].minimum_size + extra;
  }

  if (!revealed)
    *content_size = total;
}

static inline void
interpolate_reveal (GtkHdyFlap    *self,
                    GtkAllocation *alloc,
                    gboolean       folded,
                    gint          *flap_size,
                    gint          *content_size,
                    gint          *separator_size)
{
  if (self->reveal_progress <= 0) {
    compute_sizes (self, alloc, folded, FALSE, flap_size, content_size, separator_size);
  } else if (self->reveal_progress >= 1) {
    compute_sizes (self, alloc, folded, TRUE, flap_size, content_size, separator_size);
  } else {
    gint flap_revealed, content_revealed, separator_revealed;
    gint flap_hidden, content_hidden, separator_hidden;

    compute_sizes (self, alloc, folded, TRUE, &flap_revealed, &content_revealed, &separator_revealed);
    compute_sizes (self, alloc, folded, FALSE, &flap_hidden, &content_hidden, &separator_hidden);

    *flap_size =
      (gint) round (gtk_hdy_lerp (flap_hidden, flap_revealed,
                                  self->reveal_progress));
    *content_size =
      (gint) round (gtk_hdy_lerp (content_hidden, content_revealed,
                                  self->reveal_progress));
    *separator_size =
      (gint) round (gtk_hdy_lerp (separator_hidden, separator_revealed,
                                  self->reveal_progress));
  }
}

static inline void
interpolate_fold (GtkHdyFlap    *self,
                  GtkAllocation *alloc,
                  gint          *flap_size,
                  gint          *content_size,
                  gint          *separator_size)
{
  if (self->fold_progress <= 0) {
    interpolate_reveal (self, alloc, FALSE, flap_size, content_size, separator_size);
  } else if (self->fold_progress >= 1) {
    interpolate_reveal (self, alloc, TRUE, flap_size, content_size, separator_size);
  } else {
    gint flap_folded, content_folded, separator_folded;
    gint flap_unfolded, content_unfolded, separator_unfolded;

    interpolate_reveal (self, alloc, TRUE, &flap_folded, &content_folded, &separator_folded);
    interpolate_reveal (self, alloc, FALSE, &flap_unfolded, &content_unfolded, &separator_unfolded);

    *flap_size =
      (gint) round (gtk_hdy_lerp (flap_unfolded, flap_folded,
                                  self->fold_progress));
    *content_size =
      (gint) round (gtk_hdy_lerp (content_unfolded, content_folded,
                                  self->fold_progress));
    *separator_size =
      (gint) round (gtk_hdy_lerp (separator_unfolded, separator_folded,
                                  self->fold_progress));
  }
}

static void
compute_allocation (GtkHdyFlap    *self,
                    GtkAllocation *alloc,
                    GtkAllocation *flap_alloc,
                    GtkAllocation *content_alloc,
                    GtkAllocation *separator_alloc)
{
  gdouble distance;
  gint content_size, flap_size, separator_size;
  gint total, content_pos, flap_pos, separator_pos;
  gboolean content_above_flap = transition_is_content_above_flap (self);

  if (!self->flap.widget && !self->content.widget && !self->separator.widget)
    return;

  content_alloc->x = 0;
  content_alloc->y = 0;
  flap_alloc->x = 0;
  flap_alloc->y = 0;
  separator_alloc->x = 0;
  separator_alloc->y = 0;

  interpolate_fold (self, alloc, &flap_size, &content_size, &separator_size);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    flap_alloc->width = flap_size;
    content_alloc->width = content_size;
    separator_alloc->width = separator_size;
    flap_alloc->height = content_alloc->height = separator_alloc->height = alloc->height;
    total = alloc->width;
  } else {
    flap_alloc->height = flap_size;
    content_alloc->height = content_size;
    separator_alloc->height = separator_size;
    flap_alloc->width = content_alloc->width = separator_alloc->width = alloc->width;
    total = alloc->height;
  }

  if (!self->flap.widget)
    return;

  if (content_above_flap)
    distance = flap_size + separator_size;
  else
    distance = flap_size + separator_size * (1 - self->fold_progress);

  flap_pos = -(gint) round ((1 - self->reveal_progress) * transition_get_flap_motion_factor (self) * distance);

  if (content_above_flap) {
    content_pos = (gint) round (self->reveal_progress * transition_get_content_motion_factor (self) * distance);
    separator_pos = flap_pos + flap_size;
  } else {
    content_pos = total - content_size + (gint) round (self->reveal_progress * self->fold_progress * transition_get_content_motion_factor (self) * distance);
    separator_pos = content_pos - separator_size;
  }

  if (self->flap_position != get_start_or_end (self)) {
    flap_pos = total - flap_pos - flap_size;
    separator_pos = total - separator_pos - separator_size;
    content_pos = total - content_pos - content_size;
  }

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    content_alloc->x = content_pos;
    flap_alloc->x = flap_pos;
    separator_alloc->x = separator_pos;
  } else {
    content_alloc->y = content_pos;
    flap_alloc->y = flap_pos;
    separator_alloc->y = separator_pos;
  }
}

static inline void
allocate_child (GtkHdyFlap *self,
                ChildInfo  *info,
                gboolean    expand_window)
{
  GtkAllocation child_alloc;

  if (!info->widget)
    return;

  if (gtk_widget_get_realized (GTK_WIDGET (self))) {
    if (expand_window)
      gdk_window_move_resize (info->window,
                              0, 0,
                              gtk_widget_get_allocated_width (GTK_WIDGET (self)),
                              gtk_widget_get_allocated_height (GTK_WIDGET (self)));
    else
      gdk_window_move_resize (info->window,
                              info->allocation.x,
                              info->allocation.y,
                              info->allocation.width,
                              info->allocation.height);
  }

  child_alloc.x = expand_window ? info->allocation.x : 0;
  child_alloc.y = expand_window ? info->allocation.y : 0;
  child_alloc.width = info->allocation.width;
  child_alloc.height = info->allocation.height;

  gtk_widget_size_allocate (info->widget, &child_alloc);
}

static void
gtk_hdy_flap_size_allocate (GtkWidget     *widget,
                            GtkAllocation *alloc)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);

  gtk_widget_set_allocation (widget, alloc);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (gtk_widget_get_window (widget),
                            alloc->x, alloc->y, alloc->width, alloc->height);

  if (self->fold_policy == GTK_HDY_FLAP_FOLD_POLICY_AUTO) {
    GtkRequisition flap_size = { 0, 0 };
    GtkRequisition content_size = { 0, 0 };
    GtkRequisition separator_size = { 0, 0 };
    gboolean flap_expand, content_expand;

    flap_expand = gtk_widget_compute_expand (self->flap.widget, self->orientation);
    content_expand = gtk_widget_compute_expand (self->content.widget, self->orientation);

    if (self->flap.widget)
      gtk_widget_get_preferred_size (self->flap.widget,
                                     content_expand ? &flap_size : NULL,
                                     content_expand ? NULL : &flap_size);
    if (self->content.widget)
      gtk_widget_get_preferred_size (self->content.widget,
                                     flap_expand ? &content_size : NULL,
                                     flap_expand ? NULL : &content_size);
    if (self->separator.widget)
      gtk_widget_get_preferred_size (self->separator.widget, &separator_size, NULL);

    if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
      set_folded (self, alloc->width < content_size.width + flap_size.width + separator_size.width);
    else
      set_folded (self, alloc->height < content_size.height + flap_size.height + separator_size.height);
  }

  compute_allocation (self,
                      alloc,
                      &self->flap.allocation,
                      &self->content.allocation,
                      &self->separator.allocation);

  allocate_child (self, &self->content, FALSE);
  allocate_child (self, &self->separator, FALSE);
  allocate_child (self, &self->flap,
                  self->modal &&
                  self->reveal_progress > 0 &&
                  self->fold_progress > 0);

  gtk_widget_set_clip (widget, alloc);
}

/* This private method is prefixed by the call name because it will be a virtual
 * method in GTK 4.
 */
static void
gtk_hdy_flap_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      gint            for_size,
                      gint           *minimum,
                      gint           *natural,
                      gint           *minimum_baseline,
                      gint           *natural_baseline)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);

  gint content_min = 0, content_nat = 0;
  gint flap_min = 0, flap_nat = 0;
  gint separator_min = 0, separator_nat = 0;
  gint min, nat;

  if (self->content.widget)
    get_preferred_size (self->content.widget, orientation, &content_min, &content_nat);

  if (self->flap.widget)
    get_preferred_size (self->flap.widget, orientation, &flap_min, &flap_nat);

  if (self->separator.widget)
    get_preferred_size (self->separator.widget, orientation, &separator_min, &separator_nat);

  if (self->orientation == orientation) {
    gdouble min_progress, nat_progress;

    switch (self->fold_policy) {
    case GTK_HDY_FLAP_FOLD_POLICY_NEVER:
      min_progress = (1 - self->fold_progress) * self->reveal_progress;
      nat_progress = 1;
      break;

    case GTK_HDY_FLAP_FOLD_POLICY_ALWAYS:
      min_progress = 0;
      nat_progress = 0;
      break;

    case GTK_HDY_FLAP_FOLD_POLICY_AUTO:
      min_progress = 0;
      nat_progress = self->locked ? self->reveal_progress : 1;
      break;

    default:
      g_assert_not_reached ();
    }

    min = MAX (content_min + (gint) round ((flap_min + separator_min) * min_progress), flap_min);
    nat = MAX (content_nat + (gint) round ((flap_nat + separator_min) * nat_progress), flap_nat);
  } else {
    min = MAX (MAX (content_min, flap_min), separator_min);
    nat = MAX (MAX (content_nat, flap_nat), separator_nat);
  }

  if (minimum)
    *minimum = min;
  if (natural)
    *natural = nat;
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
gtk_hdy_flap_get_preferred_width_for_height (GtkWidget *widget,
                                             gint       height,
                                             gint      *minimum,
                                             gint      *natural)
{
  gtk_hdy_flap_measure (widget, GTK_ORIENTATION_HORIZONTAL, height,
                        minimum, natural, NULL, NULL);
}

static void
gtk_hdy_flap_get_preferred_width (GtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  gtk_hdy_flap_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                        minimum, natural, NULL, NULL);
}


static void
gtk_hdy_flap_get_preferred_height_for_width (GtkWidget *widget,
                                             gint       width,
                                             gint      *minimum,
                                             gint      *natural)
{
  gtk_hdy_flap_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                        minimum, natural, NULL, NULL);
}

static void
gtk_hdy_flap_get_preferred_height (GtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  gtk_hdy_flap_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                        minimum, natural, NULL, NULL);
}

static gboolean
gtk_hdy_flap_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);
  gint width, height;
  gint shadow_x = 0, shadow_y = 0;
  gdouble shadow_progress;
  GtkPanDirection shadow_direction;
  gboolean content_above_flap = transition_is_content_above_flap (self);
  GtkAllocation *shadow_alloc;
  gboolean should_clip;

  shadow_alloc = content_above_flap ? &self->content.allocation : &self->flap.allocation;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  if (self->orientation == GTK_ORIENTATION_VERTICAL) {
    if ((self->flap_position == GTK_PACK_START) != content_above_flap) {
      shadow_direction = GTK_PAN_DIRECTION_UP;
      shadow_y = shadow_alloc->y + shadow_alloc->height;
    } else {
      shadow_direction = GTK_PAN_DIRECTION_DOWN;
      shadow_y = shadow_alloc->y - height;
    }
  } else {
    if ((self->flap_position == get_start_or_end (self)) != content_above_flap) {
      shadow_direction = GTK_PAN_DIRECTION_LEFT;
      shadow_x = shadow_alloc->x + shadow_alloc->width;
    } else {
      shadow_direction = GTK_PAN_DIRECTION_RIGHT;
      shadow_x = shadow_alloc->x - width;
    }
  }

  switch (self->transition_type) {
  case GTK_HDY_FLAP_TRANSITION_TYPE_OVER:
    shadow_progress = 1 - MIN (self->reveal_progress, self->fold_progress);
    break;

  case GTK_HDY_FLAP_TRANSITION_TYPE_UNDER:
    shadow_progress = self->reveal_progress;
    break;

  case GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE:
    shadow_progress = 1;
    break;

  default:
    g_assert_not_reached ();
  }

  should_clip = transition_should_clip (self) &&
                shadow_progress < 1 &&
                self->reveal_progress > 0;

  if (should_clip) {
    cairo_save (cr);
    cairo_rectangle (cr, shadow_x, shadow_y, width, height);
    cairo_clip (cr);
  }

  if (!content_above_flap) {
    if (self->content.widget)
      gtk_container_propagate_draw (GTK_CONTAINER (self),
                                    self->content.widget,
                                    cr);

    if (self->separator.widget)
      gtk_container_propagate_draw (GTK_CONTAINER (self),
                                    self->separator.widget,
                                    cr);

    if (should_clip)
      cairo_restore (cr);
  }

  if (self->flap.widget)
    gtk_container_propagate_draw (GTK_CONTAINER (self),
                                  self->flap.widget,
                                  cr);

  if (content_above_flap) {
    if (self->separator.widget)
      gtk_container_propagate_draw (GTK_CONTAINER (self),
                                    self->separator.widget,
                                    cr);

    if (should_clip)
      cairo_restore (cr);

    if (self->content.widget)
      gtk_container_propagate_draw (GTK_CONTAINER (self),
                                    self->content.widget,
                                    cr);
  }

  if (!self->flap.widget)
    return GDK_EVENT_PROPAGATE;

  if (shadow_progress < 1 && gtk_widget_get_mapped (self->flap.widget)) {
    cairo_save (cr);
    cairo_translate (cr, shadow_x, shadow_y);
    gtk_hdy_shadow_helper_draw_shadow (self->shadow_helper, cr, width, height,
                                       shadow_progress, shadow_direction);
    cairo_restore (cr);
  }

  return GDK_EVENT_PROPAGATE;
}

static void
gtk_hdy_flap_realize (GtkWidget *widget)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);
  GtkAllocation allocation;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GdkWindow *window;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_widget_set_realized (widget, TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gdk_window_new (gtk_widget_get_parent_window (widget),
                           &attributes,
                           attributes_mask);
  gtk_widget_set_window (widget, window);
  gtk_widget_register_window (widget, window);

  register_window (self, &self->content);
  register_window (self, &self->separator);
  register_window (self, &self->flap);

  restack_windows (self);
}

static void
gtk_hdy_flap_unrealize (GtkWidget *widget)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);

  unregister_window (self, &self->content);
  unregister_window (self, &self->separator);
  unregister_window (self, &self->flap);

  GTK_WIDGET_CLASS (gtk_hdy_flap_parent_class)->unrealize (widget);
}

static void
gtk_hdy_flap_direction_changed (GtkWidget        *widget,
                                GtkTextDirection  previous_direction)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (widget);

  update_swipe_tracker (self);

  GTK_WIDGET_CLASS (gtk_hdy_flap_parent_class)->direction_changed (widget,
                                                                   previous_direction);
}

static void
gtk_hdy_flap_forall (GtkContainer *container,
                     gboolean      include_internals,
                     GtkCallback   callback,
                     gpointer      callback_data)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (container);

  if (self->content.widget)
    callback (self->content.widget, callback_data);

  if (self->separator.widget)
    callback (self->separator.widget, callback_data);

  if (self->flap.widget)
    callback (self->flap.widget, callback_data);
}

static void
gtk_hdy_flap_add (GtkContainer *container,
                  GtkWidget    *widget)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (container);

  if (self->content.widget) {
    g_warning ("Attempting to add a widget with type %s to a %s, "
               "but %s can only contain one widget at a time; "
               "it already contains a widget of type %s",
               g_type_name (G_OBJECT_TYPE (widget)),
               g_type_name (G_OBJECT_TYPE (self)),
               g_type_name (G_OBJECT_TYPE (self)),
               g_type_name (G_OBJECT_TYPE (self->content.widget)));

    return;
  }

  gtk_hdy_flap_set_content (self, widget);
}

static void
gtk_hdy_flap_remove (GtkContainer *container,
                     GtkWidget    *widget)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (container);

  if (widget == self->flap.widget)
    gtk_hdy_flap_set_flap (self, NULL);
  else if (widget == self->separator.widget)
    gtk_hdy_flap_set_separator (self, NULL);
  else if (widget == self->content.widget)
    gtk_hdy_flap_set_content (self, NULL);
  else
    g_return_if_reached ();
}

static void
gtk_hdy_flap_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (object);

  switch (prop_id) {
  case PROP_CONTENT:
    g_value_set_object (value, gtk_hdy_flap_get_content (self));
    break;
  case PROP_FLAP:
    g_value_set_object (value, gtk_hdy_flap_get_flap (self));
    break;
  case PROP_SEPARATOR:
    g_value_set_object (value, gtk_hdy_flap_get_separator (self));
    break;
  case PROP_FLAP_POSITION:
    g_value_set_enum (value, gtk_hdy_flap_get_flap_position (self));
    break;
  case PROP_REVEAL_FLAP:
    g_value_set_boolean (value, gtk_hdy_flap_get_reveal_flap (self));
    break;
  case PROP_REVEAL_DURATION:
    g_value_set_uint (value, gtk_hdy_flap_get_reveal_duration (self));
    break;
  case PROP_REVEAL_PROGRESS:
    g_value_set_double (value, gtk_hdy_flap_get_reveal_progress (self));
    break;
  case PROP_FOLD_POLICY:
    g_value_set_enum (value, gtk_hdy_flap_get_fold_policy (self));
    break;
  case PROP_FOLD_DURATION:
    g_value_set_uint (value, gtk_hdy_flap_get_fold_duration (self));
    break;
  case PROP_FOLDED:
    g_value_set_boolean (value, gtk_hdy_flap_get_folded (self));
    break;
  case PROP_LOCKED:
    g_value_set_boolean (value, gtk_hdy_flap_get_locked (self));
    break;
  case PROP_TRANSITION_TYPE:
    g_value_set_enum (value, gtk_hdy_flap_get_transition_type (self));
    break;
  case PROP_MODAL:
    g_value_set_boolean (value, gtk_hdy_flap_get_modal (self));
    break;
  case PROP_SWIPE_TO_OPEN:
    g_value_set_boolean (value, gtk_hdy_flap_get_swipe_to_open (self));
    break;
  case PROP_SWIPE_TO_CLOSE:
    g_value_set_boolean (value, gtk_hdy_flap_get_swipe_to_close (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gtk_hdy_flap_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (object);

  switch (prop_id) {
  case PROP_CONTENT:
    gtk_hdy_flap_set_content (self, g_value_get_object (value));
    break;
  case PROP_FLAP:
    gtk_hdy_flap_set_flap (self, g_value_get_object (value));
    break;
  case PROP_SEPARATOR:
    gtk_hdy_flap_set_separator (self, g_value_get_object (value));
    break;
  case PROP_FLAP_POSITION:
    gtk_hdy_flap_set_flap_position (self, g_value_get_enum (value));
    break;
  case PROP_REVEAL_FLAP:
    gtk_hdy_flap_set_reveal_flap (self, g_value_get_boolean (value));
    break;
  case PROP_REVEAL_DURATION:
    gtk_hdy_flap_set_reveal_duration (self, g_value_get_uint (value));
    break;
  case PROP_FOLD_POLICY:
    gtk_hdy_flap_set_fold_policy (self, g_value_get_enum (value));
    break;
  case PROP_FOLD_DURATION:
    gtk_hdy_flap_set_fold_duration (self, g_value_get_uint (value));
    break;
  case PROP_LOCKED:
    gtk_hdy_flap_set_locked (self, g_value_get_boolean (value));
    break;
  case PROP_TRANSITION_TYPE:
    gtk_hdy_flap_set_transition_type (self, g_value_get_enum (value));
    break;
  case PROP_MODAL:
    gtk_hdy_flap_set_modal (self, g_value_get_boolean (value));
    break;
  case PROP_SWIPE_TO_OPEN:
    gtk_hdy_flap_set_swipe_to_open (self, g_value_get_boolean (value));
    break;
  case PROP_SWIPE_TO_CLOSE:
    gtk_hdy_flap_set_swipe_to_close (self, g_value_get_boolean (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gtk_hdy_flap_dispose (GObject *object)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (object);

  g_clear_object (&self->shadow_helper);
  g_clear_object (&self->tracker);
  g_clear_object (&self->click_gesture);
  g_clear_object (&self->key_controller);

  G_OBJECT_CLASS (gtk_hdy_flap_parent_class)->dispose (object);
}

static void
gtk_hdy_flap_class_init (GtkHdyFlapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = gtk_hdy_flap_get_property;
  object_class->set_property = gtk_hdy_flap_set_property;
  object_class->dispose = gtk_hdy_flap_dispose;

  widget_class->get_preferred_width = gtk_hdy_flap_get_preferred_width;
  widget_class->get_preferred_width_for_height = gtk_hdy_flap_get_preferred_width_for_height;
  widget_class->get_preferred_height = gtk_hdy_flap_get_preferred_height;
  widget_class->get_preferred_height_for_width = gtk_hdy_flap_get_preferred_height_for_width;
  widget_class->size_allocate = gtk_hdy_flap_size_allocate;
  widget_class->draw = gtk_hdy_flap_draw;
  widget_class->realize = gtk_hdy_flap_realize;
  widget_class->unrealize = gtk_hdy_flap_unrealize;
  widget_class->direction_changed = gtk_hdy_flap_direction_changed;

  container_class->remove = gtk_hdy_flap_remove;
  container_class->add = gtk_hdy_flap_add;
  container_class->forall = gtk_hdy_flap_forall;

  /**
   * GtkHdyFlap:content:
   *
   * The content widget, always displayed when unfolded, and partially visible
   * when folded.
   *
   * Since: 1.1
   */
  props[PROP_CONTENT] =
    g_param_spec_object ("content",
                         _("Content"),
                         _("The content Widget"),
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:flap:
   *
   * The flap widget, only visible when #GtkHdyFlap:reveal-progress is greater than
   * 0.
   *
   * Since: 1.1
   */
  props[PROP_FLAP] =
    g_param_spec_object ("flap",
                         _("Flap"),
                         _("The flap widget"),
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:separator:
   *
   * The separator widget, displayed between content and flap when there's no
   * shadow to display. When exactly it's visible depends on the
   * #GtkHdyFlap:transition-type value. If %NULL, no separator will be used.
   *
   * Since: 1.1
   */
  props[PROP_SEPARATOR] =
    g_param_spec_object ("separator",
                         _("Separator"),
                         _("The separator widget"),
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:flap-position:
   *
   * The flap position for @self. If @GTK_PACK_START, the flap is displayed
   * before the content, if @GTK_PACK_END, it's displayed after the content.
   *
   * Since: 1.1
   */
  props[PROP_FLAP_POSITION] =
    g_param_spec_enum ("flap-position",
                       _("Flap Position"),
                       _("The flap position"),
                       GTK_TYPE_PACK_TYPE,
                       GTK_PACK_START,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:reveal-flap:
   *
   * Whether the flap widget is revealed.
   *
   * Since: 1.1
   */
  props[PROP_REVEAL_FLAP] =
    g_param_spec_boolean ("reveal-flap",
                          _("Reveal Flap"),
                          _("Whether the flap is revealed"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:reveal-duration:
   *
   * The reveal transition animation duration, in milliseconds.
   *
   * Since: 1.1
   */
  props[PROP_REVEAL_DURATION] =
    g_param_spec_uint ("reveal-duration",
                       _("Reveal Duration"),
                       _("The reveal transition animation duration, in milliseconds"),
                       0, G_MAXINT,
                       250,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:reveal-progress:
   *
   * The current reveal transition progress. 0 means fully hidden, 1 means fully
   * revealed See #GtkHdyFlap:reveal-flap.
   *
   * Since: 1.1
   */
  props[PROP_REVEAL_PROGRESS] =
    g_param_spec_double ("reveal-progress",
                          _("Reveal Progress"),
                          _("The current reveal transition progress"),
                          0.0, 1.0, 1.0,
                          G_PARAM_READABLE);

  /**
   * GtkHdyFlap:fold-policy:
   *
   * The current fold policy. See #GtkHdyFlapFoldPolicy for available
   * policies.
   *
   * Since: 1.1
   */
  props[PROP_FOLD_POLICY] =
    g_param_spec_enum ("fold-policy",
                       _("Fold Policy"),
                       _("The current fold policy"),
                       GTK_TYPE_HDY_FLAP_FOLD_POLICY,
                       GTK_HDY_FLAP_FOLD_POLICY_AUTO,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:fold-duration:
   *
   * The fold transition animation duration, in milliseconds.
   *
   * Since: 1.1
   */
  props[PROP_FOLD_DURATION] =
    g_param_spec_uint ("fold-duration",
                       _("Fold Duration"),
                       _("The fold transition animation duration, in milliseconds"),
                       0, G_MAXINT,
                       250,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:folded:
   *
   * Whether the flap is currently folded.
   *
   * See #GtkHdyFlap:fold-policy.
   *
   * Since: 1.1
   */
  props[PROP_FOLDED] =
    g_param_spec_boolean ("folded",
                          _("Folded"),
                          _("Whether the flap is currently folded"),
                          FALSE,
                          G_PARAM_READABLE);

  /**
   * GtkHdyFlap:locked:
   *
   * Whether the flap is locked.
   *
   * If %FALSE, folding when the flap is revealed automatically closes it, and
   * unfolding it when the flap is not revealed opens it. If %TRUE,
   * #GtkHdyFlap:reveal-flap value never changes on its own.
   *
   * Since: 1.1
   */
  props[PROP_LOCKED] =
    g_param_spec_boolean ("locked",
                          _("Locked"),
                          _("Whether the flap is locked"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:transition-type:
   *
   * The type of animation that will be used for reveal and fold transitions
   * in @self.
   *
   * #GtkHdyFlap:flap is transparent by default, which means the content will be
   * seen through it with %HDY_FLAP_TRANSITION_TYPE_OVER transitions; add the
   * .background style class to it if this is unwanted.
   *
   * Since: 1.1
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type",
                       _("Transition Type"),
                       _("The type of animation used for reveal and fold transitions"),
                       GTK_TYPE_HDY_FLAP_TRANSITION_TYPE,
                       GTK_HDY_FLAP_TRANSITION_TYPE_OVER,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:modal:
   *
   * Whether the flap is modal.
   *
   * If %TRUE, clicking the content widget while flap is revealed, as well as
   * pressing Escape key, will close the flap. If %FALSE, clicks are passed
   * through to the content widget.
   *
   * Since: 1.1
   */
  props[PROP_MODAL] =
    g_param_spec_boolean ("modal",
                          _("Modal"),
                          _("Whether the flap is modal"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:swipe-to-open:
   *
   * Whether the flap can be opened with a swipe gesture.
   *
   * The area that can be swiped depends on the #GtkHdyFlap:transition-type value.
   *
   * Since: 1.1
   */
  props[PROP_SWIPE_TO_OPEN] =
    g_param_spec_boolean ("swipe-to-open",
                          _("Swipe to Open"),
                          _("Whether the flap can be opened with a swipe gesture"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkHdyFlap:swipe-to-close:
   *
   * Whether the flap can be closed with a swipe gesture.
   *
   * The area that can be swiped depends on the #GtkHdyFlap:transition-type value.
   *
   * Since: 1.1
   */
  props[PROP_SWIPE_TO_CLOSE] =
    g_param_spec_boolean ("swipe-to-close",
                          _("Swipe to Close"),
                          _("Whether the flap can be closed with a swipe gesture"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  gtk_widget_class_set_css_name (widget_class, "flap");
}

static void
gtk_hdy_flap_init (GtkHdyFlap *self)
{
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (self));

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);

  self->orientation = GTK_ORIENTATION_HORIZONTAL;
  self->flap_position = GTK_PACK_START;
  self->fold_policy = GTK_HDY_FLAP_FOLD_POLICY_AUTO;
  self->transition_type = GTK_HDY_FLAP_TRANSITION_TYPE_OVER;
  self->reveal_flap = TRUE;
  self->locked = FALSE;
  self->reveal_progress = 1;
  self->folded = FALSE;
  self->fold_progress = 0;
  self->fold_duration = 250;
  self->reveal_duration = 250;
  self->modal = TRUE;
  self->swipe_to_open = TRUE;
  self->swipe_to_close = TRUE;

  self->shadow_helper = gtk_hdy_shadow_helper_new (GTK_WIDGET (self));
  self->tracker = gtk_hdy_swipe_tracker_new (GTK_HDY_SWIPEABLE (self));
  gtk_hdy_swipe_tracker_set_enabled (self->tracker, FALSE);

  g_signal_connect_object (self->tracker, "begin-swipe", G_CALLBACK (begin_swipe_cb), self, 0);
  g_signal_connect_object (self->tracker, "update-swipe", G_CALLBACK (update_swipe_cb), self, 0);
  g_signal_connect_object (self->tracker, "end-swipe", G_CALLBACK (end_swipe_cb), self, 0);

  update_swipe_tracker (self);

  self->click_gesture = gtk_gesture_multi_press_new (GTK_WIDGET (self));
  gtk_gesture_single_set_exclusive (GTK_GESTURE_SINGLE (self->click_gesture), TRUE);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->click_gesture), GDK_BUTTON_PRIMARY);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->click_gesture),
                                              GTK_PHASE_CAPTURE);
  g_signal_connect_object (self->click_gesture, "released", G_CALLBACK (released_cb), self, 0);

  self->key_controller = gtk_event_controller_key_new (GTK_WIDGET (self));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->key_controller),
                                              GTK_PHASE_BUBBLE);
  g_signal_connect_object (self->key_controller, "key-pressed", G_CALLBACK (key_pressed_cb), self, 0);

  gtk_style_context_add_class (context, "unfolded");
}

static void
gtk_hdy_flap_add_child (GtkBuildable *buildable,
                        GtkBuilder   *builder,
                        GObject      *child,
                        const gchar  *type)
{
  if (!type || !g_strcmp0 (type, "content"))
    gtk_hdy_flap_set_content (GTK_HDY_FLAP (buildable), GTK_WIDGET (child));
  else if (!g_strcmp0 (type, "flap"))
    gtk_hdy_flap_set_flap (GTK_HDY_FLAP (buildable), GTK_WIDGET (child));
  else if (!g_strcmp0 (type, "separator"))
    gtk_hdy_flap_set_separator (GTK_HDY_FLAP (buildable), GTK_WIDGET (child));
  else
    GTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

static void
gtk_hdy_flap_buildable_init (GtkBuildableIface *iface)
{
  iface->add_child = gtk_hdy_flap_add_child;
}

static void
gtk_hdy_flap_switch_child (GtkHdySwipeable *swipeable,
                           guint            index,
                           gint64           duration)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);

  set_reveal_flap (self, index > 0, duration, FALSE);
}

static GtkHdySwipeTracker *
gtk_hdy_flap_get_swipe_tracker (GtkHdySwipeable *swipeable)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);

  return self->tracker;
}

static gdouble
gtk_hdy_flap_get_distance (GtkHdySwipeable *swipeable)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);
  gint flap, separator;

  if (!self->flap.widget)
    return 0;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    flap = self->flap.allocation.width;
    separator = self->separator.allocation.width;
  } else {
    flap = self->flap.allocation.height;
    separator = self->separator.allocation.height;
  }

  if (transition_is_content_above_flap (self))
    return flap + separator;

  return flap + separator * (1 - self->fold_progress);
}

static gdouble *
gtk_hdy_flap_get_snap_points (GtkHdySwipeable *swipeable,
                              gint            *n_snap_points)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);
  gboolean can_open = self->reveal_progress > 0 || self->swipe_to_open || self->swipe_active;
  gboolean can_close = self->reveal_progress < 1 || self->swipe_to_close || self->swipe_active;
  gdouble *points;

  if (!can_open && !can_close)
    return NULL;

  if (can_open && can_close) {
    points = g_new0 (gdouble, 2);

    if (n_snap_points)
      *n_snap_points = 2;

    points[0] = 0;
    points[1] = 1;

    return points;
  }

  points = g_new0 (gdouble, 1);

  if (n_snap_points)
    *n_snap_points = 1;

  points[0] = can_open ? 1 : 0;

  return points;
}

static gdouble
gtk_hdy_flap_get_progress (GtkHdySwipeable *swipeable)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);

  return self->reveal_progress;
}

static gdouble
gtk_hdy_flap_get_cancel_progress (GtkHdySwipeable *swipeable)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);

  return round (self->reveal_progress);
}

static void
gtk_hdy_flap_get_swipe_area (GtkHdySwipeable           *swipeable,
                             GtkHdyNavigationDirection  navigation_direction,
                             gboolean                   is_drag,
                             GdkRectangle              *rect)
{
  GtkHdyFlap *self = GTK_HDY_FLAP (swipeable);
  GtkAllocation *alloc;
  gint width, height;
  gdouble flap_factor, content_factor;
  gboolean content_above_flap;

  if (!self->flap.widget) {
    rect->x = 0;
    rect->y = 0;
    rect->width = 0;
    rect->height = 0;

    return;
  }

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  content_above_flap = transition_is_content_above_flap (self);
  flap_factor = transition_get_flap_motion_factor (self);
  content_factor = transition_get_content_motion_factor (self);

  if (!is_drag ||
      (flap_factor >= 1 && content_factor >= 1) ||
      (self->fold_progress < 1 && flap_factor > 0)) {
    rect->x = 0;
    rect->y = 0;
    rect->width = width;
    rect->height = height;

    return;
  }

  alloc = content_above_flap
    ? &self->content.allocation
    : &self->flap.allocation;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    if (alloc->x <= 0) {
      rect->x = 0;
      rect->width = MAX (alloc->width + alloc->x, GTK_HDY_SWIPE_BORDER);
    } else if (alloc->x + alloc->width >= width) {
      rect->width = MAX (width - alloc->x, GTK_HDY_SWIPE_BORDER);
      rect->x = width - rect->width;
    } else {
      g_assert_not_reached ();
    }

    rect->y = alloc->y;
    rect->height = alloc->height;
  } else {
    if (alloc->y <= 0) {
      rect->y = 0;
      rect->height = MAX (alloc->height + alloc->y, GTK_HDY_SWIPE_BORDER);
    } else if (alloc->y + alloc->height >= height) {
      rect->height = MAX (height - alloc->y, GTK_HDY_SWIPE_BORDER);
      rect->y = height - rect->height;
    } else {
      g_assert_not_reached ();
    }

    rect->x = alloc->x;
    rect->width = alloc->width;
  }
}

static void
gtk_hdy_flap_swipeable_init (GtkHdySwipeableInterface *iface)
{
  iface->switch_child = gtk_hdy_flap_switch_child;
  iface->get_swipe_tracker = gtk_hdy_flap_get_swipe_tracker;
  iface->get_distance = gtk_hdy_flap_get_distance;
  iface->get_snap_points = gtk_hdy_flap_get_snap_points;
  iface->get_progress = gtk_hdy_flap_get_progress;
  iface->get_cancel_progress = gtk_hdy_flap_get_cancel_progress;
  iface->get_swipe_area = gtk_hdy_flap_get_swipe_area;
}

/**
 * gtk_hdy_flap_new:
 *
 * Creates a new #GtkHdyFlap.
 *
 * Returns: a new #GtkHdyFlap
 *
 * Since: 1.1
 */
GtkWidget *
gtk_hdy_flap_new (void)
{
  return g_object_new (GTK_TYPE_HDY_FLAP, NULL);
}

/**
 * gtk_hdy_flap_get_content:
 * @self: a #GtkHdyFlap
 *
 * Gets the content widget for @self
 *
 * Returns: (transfer none) (nullable): the content widget for @self
 *
 * Since: 1.1
 */
GtkWidget *
gtk_hdy_flap_get_content (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), NULL);

  return self->content.widget;
}

/**
 * gtk_hdy_flap_set_content:
 * @self: a #GtkHdyFlap
 * @content: (nullable): the content widget, or %NULL
 *
 * Sets the content widget for @self, always displayed when unfolded, and
 * partially visible when folded.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_content (GtkHdyFlap *self,
                          GtkWidget  *content)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (GTK_IS_WIDGET (content) || content == NULL);

  if (self->content.widget == content)
    return;

  if (self->content.widget)
    remove_child (self, &self->content);

  self->content.widget = content;

  if (self->content.widget)
    add_child (self, &self->content);

  update_child_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CONTENT]);
}

/**
 * gtk_hdy_flap_get_flap:
 * @self: a #GtkHdyFlap
 *
 * Gets the flap widget for @self
 *
 * Returns: (transfer none) (nullable): the flap widget for @self
 *
 * Since: 1.1
 */
GtkWidget *
gtk_hdy_flap_get_flap (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), NULL);

  return self->flap.widget;
}

/**
 * gtk_hdy_flap_set_flap:
 * @self: a #GtkHdyFlap
 * @flap: (nullable): the flap widget, or %NULL
 *
 * Sets the flap widget for @self, only visible when #GtkHdyFlap:reveal-progress is
 * greater than 0.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_flap (GtkHdyFlap *self,
                       GtkWidget  *flap)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (GTK_IS_WIDGET (flap) || flap == NULL);

  if (self->flap.widget == flap)
    return;

  if (self->flap.widget)
    remove_child (self, &self->flap);

  self->flap.widget = flap;

  if (self->flap.widget)
    add_child (self, &self->flap);

  update_swipe_tracker (self);
  update_child_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FLAP]);
}

/**
 * gtk_hdy_flap_get_separator:
 * @self: a #GtkHdyFlap
 *
 * Gets the separator widget for @self.
 *
 * Returns: (transfer none) (nullable): the separator widget for @self
 *
 * Since: 1.1
 */
GtkWidget *
gtk_hdy_flap_get_separator (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), NULL);

  return self->separator.widget;
}

/**
 * gtk_hdy_flap_set_separator:
 * @self: a #GtkHdyFlap
 * @separator: (nullable): the separator widget, or %NULL
 *
 * Sets the separator widget for @self, displayed between content and flap when
 * there's no shadow to display. When exactly it's visible depends on the
 * #GtkHdyFlap:transition-type value. If %NULL, no separator will be used.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_separator (GtkHdyFlap *self,
                            GtkWidget  *separator)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (GTK_IS_WIDGET (separator) || separator == NULL);

  if (self->separator.widget == separator)
    return;

  if (self->separator.widget)
    remove_child (self, &self->separator);

  self->separator.widget = separator;

  if (self->separator.widget)
    add_child (self, &self->separator);

  update_child_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SEPARATOR]);
}

/**
 * gtk_hdy_flap_get_flap_position:
 * @self: a #GtkHdyFlap
 *
 * Gets the flap position for @self.
 *
 * Returns: the flap position for @self
 *
 * Since: 1.1
 */
GtkPackType
gtk_hdy_flap_get_flap_position (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), GTK_PACK_START);

  return self->flap_position;
}

/**
 * gtk_hdy_flap_set_flap_position:
 * @self: a #GtkHdyFlap
 * @position: the new value
 *
 * Sets the flap position for @self. If @GTK_PACK_START, the flap is displayed
 * before the content, if @GTK_PACK_END, it's displayed after the content.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_flap_position (GtkHdyFlap  *self,
                                GtkPackType  position)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (position <= GTK_PACK_END);

  if (self->flap_position == position)
    return;

  self->flap_position = position;

  gtk_widget_queue_allocate (GTK_WIDGET (self));
  gtk_hdy_shadow_helper_clear_cache (self->shadow_helper);
  update_swipe_tracker (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FLAP_POSITION]);
}

/**
 * gtk_hdy_flap_get_reveal_flap:
 * @self: a #GtkHdyFlap
 *
 * Gets whether the flap widget is revealed for @self.
 *
 * Returns: %TRUE if the flap widget is revealed, %FALSE otherwise.
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_reveal_flap (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->reveal_flap;
}

/**
 * gtk_hdy_flap_set_reveal_flap:
 * @self: a #GtkHdyFlap
 * @reveal_flap: %TRUE to reveal the flap widget, %FALSE otherwise
 *
 * Sets whether the flap widget is revealed for @self.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_reveal_flap (GtkHdyFlap *self,
                              gboolean    reveal_flap)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  set_reveal_flap (self, reveal_flap, self->reveal_duration, TRUE);
}

/**
 * gtk_hdy_flap_get_reveal_duration:
 * @self: a #GtkHdyFlap
 *
 * Returns the amount of time (in milliseconds) that reveal transitions in @self
 * will take.
 *
 * Returns: the reveal transition duration
 *
 * Since: 1.1
 */
guint
gtk_hdy_flap_get_reveal_duration (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), 0);

  return self->reveal_duration;
}

/**
 * gtk_hdy_flap_set_reveal_duration:
 * @self: a #GtkHdyFlap
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that reveal transitions in @self will take.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_reveal_duration (GtkHdyFlap *self,
                                  guint       duration)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  if (self->reveal_duration == duration)
    return;

  self->reveal_duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REVEAL_DURATION]);
}

/**
 * gtk_hdy_flap_get_reveal_progress:
 * @self: a #GtkHdyFlap
 *
 * Gets the current reveal transition progress for @self. 0 means fully hidden,
 * 1 means fully revealed. See #GtkHdyFlap:reveal-flap.
 *
 * Returns: the current reveal progress for @self
 *
 * Since: 1.1
 */
gdouble
gtk_hdy_flap_get_reveal_progress (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), 0.0);

  return self->reveal_progress;
}

/**
 * gtk_hdy_flap_get_fold_policy:
 * @self: a #GtkHdyFlap
 *
 * Gets the current fold policy of @self. See gtk_hdy_flap_set_fold_policy().
 *
 * Returns: the current fold policy of @self
 *
 * Since: 1.1
 */
GtkHdyFlapFoldPolicy
gtk_hdy_flap_get_fold_policy (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), GTK_HDY_FLAP_FOLD_POLICY_NEVER);

  return self->fold_policy;
}

/**
 * gtk_hdy_flap_set_fold_policy:
 * @self: a #GtkHdyFlap
 * @policy: Fold policy
 *
 * Sets the current fold policy for @self. See #GtkHdyFlapFoldPolicy for available
 * policies.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_fold_policy (GtkHdyFlap           *self,
                              GtkHdyFlapFoldPolicy  policy)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (policy <= GTK_HDY_FLAP_FOLD_POLICY_AUTO);

  if (self->fold_policy == policy)
    return;

  self->fold_policy = policy;

  switch (self->fold_policy) {
  case GTK_HDY_FLAP_FOLD_POLICY_NEVER:
    set_folded (self, FALSE);
    break;

  case GTK_HDY_FLAP_FOLD_POLICY_ALWAYS:
    set_folded (self, TRUE);
    break;

  case GTK_HDY_FLAP_FOLD_POLICY_AUTO:
    gtk_widget_queue_allocate (GTK_WIDGET (self));
    break;

  default:
    g_assert_not_reached ();
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOLD_POLICY]);
}

/**
 * gtk_hdy_flap_get_fold_duration:
 * @self: a #GtkHdyFlap
 *
 * Returns the amount of time (in milliseconds) that fold transitions in @self
 * will take.
 *
 * Returns: the fold transition duration
 *
 * Since: 1.1
 */
guint
gtk_hdy_flap_get_fold_duration (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), 0);

  return self->fold_duration;
}

/**
 * gtk_hdy_flap_set_fold_duration:
 * @self: a #GtkHdyFlap
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that fold transitions in @self will take.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_fold_duration (GtkHdyFlap *self,
                                guint       duration)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  if (self->fold_duration == duration)
    return;

  self->fold_duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOLD_DURATION]);
}

/**
 * gtk_hdy_flap_get_folded:
 * @self: a #GtkHdyFlap
 *
 * Gets whether @self is currently folded.
 *
 * See #GtkHdyFlap:fold-policy.
 *
 * Returns: %TRUE if @self is currently folded, %FALSE otherwise
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_folded (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->folded;
}

/**
 * gtk_hdy_flap_get_locked:
 * @self: a #GtkHdyFlap
 *
 * Gets whether @self is locked.
 *
 * Returns: %TRUE if @self is locked, %FALSE otherwise
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_locked (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->locked;
}

/**
 * gtk_hdy_flap_set_locked:
 * @self: a #GtkHdyFlap
 * @locked: the new value
 *
 * Sets whether @self is locked.
 *
 * If %FALSE, folding @self when the flap is revealed automatically closes it,
 * and unfolding it when the flap is not revealed opens it. If %TRUE,
 * #GtkHdyFlap:reveal-flap value never changes on its own.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_locked (GtkHdyFlap *self,
                         gboolean    locked)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  locked = !!locked;

  if (self->locked == locked)
    return;

  self->locked = locked;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LOCKED]);
}

/**
 * gtk_hdy_flap_get_transition_type:
 * @self: a #GtkHdyFlap
 *
 * Gets the type of animation that will be used for reveal and fold transitions
 * in @self.
 *
 * Returns: the current transition type of @self
 *
 * Since: 1.1
 */
GtkHdyFlapTransitionType
gtk_hdy_flap_get_transition_type (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), GTK_HDY_FLAP_TRANSITION_TYPE_OVER);

  return self->transition_type;
}

/**
 * gtk_hdy_flap_set_transition_type:
 * @self: a #GtkHdyFlap
 * @transition_type: the new transition type
 *
 * Sets the type of animation that will be used for reveal and fold transitions
 * in @self.
 *
 * #GtkHdyFlap:flap is transparent by default, which means the content will be seen
 * through it with %HDY_FLAP_TRANSITION_TYPE_OVER transitions; add the
 * .background style class to it if this is unwanted.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_transition_type (GtkHdyFlap               *self,
                                  GtkHdyFlapTransitionType  transition_type)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));
  g_return_if_fail (transition_type <= GTK_HDY_FLAP_TRANSITION_TYPE_SLIDE);

  if (self->transition_type == transition_type)
    return;

  self->transition_type = transition_type;

  restack_windows (self);

  if (self->reveal_progress > 0 || (self->fold_progress > 0 && self->fold_progress < 1))
    gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_TYPE]);
}

/**
 * gtk_hdy_flap_get_modal:
 * @self: a #GtkHdyFlap
 *
 * Gets whether the @self is modal. See gtk_hdy_flap_set_modal().
 *
 * Returns: %TRUE if @self is modal
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_modal (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->modal;
}

/**
 * gtk_hdy_flap_set_modal:
 * @self: a #GtkHdyFlap
 * @modal: Whether @self can be closed with a click
 *
 * Sets whether the @self can be closed with a click.
 *
 * If @modal is %TRUE, clicking the content widget while flap is revealed, or
 * pressing Escape key, will close the flap. If %FALSE, clicks are passed
 * through to the content widget.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_modal (GtkHdyFlap *self,
                        gboolean    modal)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  modal = !!modal;

  if (self->modal == modal)
    return;

  self->modal = modal;

  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->click_gesture),
                                              modal ? GTK_PHASE_CAPTURE : GTK_PHASE_NONE);
  gtk_event_controller_set_propagation_phase (self->key_controller,
                                              modal ? GTK_PHASE_BUBBLE : GTK_PHASE_NONE);

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MODAL]);
}

/**
 * gtk_hdy_flap_get_swipe_to_open:
 * @self: a #GtkHdyFlap
 *
 * Gets whether @self can be opened with a swipe gesture.
 *
 * Returns: %TRUE if @self can be opened with a swipe gesture
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_swipe_to_open (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->swipe_to_open;
}

/**
 * gtk_hdy_flap_set_swipe_to_open:
 * @self: a #GtkHdyFlap
 * @swipe_to_open: Whether @self can be opened with a swipe gesture
 *
 * Sets whether @self can be opened with a swipe gesture.
 *
 * The area that can be swiped depends on the #GtkHdyFlap:transition-type value.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_swipe_to_open (GtkHdyFlap *self,
                                gboolean    swipe_to_open)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  swipe_to_open = !!swipe_to_open;

  if (self->swipe_to_open == swipe_to_open)
    return;

  self->swipe_to_open = swipe_to_open;

  update_swipe_tracker (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SWIPE_TO_OPEN]);
}

/**
 * gtk_hdy_flap_get_swipe_to_close:
 * @self: a #GtkHdyFlap
 *
 * Gets whether @self can be closed with a swipe gesture.
 *
 * Returns: %TRUE if @self can be closed with a swipe gesture
 *
 * Since: 1.1
 */
gboolean
gtk_hdy_flap_get_swipe_to_close (GtkHdyFlap *self)
{
  g_return_val_if_fail (GTK_IS_HDY_FLAP (self), FALSE);

  return self->swipe_to_close;
}

/**
 * gtk_hdy_flap_set_swipe_to_close:
 * @self: a #GtkHdyFlap
 * @swipe_to_close: Whether @self can be closed with a swipe gesture
 *
 * Sets whether @self can be closed with a swipe gesture.
 *
 * The area that can be swiped depends on the #GtkHdyFlap:transition-type value.
 *
 * Since: 1.1
 */
void
gtk_hdy_flap_set_swipe_to_close (GtkHdyFlap *self,
                                 gboolean    swipe_to_close)
{
  g_return_if_fail (GTK_IS_HDY_FLAP (self));

  swipe_to_close = !!swipe_to_close;

  if (self->swipe_to_close == swipe_to_close)
    return;

  self->swipe_to_close = swipe_to_close;

  update_swipe_tracker (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SWIPE_TO_CLOSE]);
}
