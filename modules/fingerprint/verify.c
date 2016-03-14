
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fingerprint.h"

extern Device *devices;

typedef struct {
  gint index;
  gboolean return_image;
  gchar *name;
  struct fp_print_data *print_data;
  gint threshold;

  //GobredArray *params;
  GobredMethodCallBack *cb;

  gboolean cancelled;
  GobredMethodCallBack *cancel_cb;
} VerifyData;


static void
verify_data_free (VerifyData *verify_data)
{
  if (verify_data->cb) {
    if (verify_data->cancelled)
      gobred_method_callback_throw_error(&verify_data->cb, "Enroll cancelled");
    else
      gobred_method_callback_throw_error(&verify_data->cb, "unknown error");
  }
  if (verify_data->cancel_cb != NULL) {
    gobred_method_callback_return (&verify_data->cancel_cb, GOBRED_BOOLEAN_TRUE);
  }

  fp_print_data_free (verify_data->print_data);
  //gobred_value_unref (enroll_data->params);
  g_slice_free (VerifyData, verify_data);
}


static void
verify_stopped_cb (struct fp_dev *dev, gpointer user_data)
{
  gint index = GPOINTER_TO_INT(user_data);
  VerifyData *verify_data = (VerifyData *)devices[index].data;
  g_atomic_int_set (&devices[verify_data->index].state, FINGERPRINT_STATE_READY);
  //devices[index].dev = NULL;
  verify_data_free(verify_data);
  //fp_async_dev_close(dev, device_closed_cb, NULL);
}



static void
verify_stage_cb (struct fp_dev *dev,
                 int result,
                 struct fp_img *img,
                 void *user_data)
{
  VerifyData *verify_data = (VerifyData *)user_data;
  gboolean success = FALSE;
  gboolean complete = FALSE;
  gchar *error = NULL;
  gchar *status = NULL;

  g_print("verify result: %d\n", result);

  switch (result) {
	case FP_VERIFY_MATCH:
		//printf("Enroll complete!\n");
    status = "match";
    success = complete = TRUE;
		break;
	case FP_VERIFY_NO_MATCH:
		status = "not-match";
    complete = TRUE;
		break;
	case FP_VERIFY_RETRY:
		status = "retry";
		break;
	case FP_VERIFY_RETRY_TOO_SHORT:
		status = "too-short";
		break;
	case FP_VERIFY_RETRY_CENTER_FINGER:
		status = "center-finger";
		break;
	case FP_VERIFY_RETRY_REMOVE_FINGER:
		status = "remove-finger";
		break;

  default:
    error = "FAIL: invalid result";
    complete = TRUE;
	}

  GobredMethodCallBack **mcb = &verify_data->cb;
  if (error) {
    g_print("ini jangan di panggil\n");
    gobred_method_callback_throw_error (mcb, "enroll error: %s", error);
  }
  //else if (enroll_data->stage >= enroll_data->max_stage) {
  //  gobred_method_callback_throw_error (mcb, "enroll failed");
  //}
  else {
    GobredDict *result = gobred_dict_new("status",
                                         GOBRED_VALUE_TYPE_STRING,
                                         status,
                                         NULL);
    if (img != NULL && verify_data->return_image) {
      gchar *data = img_to_base64 (img);
      gobred_dict_set (result, "image", gobred_value_new_take_string (data));
    }
    gobred_method_callback_return (mcb, result);
  }

  if (img)
    fp_img_free (img);

  if (complete) {
    gpointer stop_data = GINT_TO_POINTER(verify_data->index);
    if (fp_async_verify_stop(dev, verify_stopped_cb, stop_data) < 0)
      verify_stopped_cb(dev, stop_data);
  }

}

static void
dev_opened_for_verify_cb (struct fp_dev *dev, int status, void *user_data)
{
  // TODO: handle failed
  VerifyData *verify_data = (VerifyData *)user_data;
  devices[verify_data->index].dev = dev;
  if (fp_async_verify_start (dev, verify_data->print_data, verify_stage_cb, user_data) < 0) {
    gobred_method_callback_throw_error (&verify_data->cb, "can not enroll");
    verify_data_free(verify_data);
  }
}

void
fingerprint_handle_verify (GobredArray *params, GobredMethodCallBack *cb)
{
  int param_len = gobred_array_get_length (params);
  if (param_len < 1) {
    gobred_method_callback_throw_error (&cb, "Invalid number of params");
    return;
  }


  const gchar *name = gobred_array_get_string (params, 0);
  GobredDict *options = gobred_array_get_dict (params, 1);

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


  VerifyData *verify_data = g_slice_new(VerifyData);
  //enroll_data->params = gobred_value_ref (params);
  verify_data->name = g_strdup(name);
  verify_data->return_image = return_image;
  verify_data->cb = cb;
  verify_data->index = index;
  verify_data->print_data = load_print_data (name, NULL);

  g_atomic_int_set (&devices[index].state, FINGERPRINT_STATE_VERIFYING);
  devices[index].data = verify_data;

  if (devices[index].dev) {
    if (fp_async_verify_start (devices[index].dev, verify_data->print_data, verify_stage_cb, verify_data) < 0) {
      gobred_method_callback_throw_error (&verify_data->cb, "can not enroll");
      goto clean;
    }
  } else {
    if (fp_async_dev_open (devices[index].ddev, dev_opened_for_verify_cb, verify_data)) {
      gobred_method_callback_throw_error (&verify_data->cb, "can not open device");
      goto clean;
    }
  }
  return;

clean:
  verify_data_free(verify_data);
}


void
fingerprint_handle_cancel_verify (GobredArray *params, GobredMethodCallBack *cb)
{
  int index = 0;
  // TODO: what if device is still being opened?
  if (devices[index].dev != NULL && devices[index].state == FINGERPRINT_STATE_VERIFYING) {
    VerifyData *enroll_data = (VerifyData *)devices[index].data;
    enroll_data->cancelled = TRUE;
    enroll_data->cancel_cb = cb;
    fp_async_verify_stop (devices[index].dev, verify_stopped_cb, GINT_TO_POINTER(index));
  } else {
    gobred_method_callback_return (&cb, GOBRED_BOOLEAN_TRUE);
  }
}
