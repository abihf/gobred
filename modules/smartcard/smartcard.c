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
  SCardReleaseContext(context_main);
  if (readers) g_free (readers);
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
hex_to_bytes (guint8 *bytes, gchar *hex)
{
  char *h;
  guint8 *b;
  for (h = hex, b = bytes; *h; h+=2, b++) {
    *b = __h2b(h[0]) << 4 | __h2b(h[1]);
  }
}

static void
bytes_to_hex (gchar *hex, guint8 *bytes, gsize size)
{
  static gchar chex[] = "0123456789abcdef";
  int i;
  char *h = hex;
  guint8 *b = bytes;
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
          GobredValue *param = gobred_value_new_dict(
             "reader", GOBRED_VALUE_TYPE_NUMBER, (double)(i+1),
             "atr", GOBRED_VALUE_TYPE_STRING, atr_hex,
             0);
          g_print("card present %d %s\n", i, atr_hex);
          gobred_event_emit (EVENT_CARD_PRESENT, param);
          g_free (atr_hex);

					//cb(i, TRUE, g_bytes_new(readers[i].atr, readers[i].atr_len), data);
		    } else if (reader_states[i].dwEventState & SCARD_STATE_EMPTY) {
					if (readers[i].connected) {
						SCardDisconnect(readers[i].card, SCARD_LEAVE_CARD);
						readers[i].connected = FALSE;
					}
		      readers[i].card_present = FALSE;
          g_print("card removed %d\n", i);
					// cb(i, FALSE, NULL, data);
		    } else {
		      g_print("Unknown state %s: %lu\n", reader_states[i].szReader, reader_states[i].dwEventState);
		    }
		    reader_states[i].dwCurrentState = reader_states[i].dwEventState;
		  }
		}
	}
  g_free (reader_states);
  SCardReleaseContext(context);
  g_print ("kenapa udah? %lx\n", rv);
  return NULL;
}

static void
_get_readers (GobredValue *params, GobredMethodCallBack *cb)
{
  GobredValue *result = gobred_value_new_array (num_reader, 0);
  for (int i=0; i<num_reader; i++) {
    gobred_value_add_item (result, gobred_value_new_string(readers[i].name));
  }
  gobred_method_callback_return (&cb, result);
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

static gchar *
_send_single_apdu (gint index, const gchar *apdu)
{
  return NULL;
}

static void
_send_apdu (GobredValue *params, GobredMethodCallBack *cb)
{
  gint reader_index = (gint)gobred_value_get_number_item (params, 0);
  if (reader_index < 1 || reader_index > num_reader) {
    gobred_method_callback_throw_error (&cb, "Invalid index %d", reader_index);
    return;
  }
  reader_index--;
  if (_connect_reader (reader_index)) {
    gobred_method_callback_throw_error (&cb, "Cannot connect to reader: %d", reader_index);
    return;
  }
  switch (gobred_value_get_item_type (params, 1)) {
  case GOBRED_VALUE_TYPE_STRING:
      {
        const gchar *apdu_string = gobred_value_get_string_item(params, 1);
        gchar *respond = _send_single_apdu (reader_index, apdu_string);
        GobredValue *result = gobred_value_new_take_string (respond);
        gobred_method_callback_return (&cb, result);
      }
    break;
  default:
    gobred_method_callback_throw_error (&cb, "");
  }

}

void smartcard_hello()
{

}

static GobredMethodDefinition methods[] = {
  {.name = "send", .handler.simple = _send_apdu, .threaded = TRUE},
  {.name = "getReaders", .handler.simple = _get_readers},
  {NULL}
};

static GobredEventDefinition events[] = {
  {"card-present"},
  {"card-removed"},
  {NULL}
};

static GobredModuleDefinition definition = {
  .name = "smartcard",
  .methods = methods,
  .events = events,
  .init = _init,
  .free = _free
};

GobredModuleDefinition *
gobred_module_register ()
{
  return &definition;
}
