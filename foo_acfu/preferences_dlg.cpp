#include "stdafx.h"
#include "resource.h"
#include "preferences_dlg.h"
#include "embedded_sources_dlg.h"
#include "scheduler.h"
#include "updates.h"
#include "urls.h"
#include "utils.h"

enum {
  kColName,
  kColAvailable,
  kColInstalled,
  kColModule,
};

static int s_sort_column = kColName;
static bool s_sort_up = true;

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
  CListViewCtrl list((HWND)lParamSort);

  CString str1, str2;
  list.GetItemText(lParam1, s_sort_column, str1);
  list.GetItemText(lParam2, s_sort_column, str2);

  return str1.CompareNoCase(str2) * (s_sort_up ? 1 : -1);
}

DWORD SourcesList::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw) {
  return CDRF_DODEFAULT | CDRF_NOTIFYSUBITEMDRAW;
}

DWORD SourcesList::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw) {
  CDCHandle dc(lpNMCustomDraw->hdc);
  default_font_ = dc.GetCurrentFont();

  if (bold_font_) {
    bold_font_.DeleteObject();
  }
  CreateScaledFontEx(bold_font_, default_font_, 1, FW_BOLD);

  return CDRF_DODEFAULT | CDRF_NOTIFYITEMDRAW;
}

DWORD SourcesList::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw) {
  LPNMLVCUSTOMDRAW info = (LPNMLVCUSTOMDRAW)lpNMCustomDraw;
  bool grayed = false;
  bool bold = false;

  if (kColAvailable == info->iSubItem || kColInstalled == info->iSubItem) {
    LVITEM item = {0};
    item.mask = LVIF_IMAGE;
    item.iItem = info->nmcd.dwItemSpec;
    if (GetItem(&item)) {
      grayed = I_IMAGENONE == item.iImage;
      bold = !grayed && kColAvailable == info->iSubItem;
    }
  }

  // Do not care about themed colors for now
  info->clrText = GetSysColor(grayed ? COLOR_GRAYTEXT : COLOR_BTNTEXT);
  SelectObject(lpNMCustomDraw->hdc, bold ? bold_font_.m_hFont : default_font_.m_hFont);

  return CDRF_NEWFONT;
}

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

HMENU PreferencesDlg::BuildContextMenu(const acfu::source::ptr& source, const file_info_impl& info) const {
  CMenu popup(CreatePopupMenu());
  popup.AppendMenu(MF_STRING, ID_CFU_SINGLE, Tr(ID_CFU_SINGLE));
  if (info.meta_exists("download_page")) {
    popup.AppendMenu(MF_STRING, ID_GOTO_DOWNLOAD_PAGE, Tr(ID_GOTO_DOWNLOAD_PAGE));
  }
  if (info.meta_exists("download_url")) {
    popup.AppendMenu(MF_STRING, ID_DOWNLOAD, Tr(ID_DOWNLOAD));
  }
  source->context_menu_build(popup, ID_CONTEXT_MENU_BASE);

  return popup.Detach();
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

  bool is_any_modified = embedded::Embedded::IsAnyModified();
  changed = changed || is_any_modified;
  need_restart = need_restart || is_any_modified;

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

  SortList();
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

  auto guid_ptr = reinterpret_cast<GUID*>(list_.GetItemData(list_.GetSelectedIndex()));
  if (!guid_ptr) {
    return;
  }

  file_info_impl info;
  static_api_ptr_t<acfu::updates>()->get_info(*guid_ptr, info);
  auto source = acfu::source::g_get(*guid_ptr);
  file_info_impl source_info;
  source->get_info(source_info);
  info.merge_fallback(source_info);

  CMenu popup(BuildContextMenu(source, info));
  ListView_FixContextMenuPoint(list_, point);
  if (unsigned cmd = popup.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, m_hWnd)) {
    if (cmd >= ID_CONTEXT_MENU_BASE) {
      source->context_menu_command(cmd, ID_CONTEXT_MENU_BASE);
    }
    else if (ID_GOTO_DOWNLOAD_PAGE == cmd || ID_DOWNLOAD == cmd) {
      const char* url = info.meta_get(ID_DOWNLOAD == cmd ? "download_url" : "download_page", 0);
      ShellExecute(NULL, L"open", pfc::stringcvt::string_os_from_utf8(url), NULL, NULL, SW_SHOWNORMAL);
    }
    else {
      PostMessage(WM_COMMAND, cmd);
    }
  }
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

  CImageListManaged il;
  il.Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 0, 1);
  il.AddIcon(LoadIcon(NULL, IDI_EXCLAMATION));
  list_.SetImageList(il.Detach(), LVSIL_SMALL);

  ATLVERIFY(kColName == list_.InsertColumn(kColName, Tr(IDS_COL_NAME)));
  ATLVERIFY(kColAvailable == list_.InsertColumn(kColAvailable, Tr(IDS_COL_AVAILABLE)));
  ATLVERIFY(kColInstalled == list_.InsertColumn(kColInstalled, Tr(IDS_COL_INSTALLED)));
  ATLVERIFY(kColModule == list_.InsertColumn(kColModule, Tr(IDS_COL_MODULE)));

  InitList();
  UpdateList();

  split_.SubclassWindow(GetDlgItem(IDC_CFU_ALL));

  static_api_ptr_t<acfu::updates>()->register_callback(this);

  return TRUE;
}

void PreferencesDlg::OnManagedEmbedded(UINT uNotifyCode, int nID, CWindow wndCtl) {
  EmbeddedSourceDlg dlg;
  if (IDOK == dlg.DoModal(m_hWnd)) {
    callback_->on_state_changed();
  }
}

LRESULT PreferencesDlg::OnListColunmClick(LPNMHDR pnmh) {
  LPNMLISTVIEW info = (LPNMLISTVIEW)pnmh;
  if (info->iSubItem == s_sort_column) {
    s_sort_up = !s_sort_up;
  }
  else {
    s_sort_column = info->iSubItem;
  }
  SortList();

  return 0;
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
  ShellExecute(NULL, L"open", TEXT(APP_URL_WHY), NULL, NULL, SW_SHOWNORMAL);
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

void PreferencesDlg::SortList() {
  HeaderControl_SetSortIndicator(list_.GetHeader(), s_sort_column, s_sort_up);
  list_.SortItemsEx(CompareFunc, (LPARAM)list_.m_hWnd);
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
  bool is_newer = acfu::source::g_get(guid)->is_newer(info);

  CString last_version;
  if (auto value = info.meta_get("version", 0)) {
    last_version = pfc::stringcvt::string_os_from_utf8(value);
  }
  else if (is_newer) {
    ATLVERIFY(last_version.LoadString(IDS_GREATER_VERSION));
  }
  list_.SetItemText(index, kColAvailable, last_version);

  LVITEM item = {0};
  item.mask = LVIF_IMAGE;
  item.iItem = index;
  item.iImage = is_newer ? 0 : I_IMAGENONE;
  list_.SetItem(&item);
}
