#pragma once

#include "../acfu-sdk/acfu.h"

namespace embedded {

class Embedded: public acfu::source {
  FB2K_MAKE_SERVICE_INTERFACE(Embedded, acfu::source);

 public:
  virtual void Init() = 0;
  virtual const char* GetName() = 0;

  virtual void Enable(bool enable) = 0;
  virtual bool IsEnabled() = 0;
  virtual bool IsModified() = 0;

  static bool IsAnyModified();
};

// {30D4EDA7-9AFE-4B93-894D-AC02460C9582}
FOOGUIDDECL const GUID Embedded::class_guid =
{0x30d4eda7, 0x9afe, 0x4b93, { 0x89, 0x4d, 0xac, 0x2, 0x46, 0xc, 0x95, 0x82 }};

class Component: public Embedded {
 public:
  Component(const GUID& guid, const char* file_name);

  virtual GUID get_guid() override;
  virtual void get_info(file_info& info) override;

  virtual void Enable(bool enable) override;
  virtual const char* GetName() override;
  virtual void Init() override;
  virtual bool IsEnabled() override;
  virtual bool IsModified() override;

  const file_info& GetInfo() const;

 private:
  bool initially_enabled_ = true;
  cfg_bool cfg_enabled_;
  pfc::string8 file_name_;
  GUID guid_ = pfc::guid_null;
  file_info_impl info_;
};

} // namespace embedded
