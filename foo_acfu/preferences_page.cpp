#include "stdafx.h"
#include "resource.h"
#include "preferences_page.h"
#include "preferences_dlg.h"
#include "utils.h"
#include "urls.h"

// {0E966267-7DFB-433B-A07C-3F8CDD31A258}
static const GUID guid_components =
{ 0x0E966267, 0x7DFB, 0x433B, { 0xA0, 0x7C, 0x3F, 0x8C, 0xDD, 0x31, 0xA2, 0x58 } };

class PreferencesPage: public preferences_page_impl<PreferencesDlg> {
 public:
  virtual GUID get_guid() {
    return guid_preferences_page;
  }

  virtual bool get_help_url(pfc::string_base & p_out) {
    p_out = APP_URL_PREFS;
    return true;
  }

  virtual const char* get_name() {
    return APP_SHORT_NAME;
  }

  virtual GUID get_parent_guid() {
    auto guid = guid_tools;
    for_each_service<preferences_page>([&guid](auto ptr) {
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
