#include "stdafx.h"
#include "resource.h"
#include "preferences_dlg.h"
#include "scheduler.h"
#include "updates.h"

enum {
  kColName,
  kColModule,
  kColInstalled,
  kColAvailable,
};

PreferencesDlg::PreferencesDlg(preferences_page_callback::ptr callback): callback_(callback) {
  acfu::cfg_acfu_sources.sort();
}

PreferencesDlg::~PreferencesDlg() {
}

void PreferencesDlg::apply() {
  auto sources = GetCheckedSources();
  pfc::list_t<GUID>::g_swap(acfu::cfg_acfu_sources, sources);

  t_uint32 period = CUpDownCtrl(GetDlgItem(IDC_PERIOD_SPIN)).GetPos(NULL);
  static_api_ptr_t<acfu::Scheduler>()->SetPeriod(period);

  if (clear_cache_) {
    acfu::UpdatesImpl::ScheduleCleanup();
  }

  callback_->on_state_changed();
}

void PreferencesDlg::FreeList() {
  for (int i = list_.GetItemCount() - 1; i >= 0; i --) {
    if (auto guid = reinterpret_cast<GUID*>(list_.GetItemData(i))) {
      delete guid;
    }
  }
  list_.DeleteAllItems();
}

pfc::list_t<GUID> PreferencesDlg::GetCheckedSources() const {
  pfc::list_t<GUID> sources;
  ForEachInList([&](const auto& guid, auto index) {
    if (list_.GetCheckState(index)) {
      sources.add_item(guid);
    }
  });

  sources.sort();

  return sources;
}

t_uint32 PreferencesDlg::get_state() {
  bool resettable = false, changed = false, need_restart = false;

  auto sources = GetCheckedSources();
  resettable = resettable || 0 != sources.get_count();
  changed = changed || !decltype(sources)::g_equals(sources, acfu::cfg_acfu_sources);

  t_uint32 period = CUpDownCtrl(GetDlgItem(IDC_PERIOD_SPIN)).GetPos(NULL);
  resettable = resettable || acfu::Scheduler::kPeriodDefault != period;
  changed = changed || static_api_ptr_t<acfu::Scheduler>()->GetPeriod() != period;

  resettable = resettable || clear_cache_;
  changed = changed || clear_cache_;
  need_restart = need_restart  || clear_cache_;

  t_uint32 state = resettable ? preferences_state::resettable : 0;
  state |= changed ? preferences_state::changed : 0;
  state |= need_restart ? preferences_state::needs_restart : 0;

  return state;
}

void PreferencesDlg::InitList() {
  acfu::for_each_service<acfu::source>([&](auto ptr) {
    if (pfc::guid_null == ptr->get_guid()) {
      return;
    }

    file_info_impl info;
    ptr->get_info(info);

    pfc::string8 name;
    if (auto value = info.meta_get("name", 0)) {
      name = value;
    }
    else {
      name = pfc::print_guid(ptr->get_guid());
    }

    int index = list_.AddItem(list_.GetItemCount(), 0, pfc::stringcvt::string_os_from_utf8(name));
    list_.SetItemData(index, reinterpret_cast<DWORD_PTR>(new GUID(ptr->get_guid())));
    if (auto value = info.meta_get("version", 0)) {
      list_.SetItemText(index, kColInstalled, pfc::stringcvt::string_os_from_utf8(value));
    }
    if (auto value = info.meta_get("module", 0)) {
      list_.SetItemText(index, kColModule, pfc::stringcvt::string_os_from_utf8(value));
    }
    if (~0 != acfu::cfg_acfu_sources.find_item(ptr->get_guid())) {
      list_.SetCheckState(index, TRUE);
    }
  });
}

void PreferencesDlg::on_info_changed(const GUID& guid, const file_info& info) {
  ForEachInList([&](auto item_guid, auto index) {
    if (item_guid == guid) {
      UpdateListItem(index, guid, info);
    }
  });
}

void PreferencesDlg::OnCfuAll(UINT uNotifyCode, int nID, CWindow wndCtl) {
  acfu::Scheduler::Check(m_hWnd, GetCheckedSources());
}

void PreferencesDlg::OnCfuSingle(UINT uNotifyCode, int nID, CWindow wndCtl) {
  if (auto guid_ptr = reinterpret_cast<GUID*>(list_.GetItemData(list_.GetSelectedIndex()))) {
    acfu::check::g_check(m_hWnd, *guid_ptr);
  }
}

void PreferencesDlg::OnClearCache(UINT uNotifyCode, int nID, CWindow wndCtl) {
  clear_cache_ = true;
  callback_->on_state_changed();
}

void PreferencesDlg::OnContextMenu(CWindow wnd, _WTYPES_NS::CPoint point) {
  if (list_ != wnd) {
    return SetMsgHandled(FALSE);
  }
  int index = list_.GetSelectedIndex();
  if (-1 == index) {
    return;
  }

  if (-1 == point.x && -1 == point.y) { // from keyboard
    CRect client_rect, item_rect;
    list_.GetClientRect(&client_rect);
    list_.GetItemRect(index, &item_rect, LVIR_BOUNDS);
    item_rect.IntersectRect(&item_rect, &client_rect);
    if (!item_rect.IsRectEmpty()) {
      point = CPoint(item_rect.left, item_rect.bottom);
    }
    else {
      point = client_rect.CenterPoint();
    }
    list_.ClientToScreen(&point);
  }

  CMenu menu;
  ATLVERIFY(menu.LoadMenu(IDR_SOURCE_POPUP));
  menu.GetSubMenu(0).TrackPopupMenu(0, point.x, point.y, m_hWnd);
}

void PreferencesDlg::OnDestroy() {
  FreeList();
  static_api_ptr_t<acfu::updates>()->unregister_callback(this);
}

BOOL PreferencesDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
  SetWindowText(TEXT(APP_SHORT_NAME));

  CreateScaledFontEx(title_font_, GetFont(), 1, FW_BOLD);
  GetDlgItem(IDC_AVAILABLE_SOURCES).SetFont(title_font_);

  CUpDownCtrl spin(GetDlgItem(IDC_PERIOD_SPIN));
  spin.SetRange(acfu::Scheduler::kPeriodMin, acfu::Scheduler::kPeriodMax);
  spin.SetPos(static_api_ptr_t<acfu::Scheduler>()->GetPeriod());

  list_.SubclassWindow(GetDlgItem(IDC_SOURCE_LIST));
  ::SetWindowTheme(list_, L"explorer", NULL);
  DWORD style = LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP;
  list_.SetExtendedListViewStyle(style, style);

  ATLVERIFY(kColName == list_.InsertColumn(kColName, L"Name"));
  ATLVERIFY(kColModule == list_.InsertColumn(kColModule, L"Module"));
  ATLVERIFY(kColInstalled == list_.InsertColumn(kColInstalled, L"Installed", LVCFMT_RIGHT));
  ATLVERIFY(kColAvailable == list_.InsertColumn(kColAvailable, L"Available", LVCFMT_RIGHT));

  InitList();
  UpdateList();

  split_.SubclassWindow(GetDlgItem(IDC_CFU_ALL));

  static_api_ptr_t<acfu::updates>()->register_callback(this);

  return TRUE;
}

LRESULT PreferencesDlg::OnListItemChanged(LPNMHDR pnmh) {
  LPNMLISTVIEW info = (LPNMLISTVIEW)pnmh;

  if (info->uChanged & LVIF_STATE) {
    UINT state_new = info->uNewState & LVIS_STATEIMAGEMASK;
    UINT state_old = info->uOldState & LVIS_STATEIMAGEMASK;

    if (0 != state_new && 0 != state_old && state_new != state_old) {
      callback_->on_state_changed();
    }
  }

  SetMsgHandled(FALSE);
  return 0;
}

void PreferencesDlg::OnPeriodEdit(UINT uNotifyCode, int nID, CWindow wndCtl) {
  BOOL error = FALSE;
  if (0 != CUpDownCtrl(GetDlgItem(IDC_PERIOD_SPIN)).GetPos(&error) && !error) {
    callback_->on_state_changed();
  }
}

LRESULT PreferencesDlg::OnSplitDropDown(LPNMHDR pnmh) {
  CMenu menu;
  ATLVERIFY(menu.LoadMenu(IDR_MAIN_POPUP));
  CMenuHandle popup = menu.GetSubMenu(0);

  if (clear_cache_) {
    CString what, format, message;
    ATLVERIFY(popup.GetMenuString(ID_CLEAR_CACHE, what, MF_BYCOMMAND));
    ATLVERIFY(format.LoadString(IDS_NEED_APPLY));
    message.Format(format, static_cast<LPCWSTR>(what));

    MENUITEMINFO info = {sizeof(MENUITEMINFO)};
    info.fState = MFS_GRAYED | MFS_CHECKED;
    info.dwTypeData = message.GetBuffer();
    info.fMask = MIIM_STATE | MIIM_STRING;
    menu.SetMenuItemInfo(ID_CLEAR_CACHE, FALSE, &info);
  }

  CRect rect;
  GetDlgItem(IDC_CFU_ALL).GetWindowRect(&rect);
  popup.TrackPopupMenu(0, rect.left, rect.bottom, m_hWnd, &rect);

  return 0;
}

LRESULT PreferencesDlg::OnWhy(LPNMHDR pnmh) {
  ShellExecute(NULL, L"open", L"https://www.google.com", NULL, NULL, SW_SHOWNORMAL);
  return 0;
}

void PreferencesDlg::reset() {
  for (int i = list_.GetItemCount() - 1; i >= 0; i --) {
    list_.SetCheckState(i, FALSE);
  }
  CUpDownCtrl(GetDlgItem(IDC_PERIOD_SPIN)).SetPos(acfu::Scheduler::kPeriodDefault);
  clear_cache_ = false;

  callback_->on_state_changed();
}

void PreferencesDlg::UpdateList() {
  static_api_ptr_t<acfu::updates> updates;
  ForEachInList([&](auto guid, auto index) {
    file_info_impl info;
    updates->get_info(guid, info);
    UpdateListItem(index, guid, info);
  });
}

void PreferencesDlg::UpdateListItem(int index, const GUID& guid, const file_info& info) {
  CString last_version;
  if (auto value = info.meta_get("version", 0)) {
    last_version = pfc::stringcvt::string_os_from_utf8(value);
  }
  else if (acfu::source::g_get(guid)->is_newer(info)) {
    ATLVERIFY(last_version.LoadString(IDS_GREATER_VERSION));
  }
  list_.SetItemText(index, kColAvailable, last_version);
}
