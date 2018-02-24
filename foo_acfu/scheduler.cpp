#include "stdafx.h"
#include "scheduler.h"
#include "../acfu-sdk/acfu_util.h"

namespace acfu {

// {EB73FF9D-D0AF-4232-BFA6-FA21A43C953B}
static const GUID guid_cfg_period = 
{ 0xeb73ff9d, 0xd0af, 0x4232, { 0xbf, 0xa6, 0xfa, 0x21, 0xa4, 0x3c, 0x95, 0x3b } };

cfg_uint cfg_period(guid_cfg_period, Scheduler::kPeriodDefault);

// {26E16C98-32C8-440D-9E30-BD283FC41DB3}
static const GUID guid_cfg_acfu_sources = 
{ 0x26e16c98, 0x32c8, 0x440d, { 0x9e, 0x30, 0xbd, 0x28, 0x3f, 0xc4, 0x1d, 0xb3 } };

cfg_guidlist cfg_acfu_sources(guid_cfg_acfu_sources);

// {EB228486-2B26-4705-8B04-28878E9753E7}
FOOGUIDDECL const GUID Scheduler::class_guid =
{ 0xeb228486, 0x2b26, 0x4705, { 0x8b, 0x4, 0x28, 0x87, 0x8e, 0x97, 0x53, 0xe7 } };

class threaded_process_status_dummy: public threaded_process_status {
 public:
  virtual void set_progress(t_size p_state) {}
  virtual void set_progress_secondary(t_size p_state) {}
  virtual void set_item(const char * p_item,t_size p_item_len = ~0) {}
  virtual void set_item_path(const char * p_item,t_size p_item_len = ~0) {}
  virtual void set_title(const char * p_title,t_size p_title_len = ~0) {}
  virtual void force_update() {}
  virtual bool is_paused() { return false; }
  virtual bool process_pause() { return false; }
};

class Check: public threaded_process_callback {
 public:
  Check(const pfc::list_t<GUID>& sources): sources_(sources) {}

  virtual void run(threaded_process_status& status, abort_callback& abort) {
    for (t_size i = 0; i < sources_.get_size(); i ++) {
      try {
        auto source = source::g_get(sources_[i]);
        file_info_impl info;
        source->get_info(info);
        status.set_item(info.info_exists("name") ? info.info_get("name") : "");

        auto request = source->create_request();
        if (request.is_valid()) {
          file_info_impl info;
          request->run(info, abort);
          static_api_ptr_t<cache>()->set_info(source->get_guid(), info);
        }
      }
      catch (std::exception& e) {
        console::formatter()
          << "error checking for updates, source "
          << pfc::print_guid(sources_[i]) << ": " << e.what();
      }

      status.set_progress_float((1. + i) / sources_.get_size());
    }
  }

 private:
  pfc::list_t<GUID> sources_;
};

void Scheduler::Check(HWND parent, const pfc::list_t<GUID>& sources) {
  threaded_process::g_run_modal(
    new service_impl_t<acfu::Check>(sources),
    threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item,
    parent,
    "Checking for Updates..."
  );
}

class BackgroundCheck: private CSimpleThread {
 public:
  void AbortThread() {
    __super::AbortThread();
  }

  void StartThread(const pfc::list_t<GUID>& sources) {
    AbortThread();
    sources_ = sources;
    __super::StartThread(THREAD_PRIORITY_BELOW_NORMAL);
  }

 private:
  virtual unsigned ThreadProc(abort_callback& abort) {
    service_impl_t<Check> check(sources_);
    check.run(threaded_process_status_dummy(), abort);

    return 0;
  }

 private:
  pfc::list_t<GUID> sources_;
};

class SchedulerImpl: public Scheduler, public CMessageMap {
 public:
  enum {
    kTimerId = 80561,
  };

  BEGIN_MSG_MAP_EX(SchedulerImpl)
    MSG_WM_TIMER_EX(kTimerId, OnTimer)
  END_MSG_MAP()

  SchedulerImpl(): message_wnd_(L"Message", this, 0) {
  }

  void Init() {
    message_wnd_.Create(HWND_MESSAGE, CRect());
    ResetTimer();
  }

  void Free() {
    message_wnd_.DestroyWindow();
    worker_.AbortThread();
  }

  virtual t_uint32 GetPeriod() {
    return std::clamp(cfg_period.get_value(), (t_uint32)kPeriodMin, (t_uint32)kPeriodMax);
  }

  virtual void SetPeriod(t_uint32 period) {
    cfg_period = period;
    ResetTimer();
  }

 private:
  void OnTimer() {
    worker_.StartThread(cfg_acfu_sources);
  }

  void ResetTimer() {
    message_wnd_.SetTimer(kTimerId, GetPeriod() * kMsInDay);
  }

 private:
  CContainedWindow message_wnd_;
  BackgroundCheck worker_;
};

static service_factory_single_t<SchedulerImpl> g_scheduler;

class SchedulerInit: public initquit {
  virtual void on_init() {
    g_scheduler.get_static_instance().Init();
  }
  virtual void on_quit() {
    g_scheduler.get_static_instance().Free();
  }
};

static service_factory_single_t<SchedulerInit> g_scheduler_init;

} // namespace acfu
