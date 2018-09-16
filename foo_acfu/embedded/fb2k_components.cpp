#include "stdafx.h"
#include "resource.h"
#include "embedded.h"
#include "utils.h"

namespace embedded {

// {AA5AE06C-8FFA-45A1-AB7D-18A83F9A73FC}
static const GUID guid_fb2k_components =
{ 0xaa5ae06c, 0x8ffa, 0x45a1, { 0xab, 0x7d, 0x18, 0xa8, 0x3f, 0x9a, 0x73, 0xfc } };

cfg_bool cfg_fb2k_components_enabled(guid_fb2k_components, true);

class Fb2kComponents: public Embedded {
  class Request: public acfu::request {
   public:
    Request(const char* components): components_(components) {}

   private:
    virtual void run(file_info& info, abort_callback& abort) override;

   private:
    pfc::string8 components_;
  };

 public:
  virtual void Init() {}

  pfc::list_t<componentversion::ptr> GetUserComponents() {
    pfc::list_t<pfc::string8> names;
    try {
      auto base_dir = core_api::pathInProfile("user-components");
      pfc::list_t<pfc::string8> dirs;
      abort_callback_dummy abort;
      listDirectories(base_dir, dirs, abort);
      for (size_t i = 0; i < dirs.get_size(); i ++) {
        pfc::string_filename_ext name(dirs[i]);
        names.add_item(name);
      }
    }
    catch (std::exception&) {
    }

    pfc::list_t<componentversion::ptr> user_components;
    acfu::for_each_service<componentversion>([&](auto& ptr) {
      pfc::string8 file_name;
      ptr->get_file_name(file_name);
      if (~0 != names.find_item(file_name)) {
        user_components.add_item(ptr);
      }
    });

    return user_components;
  }

  virtual const char* GetName() {
    return "foobar2000 components";
  }

  virtual void Enable(bool enable) {
    cfg_fb2k_components_enabled = enable;
  }

  virtual bool IsEnabled() {
    return cfg_fb2k_components_enabled;
  }

  virtual bool IsModified() {
    return initially_enabled_ != cfg_fb2k_components_enabled;
  }

  virtual GUID get_guid() {
    return guid_fb2k_components;
  }

  virtual void get_info(file_info& info) override {
    info.meta_set("name", "foobar2000 third-party components");

    auto user_components = GetUserComponents();
    CString total;
    total.Format(Tr(IDS_TOTAL), (int)user_components.get_size());
    auto utf8 = pfc::stringcvt::string_utf8_from_os(total);
    info.meta_set("version", utf8);
  }

  virtual acfu::request::ptr create_request() override {
    pfc::string8 components;

    auto user_components = GetUserComponents();
    for (size_t i = 0; i < user_components.get_size(); i ++) {
      auto& ptr = user_components[i];

      pfc::string8 file_name;
      ptr->get_file_name(file_name);
      components << file_name << "\t";

      pfc::string8 name;
      ptr->get_component_name(name);
      components << name << "\t";

      pfc::string8 version;
      ptr->get_component_version(version);
      components << version << "\n";
    }

    acfu::request::ptr request = new service_impl_t<Request>(components.get_ptr());

    return request;
  }

  virtual bool is_newer(const file_info& info) override {
    if (auto response = info.info_get("response")) {
      // TODO: parse it here
      console::print(response);
    }
    return false;
  }

 private:
  bool initially_enabled_ = true;
};

static service_factory_single_t<Fb2kComponents> g_fb2k_components;

void Fb2kComponents::Request::run(file_info& info, abort_callback& abort) {
  pfc::string8 url = "https://www.foobar2000.org/update-components";

  auto request = static_api_ptr_t<http_client>()->create_request("POST");
  http_request_post::ptr post_data;
  if (!request->cast(post_data)) {
    throw exception_service_extension_not_found();
  }

  post_data->add_post_data("components", components_);
  post_data->add_post_data("revision", "3");
  post_data->add_post_data("appversion", core_version_info::g_get_version_string());
  // Algo to get fingerprint is unknown
//  post_data->add_post_data("fingerprint", ???);

  file::ptr response = request->run(url, abort);
  pfc::array_t<char> data;
  response->read_till_eof(data, abort);
  info.info_set_ex("response", pfc_infinite, data.get_ptr(), data.get_size());
}

} // namespace embedded
