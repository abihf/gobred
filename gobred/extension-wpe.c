#include <WPE/WebKit.h>
#include <glib.h>
#include <libgobred/gobred-bridge.h>


#include <dlfcn.h>

static void
didClearWindowObjectForFrame (WKBundlePageRef page, 
                              WKBundleFrameRef frame, 
                              WKBundleScriptWorldRef scriptWorld, 
                              const void *data) 
{
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, scriptWorld);
    gobred_bridge_setup (context);
}

static WKBundlePageLoaderClientV6 s_pageLoaderClient = {
    { 6, NULL },
    NULL, // didStartProvisionalLoadForFrame
    NULL, // didReceiveServerRedirectForProvisionalLoadForFrame
    NULL, // didFailProvisionalLoadWithErrorForFrame
    NULL, // didCommitLoadForFrame
    NULL, // didFinishDocumentLoadForFrame
    NULL, // didFinishLoadForFrame
    NULL, // didFailLoadWithErrorForFrame
    NULL, // didSameDocumentNavigationForFrame
    NULL, // didReceiveTitleForFrame
    NULL, // didFirstLayoutForFrame
    NULL, // didFirstVisuallyNonEmptyLayoutForFrame
    NULL, // didRemoveFrameFromHierarchy
    NULL, // didDisplayInsecureContentForFrame
    NULL, // didRunInsecureContentForFrame
    // didClearWindowObjectForFrame
    didClearWindowObjectForFrame,
    NULL, // didCancelClientRedirectForFrame
    NULL, // willPerformClientRedirectForFrame
    NULL, // didHandleOnloadEventsForFrame
    NULL, // didLayoutForFrame
    NULL, // didNewFirstVisuallyNonEmptyLayout_unavailable
    NULL, // didDetectXSSForFrame
    NULL, // shouldGoToBackForwardListItem
    NULL, // globalObjectIsAvailableForFrame
    NULL, // willDisconnectDOMWindowExtensionFromGlobalObject
    NULL, // didReconnectDOMWindowExtensionToGlobalObject
    NULL, // willDestroyGlobalObjectForDOMWindowExtension
    NULL, // didFinishProgress
    NULL, // shouldForceUniversalAccessFromLocalURL
    NULL, // didReceiveIntentForFrame_unavailable
    NULL, // registerIntentServiceForFrame_unavailable
    NULL, // didLayout
    NULL, // featuresUsedInPage
    NULL, // willLoadURLRequest
    NULL, // willLoadDataRequest
};

static void 
didCreatePage (WKBundleRef bundle, WKBundlePageRef page, const void *data)
{
    WKBundlePageSetPageLoaderClient(page, &s_pageLoaderClient.base);
}

static WKBundleClientV1 s_bundleClient = {
    { 1, NULL },
    // didCreatePage
    didCreatePage,
    NULL, // willDestroyPage
    NULL, // didInitializePageGroup
    NULL, // didReceiveMessage
    NULL, // didReceiveMessageToPage
};

void WKBundleInitialize(WKBundleRef bundle, WKTypeRef type)
{
    gobred_bridge_init ();
    dlopen ("libgobred-wpe.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
    //fprintf(stderr, "[WPEInjectedBundle] Initialized.\n");
    WKBundleSetClient(bundle, &s_bundleClient.base);
}
