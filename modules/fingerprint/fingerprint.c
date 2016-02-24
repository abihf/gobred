
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fingerprint.h"

Device *devices = NULL;
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

/*
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
} */

static void
device_closed_cb (struct fp_dev *dev, gpointer user_data)
{
  devices[GPOINTER_TO_INT(user_data)].dev = NULL;
}

static void
fingerprint_handle_device_close (GobredValue *params, GobredMethodCallBack *cb)
{
  fp_async_dev_close (devices[0].dev, device_closed_cb, NULL);
  gobred_method_callback_return (&cb, GOBRED_BOOLEAN_TRUE);
}

static GobredMethodDefinitionV0 methods[] = {
  {"getDeviceInfo", .handler.simple = fingerprint_get_device_info},
  {"getDevices", .handler.simple = fingerprint_get_devices},
  {"enroll", .handler.simple = fingerprint_handle_enroll},
  {"cancelEnroll", .handler.simple = fingerprint_handle_cancel_enroll},
  {"close", .handler.simple = fingerprint_handle_device_close},
  {NULL}
};

static GobredModuleDefinitionV0 definition = {
  {0, "fingerprint"},
  .methods = methods,
  .init = _init,
  .free = _free
};

G_MODULE_EXPORT GobredModuleDefinitionBase *
gobred_module_register ()
{
  return &definition.base;
}
