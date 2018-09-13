#include "stdafx.h"
#include "updates.h"

namespace acfu {
  
static const char* CLEANUP_FLAG = "." APP_BINARY_NAME "-clean-up";

static service_factory_single_t<UpdatesImpl> g_cache;

// NOTE: should be in sync with embedded::EmbeddedInit (executed _after_ it)
class CacheLoad: public init_stage_callback {
  virtual void on_init_stage(t_uint32 stage) {
    if (init_stages::after_ui_init == stage) {
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

pfc::string8 UpdatesImpl::GetCacheDir() {
  return core_api::pathInProfile(APP_BINARY_NAME "\\cache\\");
}

bool UpdatesImpl::get_info(const GUID& guid, file_info& info) {
  pfc::mutexScope lock(mutex_);

  auto it = cache_.find(pfc::string8(pfc::print_guid(guid)).get_ptr());
  if (it.is_empty()) {
    return false;
  }

  info.copy(it->m_value);

  return true;
}

void UpdatesImpl::Load() {
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
  decltype(updates_) updates;
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
        if (source->is_newer(info)) {
          updates.add_item(guid);
        }
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
    updates_ = updates;
  }

  if (0 != updates.get_count()) {
    callbacks_.for_each([&updates](auto callback) {
      callback->on_updates_available(updates);
    });
  }
}

void UpdatesImpl::register_callback(callback* callback) {
  core_api::ensure_main_thread();
  callbacks_.add_item(callback);

  decltype(updates_) updates;
  {
    pfc::mutexScope lock(mutex_);
    if (0 == updates_.get_count()) {
      return;
    }
    updates = updates_;
  }
  callback->on_updates_available(updates);
}

void UpdatesImpl::Save() {
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

void UpdatesImpl::ScheduleCleanup() {
  g_cache.get_static_instance().clean_up_ = true;
}

void UpdatesImpl::set_info(const GUID& guid, const file_info& info) {
  file_info_const_impl copy(info);
  fb2k::inMainThread([this, guid, copy = std::move(copy)] {
    SetInfoInMainThread(guid, copy);
  });
}

void UpdatesImpl::SetInfoInMainThread(const GUID& guid, const file_info& info) {
  {
    pfc::mutexScope lock(mutex_);

    auto& existing = cache_[pfc::print_guid(guid).get_ptr()];
    if (file_info::g_is_meta_equal(existing, info) && file_info::g_is_info_equal(existing, info)) {
      return;
    }
    existing = info;
  }

  callbacks_.for_each([&guid, &info](auto callback) {
    callback->on_info_changed(guid, info);
  });

  bool is_newer = false;
  if (auto source = source::g_get(guid); source.is_valid()) {
    is_newer = source->is_newer(info);
  }

  bool notify_updates_available = false;
  decltype(updates_) updates;
  {
    pfc::mutexScope lock(mutex_);

    auto index = updates_.find_item(guid);
    bool known_newer = ~0 != index;
    if (known_newer == is_newer) {
      return;
    }
    if (is_newer) {
      updates_.add_item(guid);
    }
    else {
      updates_.remove_by_idx(index);
    }
    updates = updates_;
  }

  callbacks_.for_each([&updates](auto callback) {
    callback->on_updates_available(updates);
  });
}

void UpdatesImpl::unregister_callback(callback* callback) {
  core_api::ensure_main_thread();

  auto index = callbacks_.find_item(callback);
  if (~0 != index) {
    callbacks_.remove_by_idx(index);
  }
}

} // namespace acfu
