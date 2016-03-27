
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fingerprint.h"

extern Device *devices;

typedef struct {
  gint index;
  gboolean return_image;
  gchar *save_name;

  //GobredArray *params;
  GobredMethodCallBack *cb;

  gint stage;
  gint max_stage;
  gboolean cancelled;
  GobredMethodCallBack *cancel_cb;
} EnrollData;


static void
enroll_data_free (EnrollData *enroll_data)
{
  if (enroll_data->cb) {
    if (enroll_data->cancelled)
      gobred_method_callback_throw_error(&enroll_data->cb, "Enroll cancelled");
    else
      gobred_method_callback_throw_error(&enroll_data->cb, "unknown error");
  }
  if (enroll_data->cancel_cb != NULL) {
    gobred_method_callback_return (&enroll_data->cancel_cb, GOBRED_BOOLEAN_TRUE);
  }
  //gobred_value_unref (enroll_data->params);
  g_slice_free (EnrollData, enroll_data);
}


static void
enroll_stopped_cb (struct fp_dev *dev, gpointer user_data)
{
  gint index = GPOINTER_TO_INT(user_data);
  EnrollData *enroll_data = (EnrollData *)devices[index].data;
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

  g_print("enroll result: %d\n", result);

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
  else {
    GobredDict *result = gobred_dict_new("status", GOBRED_VALUE_TYPE_STRING, status, NULL);
    if (img != NULL && enroll_data->return_image) {
      gchar *data = img_to_base64 (img);
      gobred_dict_set (result, "image", gobred_value_new_take_string (data));
    }
    if (success) {
      if (enroll_data->save_name) {
        GError *error = NULL;
        save_print_data (print_data, enroll_data->save_name, &error);
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
    gpointer stop_data = GINT_TO_POINTER(enroll_data->index);
    if (fp_async_enroll_stop(dev, enroll_stopped_cb, stop_data) < 0)
      enroll_stopped_cb(dev, stop_data);
  }

}

static void
dev_opened_for_enroll_cb (struct fp_dev *dev, int status, void *user_data)
{
  // TODO: handle failed
  EnrollData *enroll_data = (EnrollData *)user_data;
  devices[enroll_data->index].dev = dev;
  if (fp_async_enroll_start (dev, enroll_stage_cb, user_data) < 0) {
    gobred_method_callback_throw_error (&enroll_data->cb, "can not enroll");
    enroll_data_free(enroll_data);
  }
}

void
fingerprint_handle_enroll (GobredArray *params, GobredMethodCallBack *cb)
{
  int param_len = gobred_array_get_length (params);
  if (param_len < 1) {
    gobred_method_callback_throw_error (&cb, "Invalid number of params");
    return;
  }
  GobredValue *param0 = gobred_array_get (params, 0);
  const gchar *name = NULL;
  GobredDict *options = NULL;
  if (gobred_value_is_dict (param0)) {
    options = param0;
  } else {
    name = gobred_value_get_string(param0);
    if (param_len >= 2)
      options = gobred_array_get_dict (params, 1);
  }

  gint index = 0;
  gboolean return_image = FALSE;
  if (options) {
    index = gobred_dict_get_number (options, "device", 0.0);
    return_image = gobred_dict_get_boolean (options, "return_image", FALSE);
  }

  if (devices[index].state != FINGERPRINT_STATE_READY) {
    gobred_method_callback_throw_error (&cb, "Device not ready");
    return;
  }

  EnrollData *enroll_data = g_slice_new(EnrollData);
  //enroll_data->params = gobred_value_ref (params);
  enroll_data->save_name = name ? g_strdup(name) : NULL;
  enroll_data->return_image = return_image;
  enroll_data->cb = cb;
  enroll_data->stage = 0;
  enroll_data->index = index;
  if (options) {

  }

  g_atomic_int_set (&devices[index].state, FINGERPRINT_STATE_ENROLLING);
  devices[index].data = enroll_data;

  if (devices[index].dev) {
    if (fp_async_enroll_start (devices[index].dev, enroll_stage_cb, enroll_data) < 0) {
      gobred_method_callback_throw_error (&enroll_data->cb, "can not enroll");
      goto clean;
    }
  } else {
    if (fp_async_dev_open (devices[index].ddev, dev_opened_for_enroll_cb, enroll_data)) {
      gobred_method_callback_throw_error (&enroll_data->cb, "can not open device");
      goto clean;
    }
  }
  return;

clean:
  enroll_data_free(enroll_data);
}


void
fingerprint_handle_cancel_enroll (GobredArray *params, GobredMethodCallBack *cb)
{
  int index = 0;
  // TODO: what if device is still being opened?
  if (devices[index].dev != NULL && devices[index].state == FINGERPRINT_STATE_ENROLLING) {
    EnrollData *enroll_data = (EnrollData *)devices[index].data;
    enroll_data->cancelled = TRUE;
    enroll_data->cancel_cb = cb;
    fp_async_enroll_stop (devices[index].dev, enroll_stopped_cb, GINT_TO_POINTER(index));
  } else {
    gobred_method_callback_return (&cb, GOBRED_BOOLEAN_TRUE);
  }
}
