#pragma once

///////////////////////////////////////////////////////////////////////////////
// List controls with columns auto sizing to use whole control width without
// horizontal scroll
// CListColumnAutoSize - size to content all columns except one and this one to
// remaining space
// CListColumnAutoSizeEx - to remaining space can be sized several columns
//
// Classes in this file:
//
// CListColumnAutoSizeImplBase<T, TBase, TWinTraits>
// CListColumnAutoSizeImpl<T, TBase, TWinTraits>
// CListColumnAutoSizeExImpl<T, TBase, TWinTraits>
// CListColumnAutoSize
// CListColumnAutoSizeEx

#ifndef HDS_NOSIZING
#define HDS_NOSIZING  0x0800
#endif

template <DWORD t_dwStyle, DWORD t_dwExStyle, DWORD t_dwExListViewStyle>
class CListViewCtrlImplTraits: public ATL::CWinTraits<t_dwStyle, t_dwExStyle> {
 public:
  static DWORD GetExtendedListViewStyle() {
    return t_dwExListViewStyle;
  }
};

typedef CListViewCtrlImplTraits<WS_CHILD | WS_VISIBLE | LVS_REPORT, WS_EX_CLIENTEDGE, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT> CListColumnAutoSizeTraits;

template <class T, class TBase = WTL::CListViewCtrl, class TWinTraits = CListColumnAutoSizeTraits>
class CListColumnAutoSizeImplBase: public ATL::CWindowImpl<T, TBase, TWinTraits> {
 public:
  DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

  enum { kHeaderMsgMapId = 1 };

  BEGIN_MSG_MAP_EX(CListColumnAutoSizeImplBase)
    MSG_WM_KEYDOWN(OnKeyDown)
    MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
    MESSAGE_HANDLER_EX(WM_NCCALCSIZE, OnNcCalcSize)
    MESSAGE_HANDLER_EX(WM_SIZE, OnSize)
    MESSAGE_HANDLER_EX(LVM_INSERTITEMA, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_INSERTITEMW, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_DELETEITEM, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_DELETEALLITEMS, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_SETITEMA, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_SETITEMW, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_SETITEMTEXTA, OnItemChange)
    MESSAGE_HANDLER_EX(LVM_SETITEMTEXTW, OnItemChange)
    NOTIFY_CODE_HANDLER_EX(HDN_BEGINTRACK, OnHeaderBeginTrack)
    NOTIFY_CODE_HANDLER_EX(HDN_DIVIDERDBLCLICK, OnHeaderDividerDblclick)
  ALT_MSG_MAP(T::kHeaderMsgMapId) // header control message map
    MSG_WM_SETCURSOR(OnHeaderSetCursor)
  END_MSG_MAP()

  static DWORD GetExtendedLVStyle() {
    return TWinTraits::GetExtendedListViewStyle();
  }

  CListColumnAutoSizeImplBase(): header_(this, T::kHeaderMsgMapId), auto_update_(true) {}

  // Turn off auto update is helpful at work with large numbers of items
  void EnableAutoUpdate(bool enable) {
    auto_update_ = enable;
    if (auto_update_) {
      UpdateColumnsWidth();
    }
  }

  bool IsAutoUpdateEnabled() const {
    return auto_update_;
  }

  bool IsVariableWidthColumn() const {
    ATLASSERT(0); // should be implemented in child class
    return false;
  }

  BOOL SubclassWindow(HWND hWnd) {
    BOOL res = __super::SubclassWindow(hWnd);
    if (res) {
      T* pT = static_cast<T*>(this);
      pT->PostInit();
      if (pT->IsAutoUpdateEnabled()) {
        pT->UpdateColumnsWidth();
      }
    }
    return res;
  }

  void UpdateColumnsWidth() {
    if (NULL != m_hWnd) {
      T* pT = static_cast<T*>(this);
      pT->SetRedraw(FALSE);
      pT->UpdateFixedWidthColumns();
      pT->UpdateVariableWidthColumns();
      pT->SetRedraw(TRUE);
    }
  }

  void UpdateFixedWidthColumns() {
    // The easiest way to not screw it up is left resizing to the system. But in
    // case of LVSCW_AUTOSIZE_USEHEADER it resizes last column to all remaining
    // space. Workaround - made column not last by adding fake column to the end
    int count = GetHeader().GetItemCount();
    ATLVERIFY(count == InsertColumn(count, _T("")));
    T* pT = static_cast<T*>(this);
    for (int i = 0; i < count; i ++) {
      if (0 != GetColumnWidth(i) && !pT->IsVariableWidthColumn(i)) {
        SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
      }
    }
    ATLVERIFY(DeleteColumn(count));
  }

  void UpdateVariableWidthColumns() {
    ATLASSERT(0); // should be implemented in child class
  }

 protected:
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT lr = DefWindowProc(uMsg, wParam, lParam);
    T* pT = static_cast<T*>(this);
    pT->PostInit();

    return lr;
  }

  LRESULT OnHeaderBeginTrack(LPNMHDR pnmh) {
    SetMsgHandled(!WTL::RunTimeHelper::IsVista());
    return TRUE; // prevent tracking
  }

  LRESULT OnHeaderDividerDblclick(LPNMHDR pnmh) {
    SetMsgHandled(!WTL::RunTimeHelper::IsVista());
    return 0; // prevent reaction (header resizing to content)
  }

  BOOL OnHeaderSetCursor(ATL::CWindow wnd, UINT nHitTest, UINT message) {
    return TRUE; // prevent cursor change over dividers
  }

  LRESULT OnItemChange(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT lr = DefWindowProc(uMsg, wParam, lParam);
    T* pT = static_cast<T*>(this);
    if (pT->IsAutoUpdateEnabled()) {
      pT->UpdateColumnsWidth();
    }
    return lr;
  }

  void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    // Block CTRL + Add to prevent columns width auto adjust    
    SetMsgHandled(VK_ADD == nChar && (GetKeyState(VK_CONTROL) & KF_UP));
  }

  LRESULT OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT lr = DefWindowProc(uMsg, wParam, lParam);
    LPNCCALCSIZE_PARAMS params = (LPNCCALCSIZE_PARAMS)lParam;
    if (GetStyle() & WS_HSCROLL) { // to prevent flickering
      params->rgrc[0].bottom += GetSystemMetrics(SM_CYHSCROLL);
    }
    return lr;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    T* pT = static_cast<T*>(this);
    if (pT->IsAutoUpdateEnabled() && SIZE_MINIMIZED != wParam) {
      pT->UpdateVariableWidthColumns();
    }
    SetMsgHandled(FALSE);
    return 0;
  }

  void PostInit() {
    T* pT = static_cast<T*>(this);
    SetExtendedListViewStyle(pT->GetExtendedLVStyle(), pT->GetExtendedLVStyle());

    ATLASSERT(LVS_REPORT == (LVS_TYPEMASK & GetStyle()));
    if (WTL::RunTimeHelper::IsVista()) {
      GetHeader().ModifyStyle(0, HDS_NOSIZING);
    }
    else {
      ATLVERIFY(header_.SubclassWindow(GetHeader()));
    }
  }

 private:
  ATL::CContainedWindowT<WTL::CHeaderCtrl> header_;
  bool auto_update_;
};

template <class T, class TBase = WTL::CListViewCtrl, class TWinTraits = CListColumnAutoSizeTraits>
class CListColumnAutoSizeImpl: public CListColumnAutoSizeImplBase<T, TBase, TWinTraits> {
 public:
  CListColumnAutoSizeImpl(): variable_width_column_(0) {}

  bool IsVariableWidthColumn(int column) const {
    return variable_width_column_ == column;
  }

  void SetVariableWidthColumn(int index) {
    if (variable_width_column_ != index) {
      variable_width_column_ = index;
      T* pT = static_cast<T*>(this);
      if (pT->IsAutoUpdateEnabled() && NULL != m_hWnd) {
        pT->UpdateColumnsWidth();
      }
    }
  }

  void UpdateVariableWidthColumns() {
    RECT rect = {0};
    GetClientRect(&rect);

    T* pT = static_cast<T*>(this);
    int count = GetHeader().GetItemCount();
    for (int i = 0; i < count; i ++) {
      if (!pT->IsVariableWidthColumn(i)) {
        rect.right -= GetColumnWidth(i);
      }
    }
    SetColumnWidth(variable_width_column_, rect.right - rect.left);
  }

 private:
  int variable_width_column_;
};

template <class T, class TBase = WTL::CListViewCtrl, class TWinTraits = CListColumnAutoSizeTraits>
class CListColumnAutoSizeExImpl: public CListColumnAutoSizeImplBase<T, TBase, TWinTraits> {
 public:
  // Width in percents of available space, 0.0 - 0%, 1.0 - 100%
  void AddVariableWidthColumn(int index, double relative_width) {
    if (-1 != variable_width_columns_.FindKey(index)) {
      ATLASSERT(0); // already exist
    }
    else {
      variable_width_columns_.Add(index, relative_width);

#ifdef _DEBUG // self-check, total width should be <= 100%
      double total_width = 0;
      for (int i = 0; i < variable_width_columns_.GetSize(); i ++) {
        total_width += variable_width_columns_.GetValueAt(i);
      }
      ATLASSERT(1.0 >= total_width);
#endif

      T* pT = static_cast<T*>(this);
      if (pT->IsAutoUpdateEnabled()) {
        pT->UpdateColumnsWidth();
      }
    }
  }

  void ClearVariableWidthColumns() {
    variable_width_columns_.RemoveAll();
  }

  bool IsVariableWidthColumn(int column) const {
    return -1 != variable_width_columns_.FindKey(column);
  }

  void UpdateVariableWidthColumns() {
    if (0 != variable_width_columns_.GetSize()) {
      RECT rect = {0};
      GetClientRect(&rect);

      T* pT = static_cast<T*>(this);
      int count = GetHeader().GetItemCount();
      for (int i = 0; i < count; i ++) {
        if (!pT->IsVariableWidthColumn(i)) {
          rect.right -= GetColumnWidth(i);
        }
      }

      int total_width = rect.right - rect.left;
      for (int i = 0; i < variable_width_columns_.GetSize(); i ++) {
        int width = int(variable_width_columns_.GetValueAt(i) * total_width);
        SetColumnWidth(variable_width_columns_.GetKeyAt(i), width);
      }
    }
  }

 private:
  ATL::CSimpleMap<int, double> variable_width_columns_;
};

class CListColumnAutoSize: public CListColumnAutoSizeImpl<CListColumnAutoSize> {
 public:
  DECLARE_WND_SUPERCLASS(_T("ListColumnAutoSize"), GetWndClassName())
};

class CListColumnAutoSizeEx: public CListColumnAutoSizeExImpl<CListColumnAutoSizeEx> {
 public:
  DECLARE_WND_SUPERCLASS(_T("ListColumnAutoSizeEx"), GetWndClassName())
};
