/*
 * Copyright (C) 2011 Red Hat Inc.
 *
 * Author:
 *      Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "reftest-compare.h"
#include "reftest-module.h"
#include "reftest-snapshot.h"

#include <string.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
# include <direct.h>
#endif

typedef enum {
  SNAPSHOT_WINDOW,
  SNAPSHOT_DRAW
} SnapshotMode;

/* This is exactly the style information you've been looking for */
#define GTK_STYLE_PROVIDER_PRIORITY_FORCE G_MAXUINT

static char *arg_output_dir = NULL;
static char *arg_base_dir = NULL;
static char *arg_direction = NULL;
static char *arg_compare_dir = NULL;

static const GOptionEntry test_args[] = {
  { "output",         'o', 0, G_OPTION_ARG_FILENAME, &arg_output_dir,
    "Directory to save image files to", "DIR" },
  { "directory",        'd', 0, G_OPTION_ARG_FILENAME, &arg_base_dir,
    "Directory to run tests from", "DIR" },
  { "direction",       0, 0, G_OPTION_ARG_STRING, &arg_direction,
    "Set text direction", "ltr|rtl" },
  { "compare-with",    0, 0, G_OPTION_ARG_FILENAME, &arg_compare_dir,
    "Directory to compare with", "DIR" },
  { NULL }
};

static gboolean
parse_command_line (int *argc, char ***argv)
{
  GError *error = NULL;
  GOptionContext *context;
  char *schema_dir;

  context = g_option_context_new ("- run GTK reftests");
  g_option_context_add_main_entries (context, test_args, NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);

  if (!g_option_context_parse (context, argc, argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      return FALSE;
    }

  gtk_test_init (argc, argv);

  /* g_test_build_filename must be called after gtk_test_init */
  schema_dir = g_test_build_filename (G_TEST_BUILT, "", NULL);
  if (g_getenv ("GTK_TEST_MESON") == NULL)
    g_setenv ("GSETTINGS_SCHEMA_DIR", schema_dir, TRUE);
  g_free (schema_dir);

  if (g_strcmp0 (arg_direction, "rtl") == 0)
    gtk_widget_set_default_direction (GTK_TEXT_DIR_RTL);
  else if (g_strcmp0 (arg_direction, "ltr") == 0)
    gtk_widget_set_default_direction (GTK_TEXT_DIR_LTR);
  else if (arg_direction != NULL)
    g_printerr ("Invalid argument passed to --direction argument. Valid arguments are 'ltr' and 'rtl'\n");

  g_type_ensure (G_TYPE_THEMED_ICON);

  return TRUE;
}

static const char *
get_output_dir (void)
{
  static const char *output_dir = NULL;
  GError *error = NULL;

  if (output_dir)
    return output_dir;

  if (arg_output_dir)
    {
      GFile *file = g_file_new_for_commandline_arg (arg_output_dir);
      output_dir = g_file_get_path (file);
      g_object_unref (file);
    }
  else
    {
      output_dir = g_get_tmp_dir ();
    }

  if (!g_file_test (output_dir, G_FILE_TEST_EXISTS))
    {
      GFile *file;

      file = g_file_new_for_path (output_dir);
      g_assert (g_file_make_directory_with_parents (file, NULL, &error));
      g_assert_no_error (error);
      g_object_unref (file);
    }

  return output_dir;
}

static void
get_components_of_test_file (const char  *test_file,
                             char       **directory,
                             char       **basename)
{
  if (directory)
    {
      *directory = g_path_get_dirname (test_file);
    }

  if (basename)
    {
      char *base = g_path_get_basename (test_file);
      
      if (g_str_has_suffix (base, ".ui"))
        base[strlen (base) - strlen (".ui")] = '\0';

      *basename = base;
    }
}

static char *
get_output_file (const char *test_file,
                 const char *extension)
{
  const char *output_dir = get_output_dir ();
  char *result, *base;

  get_components_of_test_file (test_file, NULL, &base);

  result = g_strconcat (output_dir, G_DIR_SEPARATOR_S, base, extension, NULL);
  g_free (base);

  return result;
}

static char *
get_test_file (const char *test_file,
               const char *extension,
               gboolean    must_exist)
{
  GString *file = g_string_new (NULL);
  char *dir, *base;

  get_components_of_test_file (test_file, &dir, &base);

  file = g_string_new (dir);
  g_string_append (file, G_DIR_SEPARATOR_S);
  g_string_append (file, base);
  g_string_append (file, extension);

  g_free (dir);
  g_free (base);

  if (must_exist &&
      !g_file_test (file->str, G_FILE_TEST_EXISTS))
    {
      g_string_free (file, TRUE);
      return NULL;
    }

  return g_string_free (file, FALSE);
}

static char *
get_reference_image (const char *ui_file)
{
  char *base;
  char *reference_image;

  if (!arg_compare_dir)
    return NULL;

  get_components_of_test_file (ui_file, NULL, &base);
  reference_image = g_strconcat (arg_compare_dir, G_DIR_SEPARATOR_S, base, ".out.png", NULL);
  g_free (base);

  if (!g_file_test (reference_image, G_FILE_TEST_EXISTS))
    {
      g_free (reference_image);
      return NULL;
    }

  return reference_image;
}

static GtkStyleProvider *
add_extra_css (const char *testname,
               const char *extension)
{
  GtkStyleProvider *provider = NULL;
  char *css_file;
  
  css_file = get_test_file (testname, extension, TRUE);
  if (css_file == NULL)
    return NULL;

  provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_path (GTK_CSS_PROVIDER (provider),
                                   css_file,
                                   NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_FORCE);

  g_free (css_file);
  
  return provider;
}

static void
remove_extra_css (GtkStyleProvider *provider)
{
  if (provider == NULL)
    return;

  gtk_style_context_remove_provider_for_screen (gdk_screen_get_default (),
                                                provider);
}

static void
save_image (cairo_surface_t *surface,
            const char      *test_name,
            const char      *extension)
{
  char *filename = get_output_file (test_name, extension);

  g_test_message ("Storing test result image at %s", filename);
  g_assert (cairo_surface_write_to_png (surface, filename) == CAIRO_STATUS_SUCCESS);

  g_free (filename);
}

static char *
get_test_keyfile (const char *ui_file)
{
  return get_test_file (ui_file, ".keyfile", TRUE);
}

static void
test_ui_file (GFile *file)
{
  char *ui_file, *reference_file;
  cairo_surface_t *ui_image, *reference_image, *diff_image;
  GtkStyleProvider *provider;
  guint max_diff = 0;
  guint pixels_changed = 0;
  guint pixels = 0;

  ui_file = g_file_get_path (file);

  provider = add_extra_css (ui_file, ".css");

  ui_image = reftest_snapshot_ui_file (ui_file);

  if ((reference_file = get_reference_image (ui_file)) != NULL)
    reference_image = cairo_image_surface_create_from_png (reference_file);
  else if ((reference_file = get_test_file (ui_file, ".ref.ui", TRUE)) != NULL)
    reference_image = reftest_snapshot_ui_file (reference_file);
  else
    {
      reference_image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
      g_test_message ("No reference image.");
      g_test_fail ();
    }
  g_free (reference_file);

  diff_image = reftest_compare_surfaces (ui_image, reference_image,
                                         &max_diff, &pixels_changed, &pixels);

  save_image (ui_image, ui_file, ".out.png");
  save_image (reference_image, ui_file, ".ref.png");

  if (diff_image)
    {
      char *keyfile_path = get_test_keyfile (ui_file);
      GKeyFile *keyfile = g_key_file_new ();
      guint64 tolerated_diff = 0;
      guint64 tolerated_pixels = 0;

      if (keyfile_path != NULL)
        {
          GError *error = NULL;
          g_key_file_load_from_file (keyfile, keyfile_path, G_KEY_FILE_NONE, &error);
          g_assert_no_error (error);
          tolerated_diff = g_key_file_get_uint64 (keyfile, "reftest", "tolerated-diff-level", NULL);
          g_test_message ("Maximum difference tolerated: %" G_GUINT64_FORMAT " levels", tolerated_diff);
          tolerated_pixels = g_key_file_get_uint64 (keyfile, "reftest", "tolerated-diff-pixels", NULL);
          g_test_message ("Different pixels tolerated: %" G_GUINT64_FORMAT, tolerated_pixels);
        }

      g_test_message ("%u (out of %u) pixels differ from reference by up to %u levels",
                      pixels_changed, pixels, max_diff);
      save_image (diff_image, ui_file, ".diff.png");
      cairo_surface_destroy (diff_image);

      if (max_diff <= tolerated_diff && pixels_changed <= tolerated_pixels)
        g_test_message ("not right, but close enough?");
      else
        g_test_fail ();

      g_key_file_unref (keyfile);
      g_free (keyfile_path);
    }

  remove_extra_css (provider);
}

static int
compare_files (gconstpointer a, gconstpointer b)
{
  GFile *file1 = G_FILE (a);
  GFile *file2 = G_FILE (b);
  char *path1, *path2;
  int result;

  path1 = g_file_get_path (file1);
  path2 = g_file_get_path (file2);

  result = strcmp (path1, path2);

  g_free (path1);
  g_free (path2);

  return result;
}

static void
add_test_for_file (GFile *file)
{
  GFileEnumerator *enumerator;
  GFileInfo *info;
  GList *files;
  GError *error = NULL;


  if (g_file_query_file_type (file, 0, NULL) != G_FILE_TYPE_DIRECTORY)
    {
      g_test_add_vtable (g_file_get_path (file),
                         0,
                         g_object_ref (file),
                         NULL,
                         (GTestFixtureFunc) test_ui_file,
                         (GTestFixtureFunc) g_object_unref);
      return;
    }


  enumerator = g_file_enumerate_children (file, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, &error);
  g_assert_no_error (error);
  files = NULL;

  while ((info = g_file_enumerator_next_file (enumerator, NULL, &error)))
    {
      const char *filename;

      filename = g_file_info_get_name (info);

      if (!g_str_has_suffix (filename, ".ui") ||
          g_str_has_suffix (filename, ".ref.ui"))
        {
          g_object_unref (info);
          continue;
        }

      files = g_list_prepend (files, g_file_get_child (file, filename));

      g_object_unref (info);
    }
  
  g_assert_no_error (error);
  g_object_unref (enumerator);

  files = g_list_sort (files, compare_files);
  g_list_foreach (files, (GFunc) add_test_for_file, NULL);
  g_list_free_full (files, g_object_unref);
}

static void
enforce_default_settings (void)
{
  GtkSettings *settings;
  GTypeClass *klass;
  GParamSpec **pspecs;
  guint n_pspecs;
  int i;

  settings = gtk_settings_get_default ();

  klass = g_type_class_ref (G_OBJECT_TYPE (settings));

  pspecs = g_object_class_list_properties (G_OBJECT_CLASS (klass), &n_pspecs);
  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      const GValue *value;

      if ((pspec->flags & G_PARAM_WRITABLE) == 0)
        continue;

      if (pspec->value_type == G_TYPE_HASH_TABLE)
        continue;

      value = g_param_spec_get_default_value (pspec);
      g_object_set_property (G_OBJECT (settings), pspec->name, value);
    }

  g_free (pspecs);

  g_type_class_unref (klass);
}

int
main (int argc, char **argv)
{
  const char *basedir;
  
  /* I don't want to fight fuzzy scaling algorithms in GPUs,
   * so unless you explicitly set it to something else, we
   * will use Cairo's image surface.
   */
  g_setenv ("GDK_RENDERING", "image", FALSE);

  if (!parse_command_line (&argc, &argv))
    return 1;

  enforce_default_settings ();

  if (arg_base_dir)
    basedir = arg_base_dir;
  else
    basedir = g_test_get_dir (G_TEST_DIST);

  if (argc < 2)
    {
      GFile *dir;

      dir = g_file_new_for_path (basedir);
      
      add_test_for_file (dir);

      g_object_unref (dir);
    }
  else
    {
      guint i;

      for (i = 1; i < argc; i++)
        {
          GFile *file = g_file_new_for_commandline_arg (argv[i]);

          add_test_for_file (file);

          g_object_unref (file);
        }
    }

  /* We need to ensure the process' current working directory
   * is the same as the reftest data, because we're using the
   * "file" property of GtkImage as a relative path in builder files.
   */
  chdir (basedir);

  return g_test_run ();
}

