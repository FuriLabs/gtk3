/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __REFTEST_COMPARE_H__
#define __REFTEST_COMPARE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_MODULE_EXPORT
cairo_surface_t *       reftest_compare_surfaces        (cairo_surface_t        *surface1,
                                                         cairo_surface_t        *surface2,
                                                         guint                  *max_diff_out,
                                                         guint                  *pixels_changed_out,
                                                         guint                  *pixels_out);

G_END_DECLS

#endif /* __REFTEST_COMPARE_H__ */
