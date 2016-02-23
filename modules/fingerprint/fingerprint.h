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
  gpointer data;
} Device;

gchar *
img_to_base64(struct fp_img *img);

void
fingerprint_handle_enroll (GobredValue *params, GobredMethodCallBack *cb);

void
fingerprint_handle_cancle_enroll (GobredValue *params, GobredMethodCallBack *cb);

void
save_print_data (struct fp_print_data *print_data, const gchar *name);
