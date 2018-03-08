#include "stdafx.h"
#include "resource.h"
#include "../columns_ui-sdk/ui_extension.h"
#include "status_wnd.h"
#include "version.h"

using namespace uie;

class ResourceClient {
 public:
  void OnUiChanged() const {
    core_api::ensure_main_thread();

    for (size_t i = 0; i < windows_.get_size(); i ++) {
      windows_[i]->OnUiChanged();
    }
  }

  void RegisterWnd(StatusWnd* wnd) {
    core_api::ensure_main_thread();
    windows_.add_item(wnd);
  }

  void UnregisterWnd(StatusWnd* wnd) {
    core_api::ensure_main_thread();
    windows_.remove_item(wnd);
  }

 private:
  pfc::list_t<StatusWnd*> windows_;
};

class FontsClient: public cui::fonts::client, public ResourceClient {
 public:
  CFontHandle GetFont() {
    if (!font_) {
      cui::fonts::helper helper(get_client_guid());
      font_.Attach(helper.get_font());
    }
    return font_.m_hFont;
  }

 private:
  virtual const GUID& get_client_guid() const {
    // {7F2E0FAD-A9BE-40C8-8DA5-BE4EB8027C90}
    static const GUID guid = 
    { 0x7f2e0fad, 0xa9be, 0x40c8, { 0x8d, 0xa5, 0xbe, 0x4e, 0xb8, 0x2, 0x7c, 0x90 } };

    return guid;
  }

  virtual void get_name(pfc::string_base & p_out) const {
    p_out = APP_SHORT_NAME;
  }

  virtual cui::fonts::font_type_t get_default_font_type() const {
    return cui::fonts::font_type_labels;
  }

  virtual void on_font_changed() const {
    if (font_) {
      const_cast<CFont&>(font_).DeleteObject();
    }
    OnUiChanged();
  }

 private:
  CFont font_;
};

static service_factory_single_t<FontsClient> g_fonts;

class ColorsClient: public cui::colours::client, public ResourceClient {
 public:
  COLORREF GetColor(StatusWnd::Color color) {
    cui::colours::helper helper(get_client_guid());
    switch (color) {
      case StatusWnd::kColorBackground:  return helper.get_colour(cui::colours::colour_background);
      case StatusWnd::kColorHighlight:   return helper.get_colour(cui::colours::colour_selection_background);
    }
    ATLASSERT(0);
    return 0;
  }

 private:
  virtual const GUID& get_client_guid() const {
    // {7F2E0FAD-A9BE-40C8-8DA5-BE4EB8027C90}
    static const GUID guid = 
    { 0x7f2e0fad, 0xa9be, 0x40c8, { 0x8d, 0xa5, 0xbe, 0x4e, 0xb8, 0x2, 0x7c, 0x90 } };

    return guid;
  }

  virtual void get_name(pfc::string_base & p_out) const { p_out = APP_SHORT_NAME; }
  virtual t_size get_supported_colours() const { return cui::colours::colour_flag_background | cui::colours::colour_flag_selection_background; }
  virtual t_size get_supported_bools() const { return 0; }
  virtual bool get_themes_supported() const { return false; }
  virtual void on_colour_changed(t_size mask) const { OnUiChanged(); }
  virtual void on_bool_changed(t_size mask) const {}

 private:
  pfc::list_t<StatusWnd*> windows_;
};

static service_factory_single_t<ColorsClient> g_colors;

class StatusCui: public StatusWnd, public window {
 public:
  BEGIN_MSG_MAP_EX(StatusCui)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    CHAIN_MSG_MAP(StatusWnd)
  END_MSG_MAP()

 private:
  HWND create_or_transfer_window(HWND wnd_parent, const window_host_ptr& p_host, const ui_helpers::window_position_t& p_position) {
    if (NULL != m_hWnd) {
      ShowWindow(SW_HIDE);
      SetParent(wnd_parent);
      host_->relinquish_ownership(m_hWnd);
      SetWindowPos(0, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
    }
    else {
      CRect rect;
      p_position.convert_to_rect(rect);
      Create(wnd_parent, rect, TEXT(APP_SHORT_NAME));
    }
    host_ = p_host;

    return m_hWnd;
  }

  virtual void destroy_window() {
    DestroyWindow();
    host_.release();
  }

  virtual void get_category(pfc::string_base& out) const {
    out = "Panels";
  }

  virtual const GUID& get_extension_guid() const {
    // {15DCAFBB-757B-41F8-8481-1B0E13A3CE27}
    static const GUID guid =
    { 0x15dcafbb, 0x757b, 0x41f8, { 0x84, 0x81, 0x1b, 0xe, 0x13, 0xa3, 0xce, 0x27 } };

    return guid;
  }

  virtual void get_name(pfc::string_base& out) const {
    out = APP_SHORT_NAME;
  }

  virtual unsigned get_type() const {
    return type_panel | type_toolbar;
  }

  virtual HWND get_wnd() const {
    return m_hWnd;
  }

  virtual bool is_available(const window_host_ptr& p) const {
    return true;
  }

  int OnCreate(LPCREATESTRUCT lpCreateStruct) {
    g_colors.get_static_instance().RegisterWnd(this);
    g_fonts.get_static_instance().RegisterWnd(this);

    SetMsgHandled(FALSE);
    return 0;
  }

  void OnDestroy() {
    g_colors.get_static_instance().UnregisterWnd(this);
    g_fonts.get_static_instance().UnregisterWnd(this);

    SetMsgHandled(FALSE);
  }

  BOOL OnEraseBkgnd(CDCHandle dc) {
    RelayEraseBkgnd(m_hWnd, GetParent(), dc);
    return TRUE;
  }

 private:
  LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
    return WindowProc(wnd, msg, wp, lp);
  }

  virtual COLORREF GetUiColor(Color color) const {
    return g_colors.get_static_instance().GetColor(color);
  }

  virtual CFontHandle GetUiFont() const {
    return g_fonts.get_static_instance().GetFont();
  }

 private:
  window_host_ptr host_;
};

window_factory<StatusCui> g_status_cui;
