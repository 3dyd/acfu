#include "stdafx.h"
#include "resource.h"
#include "preferences_dlg.h"

// {B9256E8C-C335-4497-94F6-69E58AB358B7}
static const GUID guid_preferences_page =
{ 0xb9256e8c, 0xc335, 0x4497, { 0x94, 0xf6, 0x69, 0xe5, 0x8a, 0xb3, 0x58, 0xb7 } };

// {0E966267-7DFB-433B-A07C-3F8CDD31A258}
static const GUID guid_components =
{ 0x0E966267, 0x7DFB, 0x433B, { 0xA0, 0x7C, 0x3F, 0x8C, 0xDD, 0x31, 0xA2, 0x58 } };

#if 0
//#ifdef _DEBUG
class PpDebug: public initquit {
  virtual void on_init() {
    static_api_ptr_t<ui_control>()->show_preferences(guid_preferences_page);
  }
};
static service_factory_single_t<PpDebug> g_pp_debug;
#endif

class PreferencesPage: public preferences_page_impl<PreferencesDlg> {
 public:
  virtual GUID get_guid() {
    return guid_preferences_page;
  }

  virtual const char* get_name() {
    return APP_SHORT_NAME;
  }

  virtual GUID get_parent_guid() {
    auto guid = guid_tools;
    acfu::for_each_service<preferences_page>([&guid](auto ptr) {
      if (guid_preferences_page == ptr->get_guid()) {
        return;
      }
      if ((guid_root == ptr->get_parent_guid() && 0 == pfc::stricmp_ascii(ptr->get_name(), "Components")) ||
          guid_components == ptr->get_guid()) {
        guid = ptr->get_guid();
      }
    });
    return guid;
  }
};

static preferences_page_factory_t<PreferencesPage> g_preferences_page;
