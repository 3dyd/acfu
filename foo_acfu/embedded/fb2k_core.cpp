#include "stdafx.h"
#include "embedded.h"

namespace embedded {

// {39733416-2AD5-4CA6-825E-FA6967977FFE}
static const GUID guid_fb2k_core =
{0x39733416, 0x2ad5, 0x4ca6, { 0x82, 0x5e, 0xfa, 0x69, 0x67, 0x97, 0x7f, 0xfe }};

class Fb2kCore: public Component {
  class Request: public acfu::request {
   public:
    Request(const char* version): version_(version ? version : "") {}
    virtual void run(file_info& info, abort_callback& abort) override;

   private:
    pfc::string8 version_;
  };

 public:
  Fb2kCore(): Component(guid_fb2k_core, "Core") {}

  virtual void get_info(file_info& info) override {
    __super::get_info(info);
    info.meta_set("download_page", "https://foobar2000.org/download");
  }

  virtual acfu::request::ptr create_request() override {
    auto version = GetInfo().meta_get("version", 0);
    acfu::request::ptr request = new service_impl_t<Request>(version);

    return request;
  }

  virtual bool is_newer(const file_info& info) override {
    if (auto response = info.info_get("is_newer")) {
      return 0 == strcmp(response, "1");
    }
    return false;
  }
};

static service_factory_single_t<Fb2kCore> g_fb2k_core;

void Fb2kCore::Request::run(file_info& info, abort_callback& abort) {
  pfc::string8 encoded;
  urlEncode(encoded, version_.c_str());

  pfc::string8 url = "https://www.foobar2000.org/update-core?version=";
  url += encoded;

  auto request = static_api_ptr_t<http_client>()->create_request("GET");
  file::ptr response = request->run(url, abort);
  pfc::array_t<char> data;
  response->read_till_eof(data, abort);
  data.append_fromptr("", 1);

  if (0 == strcmp(data.get_ptr(), "1")) {
    info.info_set("is_newer", "1");
  }
  else {
    info.meta_set("version", version_.get_ptr());

    if (0 != strcmp(data.get_ptr(), "0")) {
      console::complain("unexpected response when checking foobar2000 core version", data.get_ptr());
    }
  }
}

} // namespace embedded
