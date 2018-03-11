#include "stdafx.h"
#include "urls.h"

DECLARE_COMPONENT_VERSION(
  APP_SHORT_NAME,
  APP_VERSION,
  APP_DESCRIPTION "\n\nRelease date: " __DATE__ "\n3dyd, acfu@3dyd.com"
);

VALIDATE_COMPONENT_FILENAME(APP_BINARY_NAME ".dll");

class AcfuRequest: public acfu::github_latest_release<AcfuRequest>, public acfu::github_conf {
 public:
  static const char* get_owner() {
    return "3dyd";
  }

  static const char* get_repo() {
    return "acfu";
  }

  virtual void process_release(const rapidjson::Value& release, file_info& info) {
    __super::process_release(release, info);
    info.meta_remove_field("download_page");
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
    info.meta_set("download_page", APP_URL_DOWNLOAD);
  }

  virtual bool is_newer(const file_info& info) {
    const char* version = info.meta_get("version", 0);
    return acfu::compare_versions(version, APP_VERSION, "v") > 0;
  }

  virtual acfu::request::ptr create_request() {
    acfu::request::ptr request = new service_impl_t<AcfuRequest>();
    return request;
  }
};

static service_factory_t<AcfuSource> g_acfu_source;
