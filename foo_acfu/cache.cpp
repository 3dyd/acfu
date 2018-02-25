#include "stdafx.h"
#include "cache.h"

namespace acfu {
  
static const char* CLEANUP_FLAG = "." APP_BINARY_NAME "-clean-up";

static service_factory_single_t<CacheImpl> g_cache;

class CacheLoad: public init_stage_callback {
  virtual void on_init_stage(t_uint32 stage) {
    if (init_stages::after_library_init == stage) {
      g_cache.get_static_instance().Load();
    }
  }
};

static service_factory_single_t<CacheLoad> g_cache_load;

class CacheSave: public initquit {
  virtual void on_quit() {
    g_cache.get_static_instance().Save();
  }
};

static service_factory_single_t<CacheSave> g_cache_save;

//////////////////////////////////////////////////////////////////////////

pfc::string8 CacheImpl::GetCacheDir() {
  return core_api::pathInProfile(APP_BINARY_NAME "\\cache\\");
}

bool CacheImpl::get_info(const GUID& guid, file_info& info) {
  pfc::mutexScope lock(mutex_);

  auto it = cache_.find(pfc::string8(pfc::print_guid(guid)).get_ptr());
  if (it.is_empty()) {
    return false;
  }

  info.copy(it->m_value);

  return true;
}

void CacheImpl::Load() {
  abort_callback_dummy abort;
  pfc::list_t<pfc::string8> paths;
  bool clean_up = false;

  try {
    auto path = GetCacheDir();
    foobar2000_io::listDirectory(path, abort, [&paths](const char* path, auto, bool is_subdirectory) {
      if (!is_subdirectory) {
        paths.add_item(path);
      }
    });

    path += CLEANUP_FLAG;
    clean_up = filesystem::g_exists(path, abort);
  }
  catch (exception_io_not_found&) {
  }
  catch (std::exception& e) {
    console::formatter() << "[" APP_BINARY_NAME "] list cache directory: " << e.what();
  }

  decltype(cache_) cache;
  paths.enumerate([&](const auto& path) {
    auto filename = pfc::string_filename_ext(path);
    try {
      if (clean_up) {
        filesystem::g_remove(path, abort);
      }
      else {
        GUID guid = pfc::GUID_from_text(filename.get_ptr());
        auto source = source::g_get(guid);
        file_ptr file;
        filesystem::g_open_read(file, path.c_str(), abort);
        file_info_impl info;
        info.from_stream(file.get_ptr(), abort);
        cache[filename.get_ptr()] = info;
      }
    }
    catch (std::exception& e) {
      console::formatter() << "[" APP_BINARY_NAME "] "
        << (clean_up ? "cleanup: error deleting " : "error loading cache entry ")
        << filename << ": " << e.what();
    }
  });

  {
    pfc::mutexScope lock(mutex_);
    cache_ = cache;
  }
}

void CacheImpl::register_callback(callback* callback) {
  core_api::ensure_main_thread();
  callbacks_.add_item(callback);
}

void CacheImpl::Save() {
  decltype(cache_) cache;
  {
    pfc::mutexScope lock(mutex_);
    cache = cache_;
  }

  abort_callback_dummy abort;
  auto dir = GetCacheDir();
  create_directory_helper::create_path(dir.c_str(), abort);

  cache.enumerate([&](const auto& key, const auto& value) {
    auto path = dir;
    path += key;
    file_ptr file;
    filesystem::g_open_write_new(file, path.c_str(), abort);
    value.to_stream(file.get_ptr(), abort);
  });

  if (clean_up_) {
    auto path = dir;
    path += CLEANUP_FLAG;
    try {
      filesystem::g_open_write_new(file::ptr(), path, abort);
    }
    catch (...) {
    }
  }
}

void CacheImpl::ScheduleCleanup() {
  g_cache.get_static_instance().clean_up_ = true;
}

void CacheImpl::set_info(const GUID& guid, const file_info& info) {
  file_info_const_impl copy(info);
  fb2k::inMainThread([this, guid, copy = std::move(copy)] {
    SetInfoInMainThread(guid, copy);
  });
}

void CacheImpl::SetInfoInMainThread(const GUID& guid, const file_info& info) {
  {
    pfc::mutexScope lock(mutex_);
    auto& existing = cache_[pfc::print_guid(guid).get_ptr()];
    if (file_info::g_is_info_equal(existing, info)) {
      return;
    }
    existing = info;
  }
  callbacks_.for_each([&guid](auto callback) {
    callback->on_info_changed(guid);
  });
}

void CacheImpl::unregister_callback(callback* callback) {
  core_api::ensure_main_thread();

  auto index = callbacks_.find_item(callback);
  if (~0 != index) {
    callbacks_.remove_by_idx(index);
  }
}

} // namespace acfu
