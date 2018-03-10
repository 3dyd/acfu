#include "stdafx.h"
#include "urls.h"

DECLARE_COMPONENT_VERSION(
  APP_SHORT_NAME,
  APP_VERSION,
  APP_DESCRIPTION "\n\nRelease date: " __DATE__ "\n3dyd, acfu@3dyd.com"
);

VALIDATE_COMPONENT_FILENAME(APP_BINARY_NAME ".dll");

struct AcfuGithubConf: acfu::github_conf {
  static const char* get_owner() {
    return "3dyd";
  }

  static const char* get_repo() {
    return "acfu";
  }
};

// #ifdef _DEBUG
// #include "preferences_page.h"
// class PpDebug: public initquit {
//   virtual void on_init() {
//     static_api_ptr_t<ui_control>()->show_preferences(guid_preferences_page);
//   }
// };
// static service_factory_single_t<PpDebug> g_pp_debug;
// #endif

class AcfuRequest: public acfu::github_releases<AcfuGithubConf> {
// #ifdef _DEBUG
//   virtual pfc::string8 form_releases_url() {
//     return "http://127.0.0.1:8888/releases.json";
//   }
// #endif

  virtual void process_release(const rapidjson::Value& release, file_info& info) {
    __super::process_release(release, info);
    info.meta_set("download_page", APP_URL_DOWNLOAD);
  }
};

class AcfuSource: public acfu::source {
  virtual GUID get_guid() {
    static const GUID guid =
    { 0xe07b43d1, 0x6050, 0x4b8c, { 0xba, 0x5f, 0xee, 0x6b, 0xcd, 0x44, 0xa2, 0x34 } };

    return guid;
  }

  virtual void get_info(file_info& info) {
    info.meta_set("version", APP_VERSION);
    info.meta_set("name", APP_SHORT_NAME);
    info.meta_set("module", APP_BINARY_NAME);
  }

  virtual bool is_newer(const file_info& info) {
    if (info.meta_exists("version")) {
      const char* version = info.meta_get("version", 0);
      return acfu::compare_versions(version, APP_VERSION, "v") > 0;
    }
    return false;
  }

  virtual acfu::request::ptr create_request() {
    acfu::request::ptr request = new service_impl_t<AcfuRequest>();
    return request;
  }
};

static service_factory_t<AcfuSource> g_acfu_source;
