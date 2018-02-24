#pragma once

#include "../acfu-sdk/acfu.h"

namespace acfu {
  
class CacheImpl: public cache {
 public:
  virtual void register_callback(callback* callback);
  virtual void unregister_callback(callback* callback);

  virtual bool get_info(const GUID& guid, file_info& info);
  virtual void set_info(const GUID& guid, const file_info& info);

  void Load();
  void Save();

  static void ScheduleCleanup();

 private:
  pfc::string8 GetCacheDir();
  void SetInfoInMainThread(const GUID& guid, const file_info& info);

 private:
  pfc::list_t<callback*> callbacks_;
  pfc::map_t<pfc::string8, file_info_const_impl, pfc::string::comparatorCaseInsensitive> cache_;
  pfc::mutex mutex_;
  bool clean_up_ = false;
};

} // namespace acfu
