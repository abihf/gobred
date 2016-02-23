
#include <glib.h>
#include <libgobred/gobred.h>
#include <libfprint/fprint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fingerprint.h"

extern Device *devices;

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

void
fingerprint_handle_enroll (GobredArray *params, GobredMethodCallBack *cb)
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
fingerprint_handle_cancle_enroll (GobredArray *params, GobredMethodCallBack *cb)
{

}
