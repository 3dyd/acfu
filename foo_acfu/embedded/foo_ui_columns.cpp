#include "stdafx.h"
#include "resource.h"
#include "embedded.h"
#include "../acfu-sdk/utils/github.h"
#include "utils.h"

namespace embedded {

// {61368423-3FDC-44DA-9924-984480BA15B5}
static const GUID guid_columns_ui =
{0x61368423, 0x3fdc, 0x44da, { 0x99, 0x24, 0x98, 0x44, 0x80, 0xba, 0x15, 0xb5 }};

// {4D7B6AFF-DA24-4460-98E0-BDF9F9106006}
static const GUID guid_cfg_cui_follow_prereleases =
{0x4d7b6aff, 0xda24, 0x4460, { 0x98, 0xe0, 0xbd, 0xf9, 0xf9, 0x10, 0x60, 0x6 }};

cfg_bool cfg_cui_follow_prereleases(guid_cfg_cui_follow_prereleases, false);

class ColumnsUi: public Component, public acfu::github_conf {
 public:
  ColumnsUi(): Component(guid_columns_ui, "foo_ui_columns") {}

  static const char* get_owner() {
    return "reupen";
  }

  static const char* get_repo() {
    return "columns_ui";
  }

  virtual void context_menu_build(HMENU menu, unsigned id_base) override {
    AppendMenu(menu, MFT_SEPARATOR, 0, NULL);

    unsigned state = cfg_cui_follow_prereleases ? MFS_CHECKED : MFS_UNCHECKED;
    AppendMenu(menu, MFT_STRING | state, id_base, Tr(IDS_FOLLOW_PRERELEASES));
  }

  virtual void context_menu_command(unsigned id, unsigned id_base) override {
    if (id_base != id) {
      uBugCheck();
    }
    cfg_cui_follow_prereleases = !cfg_cui_follow_prereleases;
  }

  virtual void get_info(file_info& info) override {
    __super::get_info(info);
    info.meta_set("download_page", "https://yuo.be/columns_ui");
  }

  virtual acfu::request::ptr create_request() override {
    acfu::request::ptr request;
    if (cfg_cui_follow_prereleases) {
      request = new service_impl_t<acfu::github_releases<ColumnsUi>>();
    }
    else {
      request = new service_impl_t<acfu::github_latest_release<ColumnsUi>>();
    }
    return request;
  }

  virtual bool is_newer(const file_info& info) override {
    const char* available = info.meta_get("version", 0);
    const char* installed = GetInfo().meta_get("version", 0);

    return acfu::is_newer(available, installed);
  }
};

static service_factory_single_t<ColumnsUi> g_columns_ui;

} // namespace embedded
