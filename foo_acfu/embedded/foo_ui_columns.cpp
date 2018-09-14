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

 public:
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

  virtual bool IsNewer(const char* available, const char* installed) override {
    try {
      auto available_parts = GetParts(available);
      auto installed_parts = GetParts(installed);
      for (size_t i = 0; i < available_parts.get_size(); i ++) {
        if (int diff = available_parts[i] - installed_parts[i]) {
          return diff > 0;
        }
      }
    }
    catch (acfu::version_error&) {
    }
    return false;
  }

  pfc::list_t<int> GetParts(std::string version) {
    std::regex version_regex("v?(\\d+)\\.(\\d+)\\.(\\d+)(?:\\.(\\d+))?(?:-(alpha|beta|rc)\\.(\\d+))?");
    std::smatch match;
    if (!std::regex_match(version, match, version_regex)) {
      throw acfu::version_error();
    }

    pfc::list_t<int> parts;
    parts.set_count(6);

    parts[0] = atoi(match[1].str().c_str());
    parts[1] = atoi(match[2].str().c_str());
    parts[2] = atoi(match[3].str().c_str());
    parts[3] = atoi(match[4].str().c_str());

    auto prerelease = match[5];
    if ("rc" == prerelease) {
      parts[4] = -1;
    }
    else if ("beta" == prerelease) {
      parts[4] = -2;
    }
    else if ("alpha" == prerelease) {
      parts[4] = -3;
    }
    else {
      parts[4] = 0;
    }

    parts[5] = atoi(match[6].str().c_str());

    return parts;
  }
};

static service_factory_single_t<ColumnsUi> g_columns_ui;

} // namespace embedded
