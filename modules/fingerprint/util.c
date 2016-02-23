
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
  gdk_pixbuf_save_to_buffer (pixbuf, (gchar**)&png_buffer, &png_size, "png", &error, NULL);
  gchar *base64 = g_base64_encode (png_buffer, png_size);
  //g_print("data %s\n", base64);
  g_free(png_buffer);
  g_object_unref (pixbuf);
  g_print("ok\n");
  return base64;
}


void
save_print_data (struct fp_print_data *print_data, const gchar *name)
{

}
