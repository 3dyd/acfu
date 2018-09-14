#include "stdafx.h"
#include "embedded.h"
#include "../acfu-sdk/utils/github.h"

namespace embedded {

// {0CDC57F6-8BBA-4E02-B6A1-6BB66421674B}
static const GUID guid_jscript =
{0xcdc57f6, 0x8bba, 0x4e02, { 0xb6, 0xa1, 0x6b, 0xb6, 0x64, 0x21, 0x67, 0x4b }};

class Jscript: public Component, public acfu::github_conf {
 public:
  Jscript(): Component(guid_jscript, "foo_jscript_panel") {}

 public:
  static const char* get_owner() {
    return "marc2k3";
  }

  static const char* get_repo() {
    return "foo_jscript_panel";
  }

  virtual void get_info(file_info& info) override {
    __super::get_info(info);
    info.meta_set("download_page", "https://github.com/marc2k3/foo_jscript_panel/releases");
  }

  virtual acfu::request::ptr create_request() override {
    acfu::request::ptr request = new service_impl_t<acfu::github_latest_release<Jscript>>();
    return request;
  }

  virtual bool IsNewer(const char* available, const char* installed) override {
    return acfu::compare_versions(available, installed, "v") > 0;
  }
};

static service_factory_single_t<Jscript> g_jscript;

} // namespace embedded
