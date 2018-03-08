#pragma once

typedef CWinTraitsOR<0, WS_EX_CONTROLPARENT> StatusWndTraits;

class StatusWnd: public CWindowImpl<StatusWnd, CWindow, StatusWndTraits>, public acfu::updates::callback {
 public:
  BEGIN_MSG_MAP_EX(StatusWnd)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    MSG_WM_SIZE(OnSize)
    COMMAND_ID_HANDLER_EX(ID_PREFERENCES, OnPreferences)
    NOTIFY_CODE_HANDLER_EX(TTN_GETDISPINFO, OnToolTipText)
  END_MSG_MAP()

  enum Color {
    kColorBackground,
    kColorHighlight,
  };

  void OnUiChanged();

 protected:
  virtual COLORREF GetUiColor(Color color) const = 0;
  virtual CFontHandle GetUiFont() const = 0;
  void DrawHighlightedBackground(CDCHandle dc);

 private:
  int OnCreate(LPCREATESTRUCT lpCreateStruct);
  void OnDestroy();
  BOOL OnEraseBkgnd(CDCHandle dc);
  void OnPreferences(UINT uNotifyCode, int nID, CWindow wndCtl);
  void OnSize(UINT nType, CSize size);
  LRESULT OnToolTipText(LPNMHDR pnmh);

  virtual void on_updates_available(const pfc::list_base_const_t<GUID>& ids);

  HBITMAP CreateButtonBitmap(CDCHandle dc, CFontHandle font, CSize size) const;
  CSize GetButtonSize(CDCHandle dc, CFontHandle font) const;
  void ResetToolBar();
  void UpdateToolBarSize();

 private:
  size_t updates_count_ = 0;
  CToolBarCtrl toolbar_;
};
