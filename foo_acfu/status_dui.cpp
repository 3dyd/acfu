#include "stdafx.h"
#include "resource.h"
#include "status_wnd.h"

class StatusDuiImpl: public StatusWnd, public ui_element_instance {
 public:
  BEGIN_MSG_MAP_EX(StatusDui)
    MSG_WM_STYLECHANGING(OnStyleChanging)
    CHAIN_MSG_MAP(StatusWnd)
  END_MSG_MAP()

  StatusDuiImpl(ui_element_config::ptr config, ui_element_instance_callback_ptr callback): callback_(callback) {
    set_configuration(config);
  }

  static ui_element_config::ptr g_get_default_configuration() {
    ui_element_config_builder builder;
    return builder.finish(g_get_guid());
  }

  static const char* g_get_description() {
    return APP_DESCRIPTION;
  }

  static GUID g_get_guid() {
    // {3C3C69FE-AF9C-44B5-A6F3-DCC2AC14A32A}
    static const GUID guid =
    { 0x3c3c69fe, 0xaf9c, 0x44b5, { 0xa6, 0xf3, 0xdc, 0xc2, 0xac, 0x14, 0xa3, 0x2a } };

    return guid;
  }

  static void g_get_name(pfc::string_base& out) {
    out = APP_SHORT_NAME;
  }

  static GUID g_get_subclass() {
    return ui_element_subclass_utility;
  }

  void initialize_window(HWND parent) {
    WIN32_OP(Create(parent) != NULL);
  }

 private:
  virtual ui_element_config::ptr get_configuration() {
    ui_element_config_builder builder;
    return builder.finish(get_guid());
  }

  virtual HWND get_wnd() {
    return m_hWnd;
  }

  virtual void notify(const GUID& what, t_size p_param1, const void * p_param2, t_size p_param2size) {
    if (ui_element_notify_colors_changed == what || ui_element_notify_font_changed == what) {
      OnUiChanged();
    }
  }

  virtual void set_configuration(ui_element_config::ptr config) {}

 private:
  void OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct) {
    if (GWL_EXSTYLE == nStyleType) {
      lpStyleStruct->styleNew &= ~WS_EX_STATICEDGE;
    }
    SetMsgHandled(FALSE);
  }

  virtual COLORREF GetUiColor(Color color) const {
    switch (color) {
      case kColorBackground:  return callback_->query_std_color(ui_color_background);
      case kColorHighlight:   return callback_->query_std_color(ui_color_highlight);
    }
    ATLASSERT(0);
    return 0;
  }

  virtual CFontHandle GetUiFont() const {
    return callback_->query_font_ex(ui_font_lists);
  }

 private:
  const ui_element_instance_callback_ptr callback_;
};

class StatusDui: public ui_element_impl<StatusDuiImpl, ui_element> {};

static service_factory_single_t<StatusDui> g_status_dui;
