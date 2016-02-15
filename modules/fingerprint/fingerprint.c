
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct fp_dev FPDevice;
typedef struct fp_dscv_dev FPDiscoverDevice;
typedef struct fp_driver FPDriver;

int setup_pollfds(void);

enum {
  FINGERPRINT_STATE_READY,
  FINGERPRINT_STATE_ENROLLING,
  FINGERPRINT_STATE_VERIFYING,
};

typedef struct {
  const gchar *name;
  FPDevice *dev;
  FPDiscoverDevice *ddev;
  volatile gint state;
} Device;

static Device *devices = NULL;
static struct fp_dscv_dev **discovered_devs;
static gint devices_num = 0;

static void
_init()
{
  if (fp_init() < 0) {
    g_warning ("[fingerprint] Failed to initialize fprint");
    return;
  }

  discovered_devs = fp_discover_devs();
  FPDiscoverDevice *ddev = NULL;


  if (discovered_devs) {
    int i;
    for (i=0; discovered_devs[i]; i++){}
    devices_num = i;

    devices = g_new0(Device, devices_num);
    for (i=0; i<devices_num; i++) {
      ddev = discovered_devs[0];
      FPDriver *drv = fp_dscv_dev_get_driver(ddev);
      devices[i].name = fp_driver_get_full_name(drv);
      devices[i].ddev = ddev;
    }
  }
  setup_pollfds();
}

static void
_free()
{
  int i;
  for (i=0; i<devices_num; i++) {
    if (devices[i].dev)
      fp_dev_close(devices[i].dev);
  }
  if (discovered_devs)
    fp_dscv_devs_free (discovered_devs);
  fp_exit ();
}

static void
fingerprint_get_devices (GobredArray *params, GobredMethodCallBack *cb)
{
  int i;
  g_print("disini masuk ga? %d\n", devices_num);
  GobredArray *result = gobred_array_new (devices_num, NULL);
  for (i=0; i<devices_num; i++) {
    gobred_array_add_string (result, devices[i].name);
    g_print("found %s\n", devices[i].name);
  }
  gobred_method_callback_return (&cb, result);
}

static FPDevice *
open_dev(gint index)
{
  g_print ("!! JANGAN DI PANGGIL");
  if (devices[index].dev) {
    return devices[index].dev;
  }
  else {
    return devices[index].dev = fp_dev_open (devices[index].ddev);
  }
}

static void
fingerprint_get_device_info (GobredArray *params, GobredMethodCallBack *cb)
{
  if (gobred_array_get_length(params) < 1) {
    gobred_method_callback_throw_error (&cb, "Invalid number of parameter");
    return;
  }

  int index = (int) gobred_array_get_number (params, 0);
  if (index >= devices_num) {
    gobred_method_callback_throw_error (&cb, "Device not found");
    return;
  }
  FPDevice *dev = open_dev(index);
  FPDriver *drv = fp_dev_get_driver(dev);

  gchar *scan_type = "unknown";
  double enroll_stage = fp_dev_get_nr_enroll_stages (dev);
  GobredValue *result = gobred_dict_new (
    "driver", GOBRED_VALUE_TYPE_STRING, fp_driver_get_name(drv),
    "driver-name", GOBRED_VALUE_TYPE_STRING, fp_driver_get_full_name (drv),
    "enroll-stage", GOBRED_VALUE_TYPE_NUMBER, enroll_stage,
    "scan-type", GOBRED_VALUE_TYPE_STRING, scan_type,
    NULL);
  gobred_method_callback_return (&cb, result);
}


static void
save_print_data (struct fp_print_data *print_data, const gchar *name)
{

}


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

  g_print("size %d\n", size);
  guint8 *png_buffer = NULL;
  gsize png_size = 0;
  GError *error = NULL;
  gdk_pixbuf_save_to_buffer (pixbuf, (gchar**)&png_buffer, &png_size, "png", &error, NULL);
  g_print("png size %lu\n", png_size);
  gchar *base64 = g_base64_encode (png_buffer, png_size);
  //g_print("data %s\n", base64);
  g_free(png_buffer);
  g_object_unref (pixbuf);
  g_print("ok\n");
  return base64;
}

static void
device_closed_cb (struct fp_dev *dev, gpointer user_data);

typedef struct {
  gint index;
  GobredArray *params;
  GobredMethodCallBack *cb;
  gint stage;
  gint max_stage;
} EnrollData;

static void
enroll_data_free (EnrollData *edata)
{
  if (edata->cb)
    gobred_method_callback_throw_error(&edata->cb, "unknown error");
  gobred_value_unref (edata->params);
  g_slice_free (EnrollData, edata);
}

static void
enroll_stopped_cb (struct fp_dev *dev, gpointer user_data)
{
  EnrollData *enroll_data = (EnrollData *)user_data;
  g_atomic_int_set (&devices[enroll_data->index].state, FINGERPRINT_STATE_READY);
  //devices[index].dev = NULL;
  g_print("enroll stopped closed\n");
  enroll_data_free(enroll_data);
  //fp_async_dev_close(dev, device_closed_cb, NULL);
}



static void
enroll_stage_cb (struct fp_dev *dev,
                 int result,
                 struct fp_print_data *print_data,
                 struct fp_img *img,
                 void *user_data)
{
  EnrollData *enroll_data = (EnrollData *)user_data;
  gboolean success = FALSE;
  gboolean complete = FALSE;
  gchar *error = NULL;
  gchar *status = NULL;

  if (dev != devices[0].dev)
    g_error("kok bisa beda???");

  switch (result) {
	case FP_ENROLL_COMPLETE:
		//printf("Enroll complete!\n");
    status = "done";
    success = complete = TRUE;
		break;
	case FP_ENROLL_FAIL:
		error = "FAIL: Enroll failed, something wen't wrong :(";
    complete = TRUE;
    //fp_async_enroll_stop(dev, _enroll_stopped_cb, NULL);
    //return;
		break;
	case FP_ENROLL_PASS:
    enroll_data->stage++;
    status = "pass";
		g_info("Enroll stage passed. Yay!");
		break;
	case FP_ENROLL_RETRY:
		status = "retry";
		break;
	case FP_ENROLL_RETRY_TOO_SHORT:
		status = "too-short";
		break;
	case FP_ENROLL_RETRY_CENTER_FINGER:
		status = "center-finger";
		break;
	case FP_ENROLL_RETRY_REMOVE_FINGER:
		status = "remove-finger";
		break;

  default:
    error = "FAIL: invalid result";
    complete = TRUE;
	}

  GobredMethodCallBack **mcb = &enroll_data->cb;
  if (error) {
    g_print("ini jangan di panggil\n");
    gobred_method_callback_throw_error (mcb, "enroll error: %s", error);
  }
  //else if (enroll_data->stage >= enroll_data->max_stage) {
  //  gobred_method_callback_throw_error (mcb, "enroll failed");
  //}
  else {
    GobredValue *options = gobred_array_get_dict(enroll_data->params, 2);
    GobredDict *result = gobred_dict_new("status", GOBRED_VALUE_TYPE_STRING, status, NULL);
    if (img != NULL && gobred_dict_get_boolean (options, "return_image", FALSE)) {
      gchar *data = img_to_base64 (img);
      gobred_dict_set (result, "image", gobred_value_new_take_string (data));
    }
    if (success) {
      if (gobred_dict_get_boolean(options, "save", TRUE)) {
        save_print_data (print_data, gobred_array_get_string (enroll_data->params, 1));
      }
      // save data
    }
    gobred_method_callback_return (mcb, result);
  }


  if (print_data)
    fp_print_data_free (print_data);
  if (img)
    fp_img_free (img);

  if (complete) {
    if (fp_async_enroll_stop(dev, enroll_stopped_cb, user_data) < 0)
      enroll_stopped_cb(dev, user_data);
    //g_idle_add (enroll_stop, stop_data);
    //enroll_stop(NULL);
  }

}

static void
dev_opened_for_enroll_cb (struct fp_dev *dev, int status, void *user_data)
{
  EnrollData *enroll_data = (EnrollData *)user_data;
  devices[enroll_data->index].dev = dev;
  if (fp_async_enroll_start (dev, enroll_stage_cb, user_data) < 0) {
    gobred_method_callback_throw_error (&enroll_data->cb, "can not enroll");
    enroll_data_free(enroll_data);
  }
}

static void
fingerprint_enroll (GobredValue *params, GobredMethodCallBack *cb)
{
  EnrollData *enroll_data = g_slice_new(EnrollData);
  gint index = 0;
  enroll_data->params = gobred_value_ref (params);
  enroll_data->cb = cb;
  enroll_data->stage = 0;
  enroll_data->index = index;

  g_atomic_int_set (&devices[index].state, FINGERPRINT_STATE_ENROLLING);

  if (devices[index].dev) {
    if (fp_async_enroll_start (devices[index].dev, enroll_stage_cb, enroll_data) < 0) {
      gobred_method_callback_throw_error (&cb, "can not enroll");
      goto clean;
    }
  } else {
    if (fp_async_dev_open (devices[index].ddev, dev_opened_for_enroll_cb, enroll_data)) {
      gobred_method_callback_throw_error (&cb, "can not open device");
      goto clean;
    }
  }
  return;

clean:
  enroll_data_free(enroll_data);
}

static void
fingerprint_enroll2 (GobredValue *params, GobredMethodCallBack *cb)
{
  FPDevice *dev = open_dev (0);
  struct fp_print_data *enrolled_print = NULL;
  struct fp_img *img = NULL;
  fp_enroll_finger_img(dev, &enrolled_print, &img);
	if (img) {
		fp_img_save_to_file(img, "enrolled.pgm");
		printf("Wrote scanned image to enrolled.pgm\n");
    gchar *data = img_to_base64 (img);
    GobredValue *result = gobred_value_new_string (data);
    g_free(data);
    gobred_method_callback_return (&cb, result);
		fp_img_free(img);
	} else {
    gobred_method_callback_throw_error (&cb, "gagal cuy");
  }
  if (enrolled_print) {
    fp_print_data_free (enrolled_print);
  }
}

static void
device_closed_cb (struct fp_dev *dev, gpointer       user_data)
{
  devices[0].dev = NULL;
}

static void
device_close (GobredValue *params, GobredMethodCallBack *cb)
{
  fp_async_dev_close (devices[0].dev, device_closed_cb, NULL);
  gobred_method_callback_return (&cb, GOBRED_BOOLEAN_TRUE);
}

static GobredMethodDefinitionV0 methods[] = {
  {.name = "close", .handler.simple = device_close},
  {.name = "enroll", .handler.simple = fingerprint_enroll},
  {.name = "enroll2", .handler.simple = fingerprint_enroll2, .threaded = TRUE},
  {.name = "getDeviceInfo", .handler.simple = fingerprint_get_device_info},
  {.name = "getDevices", .handler.simple = fingerprint_get_devices},
  {NULL}
};

static GobredModuleDefinitionV0 definition = {
  .base.version = 0,
  .base.name = "fingerprint",
  .methods = methods,
  .init = _init,
  .free = _free
};

GobredModuleDefinitionBase *
gobred_module_register ()
{
  return &definition.base;
}
