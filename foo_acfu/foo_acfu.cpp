#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
  "Auto Check for Update",
  APP_VERSION,
  "Release date: " __DATE__ "\n3dyd, acfu@3dyd.com"
);

VALIDATE_COMPONENT_FILENAME("foo_acfu.dll");

struct AcfuGithubConf: acfu::github_conf {
  static const char* get_owner() {
    return "3dyd";
  }

  static const char* get_repo() {
    return APP_BINARY_NAME;
  }
};

#if 0
// #ifdef _DEBUG
class AcfuRequest: public acfu::github_releases<AcfuGithubConf> {
  virtual pfc::string8 form_releases_url() {
    return "http://127.0.0.1:8888/releases.json";
  }
};
#else
typedef acfu::github_releases<AcfuGithubConf> AcfuRequest;
#endif

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

  virtual bool is_newer(const char* version) {
    return acfu::compare_versions(version, APP_VERSION, "v") > 0;
  }

  virtual acfu::request::ptr create_request() {
    acfu::request::ptr request = new service_impl_t<AcfuRequest>();
    return request;
  }
};

static service_factory_t<AcfuSource> g_acfu_source;
