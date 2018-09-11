#include "stdafx.h"
#include "scheduler.h"

namespace acfu {

// {EB73FF9D-D0AF-4232-BFA6-FA21A43C953B}
static const GUID guid_cfg_period =
{ 0xeb73ff9d, 0xd0af, 0x4232, { 0xbf, 0xa6, 0xfa, 0x21, 0xa4, 0x3c, 0x95, 0x3b } };

cfg_uint cfg_period(guid_cfg_period, Scheduler::kPeriodDefault);

// {264F8CC9-29A2-4DDE-934E-C8FFA666C6BB}
static const GUID guid_cfg_last_run =
{ 0x264f8cc9, 0x29a2, 0x4dde, { 0x93, 0x4e, 0xc8, 0xff, 0xa6, 0x66, 0xc6, 0xbb } };

cfg_int_t<t_filetimestamp> cfg_last_run(guid_cfg_last_run, 0);

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
        const char* name = "";
        if (info.meta_exists("name")) {
          name = info.meta_get("name", 0);
        }
        status.set_item(name);

        auto request = source->create_request();
        if (request.is_valid()) {
          file_info_impl info;
          request->run(info, abort);
          static_api_ptr_t<updates>()->set_info(source->get_guid(), info);
        }
      }
      catch (std::exception& e) {
        console::formatter()
          << APP_SHORT_NAME << ": failed for source "
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
  BackgroundCheck(std::function<void()> on_done): on_done_(on_done) {}

  void AbortThread() {
    __super::AbortThread();
  }

  void StartThread(const pfc::list_t<GUID>& sources) {
    AbortThread();
    sources_ = sources;
    __super::StartThread(THREAD_PRIORITY_BELOW_NORMAL);
  }

 private:
  virtual void ThreadDone(unsigned p_code) {
    on_done_();
  }

  virtual unsigned ThreadProc(abort_callback& abort) {
    console::formatter() << APP_SHORT_NAME << ": checking for updates...";
    service_impl_t<Check> check(sources_);
    try {
      check.run(threaded_process_status_dummy(), abort);
    }
    catch (std::exception& e) {
      console::formatter() << APP_SHORT_NAME << ": stopped: " << e.what();
      throw;
    }
    console::formatter() << APP_SHORT_NAME << ": completed";

    return 0;
  }

 private:
  pfc::list_t<GUID> sources_;
  std::function<void()> on_done_;
};

class SchedulerImpl: public Scheduler, public CMessageMap {
 public:
  enum {
    kInitTimerId = 80561,
    kAcfuTimerId,

    kInitTimerDelay = 30 * 1000
  };

  BEGIN_MSG_MAP_EX(SchedulerImpl)
    MSG_WM_TIMER(OnTimer)
  END_MSG_MAP()

  SchedulerImpl(): message_wnd_(L"Message", this, 0), worker_([&] {
    cfg_last_run = filetimestamp_from_system_timer();
    SetInitTimer();
  }) {}

  void Init() {
    message_wnd_.Create(HWND_MESSAGE, CRect());
    SetInitTimer();
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
    SetInitTimer();
  }

 private:
  void OnTimer(UINT_PTR nIDEvent) {
    if (kInitTimerId == nIDEvent) {
      SetAcfuTimer();
    }
    else if (kAcfuTimerId == nIDEvent) {
      SetInitTimer();
    }
  }

  void SetAcfuTimer() {
    core_api::assert_main_thread();

    message_wnd_.KillTimer(kInitTimerId);
    message_wnd_.KillTimer(kAcfuTimerId);

    auto next_run = GetPeriod() * system_time_periods::day + cfg_last_run;
    auto now = filetimestamp_from_system_timer();
    if (0 == cfg_last_run || next_run < now) {
      if (0 != cfg_acfu_sources.get_size()) {
        worker_.StartThread(cfg_acfu_sources);
        return;
      }
      cfg_last_run = filetimestamp_from_system_timer();
    }

    auto delay = std::min((next_run - now) / system_time_periods::second * 1000, (t_filetimestamp)USER_TIMER_MAXIMUM);
    message_wnd_.SetTimer(kAcfuTimerId, (UINT)delay);
    console::formatter() << APP_SHORT_NAME << ": next check is scheduled for " << format_filetimestamp(next_run);
  }

  void SetInitTimer() {
    core_api::assert_main_thread();

    message_wnd_.KillTimer(kAcfuTimerId);
    message_wnd_.SetTimer(kInitTimerId, kInitTimerDelay);
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
