// Copyright (c) 2012 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_BROWSER_HOST_IMPL_H_
#define CEF_LIBCEF_BROWSER_BROWSER_HOST_IMPL_H_
#pragma once

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_frame.h"
#include "libcef/browser/frame_host_impl.h"
#include "libcef/browser/javascript_dialog_manager.h"
#include "libcef/browser/menu_creator.h"
#include "libcef/common/response_manager.h"

#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/file_chooser_params.h"

#if defined(USE_AURA)
#include "third_party/WebKit/public/platform/WebCursorInfo.h"
#include "ui/base/cursor/cursor.h"
#endif

namespace content {
struct NativeWebKeyboardEvent;
}

namespace blink {
class WebMouseEvent;
class WebMouseWheelEvent;
class WebInputEvent;
}

namespace net {
class URLRequest;
}

#if defined(USE_AURA)
namespace views {
class Widget;
}
#endif

struct Cef_Request_Params;
struct Cef_Response_Params;
class CefBrowserInfo;
class CefDevToolsFrontend;
struct CefNavigateParams;
class SiteInstance;

// Implementation of CefBrowser.
//
// WebContentsDelegate: Interface for handling WebContents delegations. There is
// a one-to-one relationship between CefBrowserHostImpl and WebContents
// instances.
//
// WebContentsObserver: Interface for observing WebContents notifications and
// IPC messages. There is a one-to-one relationship between WebContents and
// RenderViewHost instances. IPC messages received by the RenderViewHost will be
// forwarded to this WebContentsObserver implementation via WebContents. IPC
// messages sent using CefBrowserHostImpl::Send() will be forwarded to the
// RenderViewHost (after posting to the UI thread if necessary). Use
// WebContentsObserver::routing_id() when sending IPC messages.
//
// NotificationObserver: Interface for observing post-processed notifications.
class CefBrowserHostImpl : public CefBrowserHost,
                           public CefBrowser,
                           public content::WebContentsDelegate,
                           public content::WebContentsObserver,
                           public content::NotificationObserver {
 public:
  // Used for handling the response to command messages.
  class CommandResponseHandler : public virtual CefBase {
   public:
     virtual void OnResponse(const std::string& response) =0;
  };

  virtual ~CefBrowserHostImpl();

  // Create a new CefBrowserHostImpl instance.
  static CefRefPtr<CefBrowserHostImpl> Create(
      const CefWindowInfo& windowInfo,
      CefRefPtr<CefClient> client,
      const CefString& url,
      const CefBrowserSettings& settings,
      CefWindowHandle opener,
      bool is_popup,
      CefRefPtr<CefRequestContext> request_context);

  // Returns the browser associated with the specified RenderViewHost.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForHost(
      const content::RenderViewHost* host);
  // Returns the browser associated with the specified RenderFrameHost.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForHost(
      const content::RenderFrameHost* host);
  // Returns the browser associated with the specified WebContents.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForContents(
      content::WebContents* contents);
  // Returns the browser associated with the specified URLRequest.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForRequest(
      net::URLRequest* request);
  // Returns the browser associated with the specified view routing IDs.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForView(
      int render_process_id, int render_routing_id);
  // Returns the browser associated with the specified frame routing IDs.
  static CefRefPtr<CefBrowserHostImpl> GetBrowserForFrame(
      int render_process_id, int render_routing_id);

  // Returns true if window rendering is disabled in CefWindowInfo.
  static bool IsWindowRenderingDisabled(const CefWindowInfo& info);

  // CefBrowserHost methods.
  virtual CefRefPtr<CefBrowser> GetBrowser() OVERRIDE;
  virtual void CloseBrowser(bool force_close) OVERRIDE;
  virtual void SetFocus(bool enable) OVERRIDE;
  virtual CefWindowHandle GetWindowHandle() OVERRIDE;
  virtual CefWindowHandle GetOpenerWindowHandle() OVERRIDE;
  virtual CefRefPtr<CefClient> GetClient() OVERRIDE;
  virtual CefRefPtr<CefRequestContext> GetRequestContext() OVERRIDE;
  virtual double GetZoomLevel() OVERRIDE;
  virtual void SetZoomLevel(double zoomLevel) OVERRIDE;
  virtual void RunFileDialog(
      FileDialogMode mode,
      const CefString& title,
      const CefString& default_file_name,
      const std::vector<CefString>& accept_types,
      CefRefPtr<CefRunFileDialogCallback> callback) OVERRIDE;
  virtual void StartDownload(const CefString& url) OVERRIDE;
  virtual void Print() OVERRIDE;
  virtual void Find(int identifier, const CefString& searchText,
                    bool forward, bool matchCase, bool findNext) OVERRIDE;
  virtual void StopFinding(bool clearSelection) OVERRIDE;
  virtual void ShowDevTools(const CefWindowInfo& windowInfo,
                            CefRefPtr<CefClient> client,
                            const CefBrowserSettings& settings) OVERRIDE;
  virtual void CloseDevTools() OVERRIDE;
  virtual void SetMouseCursorChangeDisabled(bool disabled) OVERRIDE;
  virtual bool IsMouseCursorChangeDisabled() OVERRIDE;
  virtual bool IsWindowRenderingDisabled() OVERRIDE;
  virtual void WasResized() OVERRIDE;
  virtual void WasHidden(bool hidden) OVERRIDE;
  virtual void NotifyScreenInfoChanged() OVERRIDE;
  virtual void Invalidate(const CefRect& dirtyRect,
                          PaintElementType type) OVERRIDE;
  virtual void SendKeyEvent(const CefKeyEvent& event) OVERRIDE;
  virtual void SendMouseClickEvent(const CefMouseEvent& event,
                                   MouseButtonType type,
                                   bool mouseUp, int clickCount) OVERRIDE;
  virtual void SendMouseMoveEvent(const CefMouseEvent& event,
                                  bool mouseLeave) OVERRIDE;
  virtual void SendMouseWheelEvent(const CefMouseEvent& event,
                                   int deltaX, int deltaY) OVERRIDE;
  virtual void SendFocusEvent(bool setFocus) OVERRIDE;
  virtual void SendCaptureLostEvent() OVERRIDE;
  virtual CefTextInputContext GetNSTextInputContext() OVERRIDE;
  virtual void HandleKeyEventBeforeTextInputClient(CefEventHandle keyEvent)
      OVERRIDE;
  virtual void HandleKeyEventAfterTextInputClient(CefEventHandle keyEvent)
      OVERRIDE;

  // CefBrowser methods.
  virtual CefRefPtr<CefBrowserHost> GetHost() OVERRIDE;
  virtual bool CanGoBack() OVERRIDE;
  virtual void GoBack() OVERRIDE;
  virtual bool CanGoForward() OVERRIDE;
  virtual void GoForward() OVERRIDE;
  virtual bool IsLoading() OVERRIDE;
  virtual void Reload() OVERRIDE;
  virtual void ReloadIgnoreCache() OVERRIDE;
  virtual void StopLoad() OVERRIDE;
  virtual int GetIdentifier() OVERRIDE;
  virtual bool IsSame(CefRefPtr<CefBrowser> that) OVERRIDE;
  virtual bool IsPopup() OVERRIDE;
  virtual bool HasDocument() OVERRIDE;
  virtual CefRefPtr<CefFrame> GetMainFrame() OVERRIDE;
  virtual CefRefPtr<CefFrame> GetFocusedFrame() OVERRIDE;
  virtual CefRefPtr<CefFrame> GetFrame(int64 identifier) OVERRIDE;
  virtual CefRefPtr<CefFrame> GetFrame(const CefString& name) OVERRIDE;
  virtual size_t GetFrameCount() OVERRIDE;
  virtual void GetFrameIdentifiers(std::vector<int64>& identifiers) OVERRIDE;
  virtual void GetFrameNames(std::vector<CefString>& names) OVERRIDE;
  virtual bool SendProcessMessage(
      CefProcessId target_process,
      CefRefPtr<CefProcessMessage> message) OVERRIDE;

  // Called when the OS window hosting the browser is destroyed.
  void WindowDestroyed();

  // Destroy the browser members. This method should only be called after the
  // native browser window is not longer processing messages.
  void DestroyBrowser();

  // Returns the native view for the WebContents.
  gfx::NativeView GetContentView() const;

  // Returns a pointer to the WebContents.
  content::WebContents* GetWebContents() const;

  // Returns the frame associated with the specified URLRequest.
  CefRefPtr<CefFrame> GetFrameForRequest(net::URLRequest* request);

  // Navigate as specified by the |params| argument.
  void Navigate(const CefNavigateParams& params);

  // Load the specified request.
  void LoadRequest(int64 frame_id, CefRefPtr<CefRequest> request);

  // Load the specified URL.
  void LoadURL(int64 frame_id,
               const std::string& url,
               const content::Referrer& referrer,
               content::PageTransition transition,
               const std::string& extra_headers);

  // Load the specified string.
  void LoadString(int64 frame_id, const std::string& string,
                  const std::string& url);

  // Send a command to the renderer for execution.
  void SendCommand(int64 frame_id, const std::string& command,
                   CefRefPtr<CefResponseManager::Handler> responseHandler);

  // Send code to the renderer for execution.
  void SendCode(int64 frame_id, bool is_javascript, const std::string& code,
                const std::string& script_url, int script_start_line,
                CefRefPtr<CefResponseManager::Handler> responseHandler);

  bool SendProcessMessage(CefProcessId target_process,
                          const std::string& name,
                          base::ListValue* arguments,
                          bool user_initiated);

  // Open the specified text in the default text editor.
  bool ViewText(const std::string& text);

  // Handler for URLs involving external protocols.
  void HandleExternalProtocol(const GURL& url);

  // Thread safe accessors.
  const CefBrowserSettings& settings() const { return settings_; }
  CefRefPtr<CefClient> client() const { return client_; }
  int browser_id() const;

  // Returns the URL that is currently loading (or loaded) in the main frame.
  GURL GetLoadingURL();

  bool IsTransparent();

#if defined(OS_WIN)
  static void RegisterWindowClass();
#endif

#if defined(USE_AURA)
  ui::PlatformCursor GetPlatformCursor(blink::WebCursorInfo::Type type);
#endif

  void OnSetFocus(cef_focus_source_t source);

  // The argument vector will be empty if the dialog was cancelled.
  typedef base::Callback<void(const std::vector<base::FilePath>&)>
      RunFileChooserCallback;

  // Run the file chooser dialog specified by |params|. Only a single dialog may
  // be pending at any given time. |callback| will be executed asynchronously
  // after the dialog is dismissed or if another dialog is already pending.
  void RunFileChooser(const content::FileChooserParams& params,
                      const RunFileChooserCallback& callback);

  // Used when creating a new popup window.
  struct PendingPopupInfo {
    CefWindowInfo window_info;
    CefBrowserSettings settings;
    CefRefPtr<CefClient> client;
  };
  // Returns false if a popup is already pending.
  bool SetPendingPopupInfo(scoped_ptr<PendingPopupInfo> info);

  enum DestructionState {
    DESTRUCTION_STATE_NONE = 0,
    DESTRUCTION_STATE_PENDING,
    DESTRUCTION_STATE_ACCEPTED,
    DESTRUCTION_STATE_COMPLETED
  };
  DestructionState destruction_state() const { return destruction_state_; }

 private:
  class DevToolsWebContentsObserver;

  static CefRefPtr<CefBrowserHostImpl> CreateInternal(
      const CefWindowInfo& window_info,
      const CefBrowserSettings& settings,
      CefRefPtr<CefClient> client,
      content::WebContents* web_contents,
      scoped_refptr<CefBrowserInfo> browser_info,
      CefWindowHandle opener,
      CefRefPtr<CefRequestContext> request_context);

  // content::WebContentsDelegate methods.
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;
  virtual void LoadingStateChanged(content::WebContents* source) OVERRIDE;
  virtual void CloseContents(content::WebContents* source) OVERRIDE;
  virtual void UpdateTargetURL(content::WebContents* source,
                               int32 page_id,
                               const GURL& url) OVERRIDE;
  virtual bool AddMessageToConsole(content::WebContents* source,
                                   int32 level,
                                   const base::string16& message,
                                   int32 line_no,
                                   const base::string16& source_id) OVERRIDE;
  virtual void BeforeUnloadFired(content::WebContents* source,
                                 bool proceed,
                                 bool* proceed_to_fire_unload) OVERRIDE;
  virtual bool TakeFocus(content::WebContents* source,
                         bool reverse) OVERRIDE;
  virtual void WebContentsFocused(content::WebContents* contents) OVERRIDE;
  virtual bool HandleContextMenu(
      content::RenderFrameHost* render_frame_host,
      const content::ContextMenuParams& params) OVERRIDE;
  virtual bool PreHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event,
      bool* is_keyboard_shortcut) OVERRIDE;
  virtual void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) OVERRIDE;
  virtual bool CanDragEnter(
      content::WebContents* source,
      const content::DropData& data,
      blink::WebDragOperationsMask operations_allowed) OVERRIDE;
  virtual bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int route_id,
      WindowContainerType window_container_type,
      const base::string16& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) OVERRIDE;
  virtual void WebContentsCreated(content::WebContents* source_contents,
                                  int64 source_frame_id,
                                  const base::string16& frame_name,
                                  const GURL& target_url,
                                  content::WebContents* new_contents) OVERRIDE;
  virtual void DidNavigateMainFramePostCommit(
      content::WebContents* tab) OVERRIDE;
  virtual content::JavaScriptDialogManager* GetJavaScriptDialogManager()
      OVERRIDE;
  virtual void RunFileChooser(
      content::WebContents* tab,
      const content::FileChooserParams& params) OVERRIDE;
  virtual void UpdatePreferredSize(content::WebContents* source,
                                   const gfx::Size& pref_size) OVERRIDE;
  virtual void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) OVERRIDE;

  // content::WebContentsObserver methods.
  using content::WebContentsObserver::BeforeUnloadFired;
  using content::WebContentsObserver::WasHidden;
  virtual void RenderFrameCreated(
      content::RenderFrameHost* render_frame_host) OVERRIDE;
  virtual void RenderFrameDeleted(
      content::RenderFrameHost* render_frame_host) OVERRIDE;
  virtual void RenderViewCreated(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void RenderViewDeleted(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void RenderViewReady() OVERRIDE;
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;
  virtual void DidCommitProvisionalLoadForFrame(
      int64 frame_id,
      const base::string16& frame_unique_name,
      bool is_main_frame,
      const GURL& url,
      content::PageTransition transition_type,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidFailProvisionalLoad(
      int64 frame_id,
      const base::string16& frame_unique_name,
      bool is_main_frame,
      const GURL& validated_url,
      int error_code,
      const base::string16& error_description,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DocumentAvailableInMainFrame() OVERRIDE;
  virtual void DidFailLoad(int64 frame_id,
                           const GURL& validated_url,
                           bool is_main_frame,
                           int error_code,
                           const base::string16& error_description,
                           content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void PluginCrashed(const base::FilePath& plugin_path,
                             base::ProcessId plugin_pid) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  // Override to provide a thread safe implementation.
  virtual bool Send(IPC::Message* message) OVERRIDE;

  // content::WebContentsObserver::OnMessageReceived() message handlers.
  void OnFrameIdentified(int64 frame_id,
                         int64 parent_frame_id,
                         base::string16 name);
  void OnDidFinishLoad(
      int64 frame_id,
      const GURL& validated_url,
      bool is_main_frame,
      int http_status_code);
  void OnLoadingURLChange(const GURL& pending_url);
  void OnRequest(const Cef_Request_Params& params);
  void OnResponse(const Cef_Response_Params& params);
  void OnResponseAck(int request_id);

  // content::NotificationObserver methods.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  CefBrowserHostImpl(const CefWindowInfo& window_info,
                     const CefBrowserSettings& settings,
                     CefRefPtr<CefClient> client,
                     content::WebContents* web_contents,
                     scoped_refptr<CefBrowserInfo> browser_info,
                     CefWindowHandle opener);

  // Updates and returns an existing frame or creates a new frame. Pass
  // CefFrameHostImpl::kUnspecifiedFrameId for |parent_frame_id| if unknown.
  CefRefPtr<CefFrame> GetOrCreateFrame(int64 frame_id,
                                       int64 parent_frame_id,
                                       bool is_main_frame,
                                       base::string16 frame_name,
                                       const GURL& frame_url);
  // Remove the reference to the frame and mark it as detached.
  void DetachFrame(int64 frame_id);
  // Remove the references to all frames and mark them as detached.
  void DetachAllFrames();
  // Set the frame that currently has focus.
  void SetFocusedFrame(int64 frame_id);

#if defined(OS_WIN)
  static LPCTSTR GetWndClass();
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
                                  WPARAM wParam, LPARAM lParam);
#endif

  // Create the window.
  bool PlatformCreateWindow();
  // Sends a message via the OS to close the native browser window.
  // DestroyBrowser will be called after the native window has closed.
  void PlatformCloseWindow();
  // Resize the window to the given dimensions.
  void PlatformSizeTo(int width, int height);
  // Return the handle for this window.
  CefWindowHandle PlatformGetWindowHandle();
  // Open the specified text in the default text editor.
  bool PlatformViewText(const std::string& text);
  // Forward the keyboard event to the application or frame window to allow
  // processing of shortcut keys.
  void PlatformHandleKeyboardEvent(
      const content::NativeWebKeyboardEvent& event);
  // Invoke platform specific handling for the external protocol.
  void PlatformHandleExternalProtocol(const GURL& url);
  // Invoke platform specific file chooser dialog.
  void PlatformRunFileChooser(const content::FileChooserParams& params,
                              RunFileChooserCallback callback);

  void PlatformTranslateKeyEvent(content::NativeWebKeyboardEvent& native_event,
                                 const CefKeyEvent& key_event);
  void PlatformTranslateClickEvent(blink::WebMouseEvent& web_event,
                                   const CefMouseEvent& mouse_event,
                                   CefBrowserHost::MouseButtonType type,
                                   bool mouseUp, int clickCount);
  void PlatformTranslateMoveEvent(blink::WebMouseEvent& web_event,
                                  const CefMouseEvent& mouse_event,
                                  bool mouseLeave);
  void PlatformTranslateWheelEvent(blink::WebMouseWheelEvent& web_event,
                                   const CefMouseEvent& mouse_event,
                                   int deltaX, int deltaY);
  void PlatformTranslateMouseEvent(blink::WebMouseEvent& web_event,
                                  const CefMouseEvent& mouse_event);

  int TranslateModifiers(uint32 cefKeyStates);
  void SendMouseEvent(const blink::WebMouseEvent& web_event);

  void OnAddressChange(CefRefPtr<CefFrame> frame,
                       const GURL& url);
  void OnLoadStart(CefRefPtr<CefFrame> frame,
                   const GURL& url,
                   content::PageTransition transition_type);
  void OnLoadError(CefRefPtr<CefFrame> frame,
                   const GURL& url,
                   int error_code,
                   const base::string16& error_description);
  void OnLoadEnd(CefRefPtr<CefFrame> frame,
                 const GURL& url,
                 int http_status_code);

  // Continuation from RunFileChooser.
  void RunFileChooserOnUIThread(const content::FileChooserParams& params,
                                const RunFileChooserCallback& callback);

  // Used with RunFileChooser to clear the |file_chooser_pending_| flag.
  void OnRunFileChooserCallback(const RunFileChooserCallback& callback,
                                const std::vector<base::FilePath>& file_paths);

  // Used with WebContentsDelegate::RunFileChooser to notify the WebContents.
  void OnRunFileChooserDelegateCallback(
      content::WebContents* tab,
      content::FileChooserParams::Mode mode,
      const std::vector<base::FilePath>& file_paths);

  void OnDevToolsWebContentsDestroyed();

  CefWindowInfo window_info_;
  CefBrowserSettings settings_;
  CefRefPtr<CefClient> client_;
  scoped_ptr<content::WebContents> web_contents_;
  scoped_refptr<CefBrowserInfo> browser_info_;
  CefWindowHandle opener_;
  CefRefPtr<CefRequestContext> request_context_;

  // Pending popup information. Access must be protected by
  // |pending_popup_info_lock_|.
  base::Lock pending_popup_info_lock_;
  scoped_ptr<PendingPopupInfo> pending_popup_info_;

  // Volatile state information. All access must be protected by the state lock.
  base::Lock state_lock_;
  bool is_loading_;
  bool can_go_back_;
  bool can_go_forward_;
  bool has_document_;
  GURL loading_url_;

  // Messages we queue while waiting for the RenderView to be ready. We queue
  // them here instead of in the RenderProcessHost to ensure that they're sent
  // after the CefRenderViewObserver has been created on the renderer side.
  std::queue<IPC::Message*> queued_messages_;
  bool queue_messages_;

  // Map of unique frame ids to CefFrameHostImpl references.
  typedef std::map<int64, CefRefPtr<CefFrameHostImpl> > FrameMap;
  FrameMap frames_;
  // The unique frame id currently identified as the main frame.
  int64 main_frame_id_;
  // The unique frame id currently identified as the focused frame.
  int64 focused_frame_id_;
  // Used when no other frame exists. Provides limited functionality.
  CefRefPtr<CefFrameHostImpl> placeholder_frame_;

  // Represents the current browser destruction state. Only accessed on the UI
  // thread.
  DestructionState destruction_state_;

  // True if the OS window hosting the browser has been destroyed. Only accessed
  // on the UI thread.
  bool window_destroyed_;

  // True if currently in the OnSetFocus callback. Only accessed on the UI
  // thread.
  bool is_in_onsetfocus_;

  // True if the focus is currently on an editable field on the page. Only
  // accessed on the UI thread.
  bool focus_on_editable_field_;

  // True if mouse cursor change is disabled.
  bool mouse_cursor_change_disabled_;

  // Used for managing notification subscriptions.
  scoped_ptr<content::NotificationRegistrar> registrar_;

  // Manages response registrations.
  scoped_ptr<CefResponseManager> response_manager_;

  // Used for creating and managing JavaScript dialogs.
  scoped_ptr<CefJavaScriptDialogManager> dialog_manager_;

  // Used for creating and managing context menus.
  scoped_ptr<CefMenuCreator> menu_creator_;

  // Track the lifespan of the frontend WebContents associated with this
  // browser.
  scoped_ptr<DevToolsWebContentsObserver> devtools_observer_;
  // CefDevToolsFrontend will delete itself when the frontend WebContents is
  // destroyed.
  CefDevToolsFrontend* devtools_frontend_;

  // True if a file chooser is currently pending.
  bool file_chooser_pending_;

  // Current title for the main frame. Only accessed on the UI thread.
  base::string16 title_;

#if defined(USE_AURA)
  // Widget hosting the web contents. It will be deleted automatically when the
  // associated root window is destroyed.
  views::Widget* window_widget_;
#endif  // defined(USE_AURA)

  IMPLEMENT_REFCOUNTING(CefBrowserHostImpl);
  DISALLOW_EVIL_CONSTRUCTORS(CefBrowserHostImpl);
};

#endif  // CEF_LIBCEF_BROWSER_BROWSER_HOST_IMPL_H_
