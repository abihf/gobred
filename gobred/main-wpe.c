#include <WPE/WebKit.h>
#include <WPE/WebKit/WKCookieManagerSoup.h>

#include <glib.h>

static GMainLoop* loop;

static void
decidePolicyForNavigationResponse (WKPageRef page, WKNavigationResponseRef res, WKFramePolicyListenerRef listener, WKTypeRef type, const void* data) {
	WKFramePolicyListenerUse(listener);
}

static void
decidePolicyForNavigationAction (WKPageRef page, WKNavigationActionRef res, WKFramePolicyListenerRef listener, WKTypeRef type, const void* data) {
	WKFramePolicyListenerUse(listener);
}


static void
webProcessDidCrash (WKPageRef oldPage, const void* clientInfo)
{
    g_warning("Web process crash!\n");
    g_main_loop_quit (loop);
}


static WKPageNavigationClientV0 s_navigationClient = {
    { 0, NULL },
    decidePolicyForNavigationAction, // decidePolicyForNavigationAction
    decidePolicyForNavigationResponse, // decidePolicyForNavigationResponse
    NULL, // decidePolicyForPluginLoad
    NULL, // didStartProvisionalNavigation
    NULL, // didReceiveServerRedirectForProvisionalNavigation
    NULL, // didFailProvisionalNavigation
    NULL, // didCommitNavigation
    NULL, // didFinishNavigation
    NULL, // didFailNavigation
    NULL, // didFailProvisionalLoadInSubframe
    NULL, // didFinishDocumentLoad
    NULL, // didSameDocumentNavigation
    NULL, // renderingProgressDidChange
    NULL, // canAuthenticateAgainstProtectionSpace
    NULL, // didReceiveAuthenticationChallenge
    webProcessDidCrash, // webProcessDidCrash
    NULL, // copyWebCryptoMasterKey
    NULL, // didBeginNavigationGesture
    NULL, // willEndNavigationGesture
    NULL, // didEndNavigationGesture
    NULL, // didRemoveNavigationGestureSnapshot
};


int main(int argc, char* argv[])
{
    loop = g_main_loop_new(NULL, FALSE);

    WKContextConfigurationRef contextConfiguration = WKContextConfigurationCreate();
    WKStringRef injectedBundlePath = WKStringCreateWithUTF8CString(PACKAGE_LIB_DIR "/WPEInjectedBundle.so");
    WKContextConfigurationSetInjectedBundlePath(contextConfiguration, injectedBundlePath);
    WKRelease(injectedBundlePath);

    gchar *storagePath = g_build_filename(g_get_user_data_dir(), "gobred/local-storage", NULL);
    g_mkdir_with_parents (storagePath, 0700);
    WKStringRef storagePathWK = WKStringCreateWithUTF8CString(storagePath);
    g_free(storagePath);
    WKContextConfigurationSetLocalStorageDirectory(contextConfiguration, storagePathWK);
    WKRelease(storagePathWK);

    gchar *dbPath = g_build_filename(g_get_user_data_dir(), "gobred/database", NULL);
    g_mkdir_with_parents (dbPath, 0700);
    WKStringRef dbPathWK = WKStringCreateWithUTF8CString(dbPath);
    g_free(dbPath);
    WKContextConfigurationSetIndexedDBDatabaseDirectory(contextConfiguration, dbPathWK);
    WKRelease(dbPathWK);

    WKContextRef context = WKContextCreateWithConfiguration(contextConfiguration);
    WKRelease(contextConfiguration);

    WKStringRef pageGroupIdentifier = WKStringCreateWithUTF8CString("GobredPageGroup");
    WKPageGroupRef pageGroup = WKPageGroupCreateWithIdentifier(pageGroupIdentifier);
    WKRelease(pageGroupIdentifier);

    WKPreferencesRef preferences = WKPreferencesCreate();
    // Allow mixed content.
    WKPreferencesSetAllowRunningOfInsecureContent(preferences, true);
    WKPreferencesSetAllowDisplayOfInsecureContent(preferences, true);
    if (!g_getenv("WPE_SHELL_DISABLE_CONSOLE_LOG"))
      WKPreferencesSetLogsPageMessagesToSystemConsoleEnabled(preferences, true);
    WKPageGroupSetPreferences(pageGroup, preferences);

    WKPageConfigurationRef pageConfiguration  = WKPageConfigurationCreate();
    WKPageConfigurationSetContext(pageConfiguration, context);
    WKPageConfigurationSetPageGroup(pageConfiguration, pageGroup);

    if (!!g_getenv("WPE_SHELL_COOKIE_STORAGE")) {
      gchar *cookieDatabasePath = g_build_filename(g_get_user_data_dir(), "gobred/cookies.db", NULL);
      WKStringRef path = WKStringCreateWithUTF8CString(cookieDatabasePath);
      g_free(cookieDatabasePath);
      WKCookieManagerRef cookieManager = WKContextGetCookieManager(context);
      WKCookieManagerSetCookiePersistentStorage(cookieManager, path, kWKCookieStorageTypeSQLite);
    }

    WKViewRef view = WKViewCreate(pageConfiguration);
    WKPageRef page = WKViewGetPage(view);
    WKPageSetPageNavigationClient(page, &s_navigationClient.base);

    const char* url = "http://youtube.com/tv";
    if (argc > 1)
        url = argv[1];

    WKURLRef shellURL = WKURLCreateWithUTF8CString(url);
    WKPageLoadURL(page, shellURL);
    WKRelease(shellURL);
    
    g_main_loop_run(loop);

    WKRelease(view);
    WKRelease(pageConfiguration);
    WKRelease(pageGroup);
    WKRelease(context);
    WKRelease(preferences);
    g_main_loop_unref(loop);
    return 0;
}
