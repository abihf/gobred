
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fingerprint.h"


static void
pixbuf_destroy(guchar *pixels, gpointer data)
{
	g_free(pixels);
}

gchar *
img_to_base64(struct fp_img *img)
{
  int width = fp_img_get_width (img);
  int height = fp_img_get_height (img);
  guint8 *data = fp_img_get_data (img);
  int size = width * height;
  guint8 *rgbdata = g_new(guint8, size * 3);
  int i, j=0;
  for (i=0; i<size; i++) {
    guint8 pixel = data[i];
    rgbdata[j++] = pixel;
    rgbdata[j++] = pixel;
    rgbdata[j++] = pixel;
  }
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(rgbdata, GDK_COLORSPACE_RGB,
			FALSE, 8, width, height, width * 3, pixbuf_destroy, NULL);

  guint8 *png_buffer = NULL;
  gsize png_size = 0;
  GError *error = NULL;
  gdk_pixbuf_save_to_buffer (pixbuf, (gchar**)&png_buffer, &png_size, "png", &error, "compression", 9, NULL);
  gchar *base64 = g_base64_encode (png_buffer, png_size);
  //g_print("data %s\n", base64);
  g_free(png_buffer);
  g_object_unref (pixbuf);
  g_print("ok\n");
  return base64;
}

static const gchar *BASE_DATA_DIR = "gobred/fingerprint";

gchar *
get_print_data_path (const gchar *name)
{
  return g_build_path ("/", g_get_user_data_dir(), BASE_DATA_DIR, name, NULL);
}

typedef struct {
  gpointer buffer;
  gsize length;

} PrintDataSaveUserData;

gint
save_print_data (struct fp_print_data *print_data, const gchar *name, GError **error)
{
  gchar *file_path = get_print_data_path(name);
  GFile *file = g_file_new_for_path (file_path);
  g_free (file_path);

  GOutputStream *write_stream = G_OUTPUT_STREAM (
    g_file_replace (file, NULL, FALSE, 0, NULL, error));
  if (error && *error != NULL) return -1;

  guint8 *buffer = NULL;
  gsize length = fp_print_data_get_data (print_data, &buffer);

  g_output_stream_write (write_stream, &length, sizeof(length), NULL, NULL);

  GConverter *gz_converter = G_CONVERTER (
    g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, -1));
  GOutputStream *gz_stream = G_OUTPUT_STREAM (
    g_converter_output_stream_new (write_stream, gz_converter));

  PrintDataSaveUserData *user_data = g_slice_new(PrintDataSaveUserData);
  user_data->buffer = buffer;
  user_data->length = length;

  g_output_stream_write (gz_stream, buffer, length, NULL, NULL);
  //g_file_replace_contents (file, );

  g_free (buffer);
  g_object_unref (gz_stream);
  g_object_unref (write_stream);
  g_object_unref (file);
  return 0;
}

struct fp_print_data *
load_print_data (const gchar *name, GError **error)
{
  gchar *file_path = get_print_data_path(name);
  GFile *file = g_file_new_for_path (file_path);
  g_free (file_path);

  GInputStream *read_stream = G_INPUT_STREAM (
    g_file_read (file, NULL, error) );
  if (error && *error != NULL)
    return NULL;

  gsize length = 0;
  g_input_stream_read (read_stream, &length, sizeof(length), NULL, NULL);
  guint8 *buffer = g_malloc (length);

  GConverter *gz_converter = G_CONVERTER (
    g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP));
  GInputStream *gz_stream = G_INPUT_STREAM (
    g_converter_input_stream_new (read_stream, gz_converter));

  g_input_stream_read (gz_stream, buffer, length, NULL, NULL);

  struct fp_print_data *data =
    fp_print_data_from_data (buffer, length);

  g_free (buffer);
  g_object_unref (gz_stream);
  g_object_unref (read_stream);
  g_object_unref (file);

  return data;
}
