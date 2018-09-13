#include "stdafx.h"
#include "embedded_source.h"
#include "../../acfu-sdk/utils/github.h"

namespace embedded {

// {61368423-3FDC-44DA-9924-984480BA15B5}
static const GUID guid_columns_ui =
{0x61368423, 0x3fdc, 0x44da, { 0x99, 0x24, 0x98, 0x44, 0x80, 0xba, 0x15, 0xb5 }};

class ColumnsUi: public Component, public acfu::github_conf {
 public:
  ColumnsUi(): Component(guid_columns_ui, "foo_ui_columns") {}

 public:
  static const char* get_owner() {
    return "reupen";
  }

  static const char* get_repo() {
    return "columns_ui";
  }

  virtual void get_info(file_info& info) override {
    __super::get_info(info);
    info.meta_set("download_page", "https://yuo.be/columns_ui");
  }

  virtual acfu::request::ptr create_request() override {
    acfu::request::ptr request = new service_impl_t<acfu::github_latest_release<ColumnsUi>>();
    return request;
  }

  virtual bool IsNewer(const char* available, const char* isntalled) override {
    return acfu::compare_versions(available, isntalled, "v") > 0;
  }
};

static service_factory_single_t<ColumnsUi> g_columns_ui;

} // namespace embedded
