#pragma once

// Split button has appeared in Vista so for XP it needs to be implemented
// manually. This class wraps all that need to use real split button in
// OS>=Vista and to create a control similar to split button in XP
class CSplitButton: public CWindowImpl<CSplitButton, CButton> {
 public:
  BEGIN_MSG_MAP_EX(CSplitButton)
    MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
    if (!RunTimeHelper::IsVista()) {
      MSG_WM_GETDLGCODE(OnGetDlgCode)
      MSG_WM_KEYDOWN(OnKeyDown)
      MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
      MSG_WM_LBUTTONDOWN(OnLButtonDown)
      MSG_WM_PAINT(OnPaint)
      MESSAGE_HANDLER_EX(WM_SETTEXT, OnSetText)
      MESSAGE_HANDLER_EX(WM_ENABLE, RedrawByMessage)
      MESSAGE_HANDLER_EX(WM_UPDATEUISTATE, RedrawByMessage)
      MESSAGE_HANDLER_EX(BCM_SETSPLITINFO, OnSetSplitInfo)
    }
  END_MSG_MAP()

  CSplitButton(): style_(0), ignore_set_sext_(false) {}

  // After return rc_draw contain arrow rectangle
  static void DrawArrow(CDCHandle dc, RECT& rc_draw, COLORREF clr_arrow) {
    CRect rc_arrow;
    rc_arrow.left = rc_draw.left + ::GetSystemMetrics(SM_CXEDGE);
    rc_arrow.top = (rc_draw.bottom + rc_draw.top) / 2 - kArrowSizeY / 2;
    rc_arrow.right = rc_arrow.left + kArrowSizeX;
    rc_arrow.bottom = (rc_draw.bottom + rc_draw.top) / 2 + kArrowSizeY / 2;

    POINT pt_arrow[3];
    pt_arrow[0].x = rc_arrow.left;
    pt_arrow[0].y = rc_arrow.top;
    pt_arrow[1].x = rc_arrow.right;
    pt_arrow[1].y = rc_arrow.top;
    pt_arrow[2].x = (rc_arrow.left + rc_arrow.right) / 2;
    pt_arrow[2].y = rc_arrow.bottom;

    dc.SaveDC();

    CBrush br_arrow;
    br_arrow.CreateSolidBrush(clr_arrow);
    CPen pen_arrow;
    pen_arrow.CreatePen(PS_SOLID, 0, clr_arrow);
    dc.SelectBrush(br_arrow);
    dc.SelectPen(pen_arrow);

    dc.SetPolyFillMode(WINDING);
    dc.Polygon(pt_arrow, 3);

    dc.RestoreDC(-1);
    rc_draw = rc_arrow;
    rc_draw.right += 1;
    rc_draw.bottom += 1;
  }

  // After return rc_draw contain splitter rectangle
  static void DrawSplitter(CDCHandle dc, RECT& rc_draw) {
    // Draw separator
    rc_draw.right = rc_draw.left;
    InflateRect(&rc_draw, 0, - 2 * ::GetSystemMetrics(SM_CXEDGE));
    dc.DrawEdge(&rc_draw, EDGE_ETCHED, BF_RIGHT);
    rc_draw.left -= ::GetSystemMetrics(SM_CXEDGE);
  }

  BOOL SubclassWindow(HWND hwnd) {
    if (__super::SubclassWindow(hwnd)) {
      Init();
      return TRUE;
    }
    return FALSE;
  }

 protected:
  void AppearanceFix() {
    ATLASSERT(!ignore_set_sext_);
    CString text;
    GetWindowText(text);
    if (!text.IsEmpty()) {
      text += L"   ";
      ignore_set_sext_ = true;
      SetWindowText(text);
      ignore_set_sext_ = false;
    }
  }

  void DoPaint(CDCHandle dc) {
    DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, 0);

    // Draw the arrow
    CRect rc_arrow = GetArrowArea(), rc_splitter = rc_arrow;
    COLORREF clr_arrow = GetSysColor(IsWindowEnabled() ? COLOR_BTNTEXT : COLOR_GRAYTEXT);

    DrawArrow(dc, rc_arrow, clr_arrow);
    if (0 == (BCSS_NOSPLIT & style_)) {
      DrawSplitter(dc, rc_splitter);
    }
  }

  CRect GetArrowArea() {
    CRect rc_draw;
    GetClientRect(&rc_draw);
    rc_draw.left = rc_draw.right - kArrowSizeX - 4 * ::GetSystemMetrics(SM_CXEDGE);

    return rc_draw;
  }

  void Init() {
    if (RunTimeHelper::IsVista()) {
      SetButtonStyle(BS_SPLITBUTTON);
    }
    else {
      AppearanceFix();
    }
  }

  void NotifyDropDown() {
    if (0 == (BCSS_NOSPLIT & style_)) {
      NMBCDROPDOWN param = {0};
      param.hdr.code = BCN_DROPDOWN;
      param.hdr.hwndFrom = m_hWnd;
      param.hdr.idFrom = GetDlgCtrlID();
      param.rcButton = GetArrowArea(); // TODO: check what it really should contain
      GetParent().SendMessage(WM_NOTIFY, param.hdr.idFrom, reinterpret_cast<LPARAM>(&param));
    }
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Init();
    SetMsgHandled(FALSE);
    return 0;
  }

  UINT OnGetDlgCode(LPMSG lpMsg) {
    UINT code = (UINT)DefWindowProc();
    return code | DLGC_WANTARROWS;
  }

  void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    if (VK_DOWN == nChar && 0 != (GetKeyState(VK_CONTROL) & KF_UP)) {
      NotifyDropDown();
    }
    else {
      SetMsgHandled(FALSE);
    }
  }

  void OnLButtonDblClk(UINT nFlags, CPoint point) {
    // Nothing, just to block this message
  }

  void OnLButtonDown(UINT nFlags, CPoint point) {
    if (GetArrowArea().PtInRect(point)) {
      NotifyDropDown();
    }
    else {
      SetMsgHandled(FALSE);
    }
  }

  void OnPaint(CDCHandle dc) {
    if (dc) {
      DoPaint(dc);
    }
    else {
      CPaintDC dc(m_hWnd);
      CMemoryDC mem_dc(dc.m_hDC, dc.m_ps.rcPaint);
      DoPaint(mem_dc.m_hDC);
    }
  }

  LRESULT OnSetSplitInfo(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PBUTTON_SPLITINFO info = (PBUTTON_SPLITINFO)lParam;
    if (BCSIF_STYLE & info->mask) {
      style_ = info->uSplitStyle;
      if (m_hWnd) {
        Invalidate();
      }
    }
    return TRUE;
  }

  LRESULT OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT res = DefWindowProc(uMsg, wParam, lParam);
    if (!ignore_set_sext_ && !RunTimeHelper::IsVista()) {
      AppearanceFix();
    }
    return res;
  }

  LRESULT RedrawByMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Invalidate();
    SetMsgHandled(FALSE);
    return 0;
  }

 private:
  static const int kArrowSizeX = 8;
  static const int kArrowSizeY = 4;
  UINT style_;
  bool ignore_set_sext_;
};
