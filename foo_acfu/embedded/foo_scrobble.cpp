#include "stdafx.h"
#include "embedded.h"
#include "../acfu-sdk/utils/github.h"

namespace embedded {

// {3F810CF3-8D01-456C-80EA-062BFF2F5F29}
static const GUID guid_scrobble =
{0x3f810cf3, 0x8d01, 0x456c, { 0x80, 0xea, 0x6, 0x2b, 0xff, 0x2f, 0x5f, 0x29 }};

class Scrobble: public Component, public acfu::github_conf {
 public:
  Scrobble(): Component(guid_scrobble, "foo_scrobble") {}

  static const char* get_owner() {
    return "gix";
  }

  static const char* get_repo() {
    return "foo_scrobble";
  }

  virtual void get_info(file_info& info) override {
    __super::get_info(info);
    info.meta_set("download_page", "https://github.com/gix/foo_scrobble/releases");
  }

  virtual acfu::request::ptr create_request() override {
    acfu::request::ptr request = new service_impl_t<acfu::github_latest_release<Scrobble>>();
    return request;
  }

  virtual bool is_newer(const file_info& info) override {
    const char* available = info.meta_get("version", 0);
    const char* installed = GetInfo().meta_get("version", 0);

    return acfu::is_newer(available, installed);
  }
};

static service_factory_single_t<Scrobble> g_scrobble;

} // namespace embedded
