#pragma once

#include "../acfu-sdk/acfu.h"
#include "list_column_auto_size.h"
#include "split_button.h"

class SourcesList: public CListColumnAutoSizeImpl<SourcesList>,
                   public CCustomDraw<SourcesList> {
 public:
  BEGIN_MSG_MAP_EX(SourcesList)
    CHAIN_MSG_MAP(CListColumnAutoSizeImpl<SourcesList>)
    CHAIN_MSG_MAP_ALT(CCustomDraw<SourcesList>, 1)
    DEFAULT_REFLECTION_HANDLER()
  END_MSG_MAP()

  DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
  DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
  DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw);

 private:
  CFont bold_font_;
  CFontHandle default_font_;
};

class PreferencesDlg: public CDialogImpl<PreferencesDlg>,
                      public preferences_page_instance,
                      public acfu::updates::callback {
 public:
  enum { IDD = IDD_PROPERTIES };

  BEGIN_MSG_MAP_EX(PreferencesDlg)
    MSG_WM_CONTEXTMENU(OnContextMenu)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDC_CFU_ALL, OnCfuAll)
    COMMAND_ID_HANDLER_EX(ID_CFU_SINGLE, OnCfuSingle)
    COMMAND_ID_HANDLER_EX(ID_CLEAR_CACHE, OnClearCache)
    COMMAND_ID_HANDLER_EX(ID_MANAGE_EMBEDDED, OnManagedEmbedded)
    COMMAND_HANDLER_EX(IDC_PERIOD_EDIT, EN_CHANGE, OnPeriodEdit)
    NOTIFY_HANDLER_EX(IDC_SOURCE_LIST, LVN_COLUMNCLICK, OnListColunmClick)
    NOTIFY_HANDLER_EX(IDC_SOURCE_LIST, LVN_ITEMCHANGED, OnListItemChanged)
    NOTIFY_HANDLER_EX(IDC_CFU_ALL, BCN_DROPDOWN, OnSplitDropDown)
    NOTIFY_HANDLER_EX(IDC_WHY_LINK, NM_CLICK, OnWhy)
    NOTIFY_HANDLER_EX(IDC_WHY_LINK, NM_RETURN, OnWhy)
    REFLECT_NOTIFICATIONS_HWND_FILTERED(list_)
  END_MSG_MAP()

  PreferencesDlg(preferences_page_callback::ptr callback);
  ~PreferencesDlg();

 private:
  void OnCfuAll(UINT uNotifyCode, int nID, CWindow wndCtl);
  void OnCfuSingle(UINT uNotifyCode, int nID, CWindow wndCtl);
  void OnClearCache(UINT uNotifyCode, int nID, CWindow wndCtl);
  void OnContextMenu(CWindow wnd, CPoint point);
  void OnDestroy();
  BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
  void OnManagedEmbedded(UINT uNotifyCode, int nID, CWindow wndCtl);
  LRESULT OnListColunmClick(LPNMHDR pnmh);
  LRESULT OnListItemChanged(LPNMHDR pnmh);
  void OnPeriodEdit(UINT uNotifyCode, int nID, CWindow wndCtl);
  LRESULT OnSplitDropDown(LPNMHDR pnmh);
  LRESULT OnWhy(LPNMHDR pnmh);

  // preferences_page_instance
  virtual void apply();
  virtual t_uint32 get_state();
  HWND get_wnd() { return m_hWnd; }
  virtual void reset();

  // acfu::cache::callback
  virtual void on_info_changed(const GUID& guid, const file_info& info);

  HMENU BuildContextMenu(const acfu::source::ptr& source, const file_info_impl& info) const;
  pfc::list_t<GUID> GetCheckedSources() const;
  void FreeList();
  void InitList();
  void SortList();
  void UpdateList();
  void UpdateListItem(int index, const GUID& guid, const file_info& info);

  template <class TFunc>
  void ForEachInList(TFunc func) const {
    for (int i = list_.GetItemCount() - 1; i >= 0; i --) {
      if (auto guid_ptr = reinterpret_cast<GUID*>(list_.GetItemData(i))) {
        func(*guid_ptr, i);
      }
    }
  }

 private:
  const preferences_page_callback::ptr callback_;
  CFont title_font_;
  SourcesList list_;
  CSplitButton split_;
  bool clear_cache_ = false;
};
