#include "stdafx.h"
#include "embedded.h"
#include "utils.h"

namespace embedded {

bool Embedded::IsAnyModified() {
  bool modified = false;
  for_each_service<acfu::source, Embedded>([&modified](auto& ptr) {
    modified = modified || ptr->IsModified();
  });
  return modified;
}

Component::Component(const GUID& guid, const char* file_name)
  : cfg_enabled_(guid, true), file_name_(file_name) {
}

void Component::Enable(bool enable) {
  cfg_enabled_ = enable;
}

GUID Component::get_guid() {
  return guid_;
}

void Component::get_info(file_info& info) {
  info.copy_meta(info_);
}

const char* Component::GetName() {
  return file_name_.get_ptr();
}

void Component::Init() {
  initially_enabled_ = cfg_enabled_;
  if (!initially_enabled_) {
    return;
  }

  componentversion::ptr cv;
  acfu::for_each_service<componentversion>([&](auto& ptr) {
    pfc::string8 file_name;
    ptr->get_file_name(file_name);
    if (file_name == file_name_) {
      cv = ptr;
    }
  });
  if (cv.is_empty()) {
    return;
  }

  guid_ = cfg_enabled_.get_guid();

  info_.meta_set("module", file_name_.get_ptr());

  pfc::string8 version;
  cv->get_component_version(version);
  if (!version.is_empty()) {
    info_.meta_set("version", version.get_ptr());
  }

  pfc::string8 name;
  cv->get_component_name(name);
  if (!name.is_empty()) {
    info_.meta_set("name", name.get_ptr());
  }
}

bool Component::is_newer(const file_info& info) {
  const char* available = info.meta_get("version", 0);
  const char* installed = info_.meta_get("version", 0);

  return IsNewer(available, installed);
}

bool Component::IsEnabled() {
  return cfg_enabled_;
}

bool Component::IsModified() {
  return initially_enabled_ != cfg_enabled_;
}

// NOTE: should be in sync with acfu::CacheLoad (executed _before_ it)
class EmbeddedInit: public init_stage_callback {
  virtual void on_init_stage(t_uint32 stage) {
    if (init_stages::before_ui_init == stage) {
      for_each_service<acfu::source, Embedded>([](auto& ptr) {
        ptr->Init();
      });
    }
  }
};

static service_factory_single_t<EmbeddedInit> g_init;

} // namespace embedded
