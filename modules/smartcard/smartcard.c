//
#include <glib.h>
#include <winscard.h>
#include <string.h>
#include <libgobred/gobred.h>

#define MAX_READER 16

#define STATUS_NONE 0
#define STATUS_PRESENT 1
#define STATUS_CONNECTED 3

//////////////// error
//

enum {
  SMARTCARD_ERROR_
};

 typedef struct {
	char name[MAX_READERNAME];
	gboolean card_present;
	gboolean connected;
	SCARDHANDLE card;
	const SCARD_IO_REQUEST *io_request;
	BYTE atr[MAX_ATR_SIZE];
	int atr_len;

} ReaderInfo;

static const gchar *EVENT_CARD_PRESENT = "smartcard.card-present";
static const gchar *EVENT_CARD_REMOVED = "smartcard.card-removed";

static SCARDCONTEXT context_main = 0L;
static SCARDCONTEXT context_loop = 0L;

static int num_reader = 0;
static ReaderInfo *readers;

static gpointer
_status_loop_thread (gpointer);


static void
_init()
{
  SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context_main);
  //_scan_reader ();
  GThread *thread = g_thread_new ("smartcard-loop", _status_loop_thread, NULL);
  g_thread_unref (thread);
}

static void
_free()
{
  if (context_main)
    SCardReleaseContext(context_main);
  if (context_loop)
    SCardReleaseContext(context_loop);
  if (readers)
    g_free (readers);
}


static void
_scan_reader (SCARDCONTEXT context)
{
  static GMutex mutex;
  //g_return_if_fail (context_main != 0L);
  g_mutex_lock (&mutex);

  DWORD dwReaders;
	LPSTR ptr, mszReaders = NULL;
	dwReaders = 0; // SCARD_AUTOALLOCATE;
	LONG rv = SCardListReaders(context, NULL, NULL, &dwReaders);
	if (rv != SCARD_S_SUCCESS || dwReaders == 0) {
    g_warning ("smartcard: No reader found");
    return;
  }
  mszReaders = g_newa(char, dwReaders);
  SCardListReaders(context, NULL, mszReaders, &dwReaders);

  num_reader = 0;
  if (readers) g_free (readers);
  for (ptr = mszReaders; *ptr; ptr += strlen(ptr)) {
    g_debug("smartcard: found reader %s\n", ptr);
    num_reader++;
  }
  readers = g_new0(ReaderInfo, num_reader + 1);
  int i = 0;
	for (ptr = mszReaders; *ptr; ptr += strlen(ptr)) {
		strcpy(readers[i++].name, ptr);
	}

  g_mutex_unlock (&mutex);
  //g_free (mszReaders);
}

#define __h2b(h) ( (h>='0'&&h<='9') ? h-'0' : \
                   (h>='a'&&h<='f') ? h-'a'+10 : \
                   (h>='A'&&h<='F') ? h-'A'+10 : 0 \
                 )
static void
hex_to_bytes (guint8 *bytes, const gchar *hex)
{
  const char *h;
  guint8 *b;
  for (h = hex, b = bytes; *h; h+=2, b++) {
    *b = __h2b(h[0]) << 4 | __h2b(h[1]);
  }
}

static void
bytes_to_hex (gchar *hex, const guint8 *bytes, gsize size)
{
  static gchar chex[] = "0123456789abcdef";
  int i;
  char *h = hex;
  const guint8 *b = bytes;
  for (i=0; i<size; i++) {
    guint8 u = (b[i] >> 4) & 0xf;
    h[i*2] = chex[u];
    h[i*2+1] = chex[b[i] & 0xf];
  }
  h[size*2] = 0;
}

static gpointer
_status_loop_thread (gpointer data)
{
  SCARDCONTEXT context = 0;
	SCARD_READERSTATE *reader_states;
	int i;
	LONG rv;
	gboolean done = FALSE;
	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);

  if (num_reader == 0)
		_scan_reader (context);

	if (num_reader == 0) {
    SCardReleaseContext (context);
		return NULL; // no reader
  }

  //context_loop = context;
	reader_states = g_new0(SCARD_READERSTATE, num_reader);
	for(i=0; i<num_reader; i++) {
		reader_states[i].szReader = readers[i].name;
		reader_states[i].dwCurrentState = SCARD_STATE_UNAWARE;
	}

	while (!done) {
		rv = SCardGetStatusChange(context, 1000, reader_states, num_reader);
		if (rv == SCARD_E_TIMEOUT) continue;
		if (rv != SCARD_S_SUCCESS) break;
		for (i=0; i<num_reader; i++) {
		  if (reader_states[i].dwEventState & SCARD_STATE_CHANGED) {
		    if (reader_states[i].dwEventState & SCARD_STATE_PRESENT) {
		      readers[i].card_present = TRUE;
					readers[i].atr_len = MIN(reader_states[i].cbAtr, MAX_ATR_SIZE);
					memcpy(readers[i].atr, reader_states[i].rgbAtr, readers[i].atr_len);
          gchar *atr_hex = g_new(gchar, readers[i].atr_len * 2 + 1);
          bytes_to_hex (atr_hex, readers[i].atr, readers[i].atr_len);
          GobredDict *param = gobred_dict_new (
             "reader", GOBRED_VALUE_TYPE_NUMBER, (double)(i+1),
             "atr", GOBRED_VALUE_TYPE_STRING, atr_hex,
             0);
          g_info("card present %d %s", i, atr_hex);
          gobred_event_emit (EVENT_CARD_PRESENT, param);
          g_free (atr_hex);
		    }
        else if (reader_states[i].dwEventState & SCARD_STATE_EMPTY) {
					if (readers[i].connected) {
						SCardDisconnect(readers[i].card, SCARD_LEAVE_CARD);
						readers[i].connected = FALSE;
					}
		      readers[i].card_present = FALSE;
          GobredValue *param = gobred_dict_new (
             "reader", GOBRED_VALUE_TYPE_NUMBER, (double)(i+1),
             0);
          g_info("card removed %d", i);
          gobred_event_emit (EVENT_CARD_REMOVED, param);
		    }

        else {
		      g_warning("unknown state %s: %lu", reader_states[i].szReader, reader_states[i].dwEventState);
		    }
		    reader_states[i].dwCurrentState = reader_states[i].dwEventState;
		  }
		}
	}
  g_free (reader_states);
  SCardReleaseContext(context);
  return NULL;
}

static void
_get_readers (GobredValue *params, GobredMethodCallBack *cb)
{
  GobredValue *result = gobred_array_new (num_reader, 0);
  for (int i=0; i<num_reader; i++) {
    gobred_array_add (result, gobred_value_new_string(readers[i].name));
  }
  gobred_method_callback_return (&cb, result);
}

static void
_get_reader (GobredValue *params, GobredMethodCallBack *cb)
{
  gint index = (gint)gobred_array_get_number (params, 0) - 1;
  if (index < 0 || index >= num_reader) {
    gobred_method_callback_throw_error (&cb, "invalid reader index %d", index + 1);
  } else {
    GobredValue *result = gobred_dict_new (
      "name", GOBRED_VALUE_TYPE_STRING, readers[index].name,
      "card", GOBRED_VALUE_TYPE_STRING, readers[index].card_present ? "present" : "none",
      NULL
    );
    gobred_method_callback_return (&cb, result);
  }
}


static gboolean
_connect_reader (gint index)
{
  if (readers[index].connected) {
    return TRUE;
  }
  else {
    if (!readers[index].card_present) {
      return FALSE;
    } else {
      SCARDHANDLE h_card;
      DWORD active_protocol;
      SCardConnect (context_main,
                    readers[index].name,
                    SCARD_SHARE_SHARED,
                    SCARD_PROTOCOL_T0,
                    &h_card,
                    &active_protocol);
      readers[index].card = h_card;
      readers[index].io_request = SCARD_PCI_T0;
      readers[index].connected = TRUE;
      return TRUE;
    }
  }
}

static gint
_send_single_apdu (gint index, const gchar *apdu, gchar **response, gboolean return_ascii)
{
  ReaderInfo *reader = &readers[index];
  gsize apdu_len = strlen(apdu) >> 1;
  guint8 *apdu_bytes = g_newa(guint8, apdu_len);
  guint8 response_buffer[262];
  gsize response_length = 262;
  hex_to_bytes (apdu_bytes, apdu);
  LONG rv = SCardTransmit(reader->card,
                          reader->io_request,
                          apdu_bytes,
                          apdu_len,
                          NULL,
                          response_buffer,
                          &response_length);
  if (rv == SCARD_S_SUCCESS) {
    if (response_length > 2) {
      gchar *response_text;
      if (return_ascii) {
        response_text = g_new(gchar, response_length - 1);
        memcpy(response_text, response_buffer, response_length - 2);
        response_text[response_length - 2] = 0;
      } else {
        response_text = g_new(gchar, (response_length - 2) * 2 + 1);
        bytes_to_hex(response_text, response_buffer, response_length - 2);
      }
      *response = response_text;
    } else {
      *response = NULL;
    }
    return response_buffer[response_length - 2] << 8 | response_buffer[response_length - 1];
  }
  return -1;
}

static void
_send_apdu (GobredValue *params, GobredMethodCallBack *cb)
{
  gint argc = gobred_array_get_length (params);
  if (argc < 2) {
    gobred_method_callback_throw_error (&cb, "invalid number of arguments");
    return;
  }
  if (gobred_array_get_item_type (params, 0) != GOBRED_VALUE_TYPE_NUMBER) {
    gobred_method_callback_throw_error (&cb, "invalid argument type: args[0] = number");
    return;
  }
  gint reader_index = (gint)gobred_array_get_number (params, 0);
  if (reader_index < 1 || reader_index > num_reader) {
    gobred_method_callback_throw_error (&cb, "invalid reader index %d", reader_index);
    return;
  }
  reader_index--;
  if (_connect_reader (reader_index)) {
    gobred_method_callback_throw_error (&cb, "can not connect to reader %d", reader_index);
    return;
  }
  gboolean use_ascii = FALSE;
  if (argc >= 3)
    use_ascii = gobred_array_get_boolean (params, 2);
  switch (gobred_array_get_item_type (params, 1)) {
  case GOBRED_VALUE_TYPE_STRING:
      {
        const gchar *apdu_string = gobred_array_get_string(params, 1);
        gchar *response;
        gint sw = _send_single_apdu (reader_index, apdu_string, &response, use_ascii);
        GobredValue *result = gobred_array_new (2,
                                                      GOBRED_VALUE_TYPE_NUMBER, (double)sw,
                                                      NULL);
        if (response)
          gobred_array_add(result, gobred_value_new_take_string (response));
        gobred_method_callback_return (&cb, result);
      }
    break;
  case GOBRED_VALUE_TYPE_ARRAY:

  default:
    gobred_method_callback_throw_error (&cb, "invalid argument type: args[2] = ");
  }
}

static GobredMethodDefinitionV0 methods[] = {
  {.name = "send", .handler.simple = _send_apdu, .threaded = TRUE},
  {.name = "getReaders", .handler.simple = _get_readers},
  {.name = "getReader", .handler.simple = _get_reader},
  {NULL}
};

static GobredEventDefinitionV0 events[] = {
  {"card-present"},
  {"card-removed"},
  {NULL}
};

static GobredModuleDefinitionV0 definition = {
  .base.version = 0,
  .base.name = "smartcard",
  .methods = methods,
  .events = events,
  .init = _init,
  .free = _free
};

GobredModuleDefinitionBase *
gobred_module_register ()
{
  return &definition.base;
}
