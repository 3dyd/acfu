#pragma once

namespace acfu {

extern cfg_guidlist cfg_acfu_sources;

class Scheduler: public service_base {
  FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(Scheduler);

 public:
  enum {
    kMsInDay = 1000 * 60 * 60 * 24,
    kPeriodMax = USER_TIMER_MAXIMUM / kMsInDay,
    kPeriodMin = 1,
    kPeriodDefault = 7,
  };

  virtual t_uint32 GetPeriod() = 0;
  virtual void SetPeriod(t_uint32 period) = 0;

  static void Check(HWND parent, const pfc::list_t<GUID>& sources);
};

} // namespace acfu
