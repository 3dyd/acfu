#include "stdafx.h"
#include "resource.h"
#include "status_wnd.h"
#include "preferences_page.h"

HBITMAP StatusWnd::CreateButtonBitmap(CDCHandle dc, CFontHandle font, CSize size) const {
  if (0 == updates_count_) {
    return NULL;
  }

  BITMAPINFO bmi = {{sizeof(bmi.bmiHeader), size.cx, size.cy, 1, 32, 0, DWORD(size.cx * size.cy)}};
  HBITMAP bitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
  CDC mem_dc(CreateCompatibleDC(dc));
  SelectObjectScope bitmap_scope(mem_dc, bitmap);

  CRect rect(CPoint(), size);
  COLORREF fg_color = GetUiColor(kColorHighlight);
  mem_dc.FillSolidRect(&rect, fg_color);

  COLORREF bg_color = GetUiColor(kColorBackground);
  SelectObjectScope font_scope(mem_dc, font);
  mem_dc.SetBkColor(fg_color);
  mem_dc.SetTextColor(bg_color);

  wchar_t count[20];
  _itow_s(updates_count_, count, _countof(count), 10);
  mem_dc.DrawText(count, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  return bitmap;
}

void StatusWnd::DrawHighlightedBackground(CDCHandle dc) {
  if (updates_count_ > 0) {
    CRect rect;
    toolbar_.GetWindowRect(&rect);
    ScreenToClient(&rect);
    dc.FillSolidRect(&rect, GetUiColor(kColorHighlight));
  }
}

CSize StatusWnd::GetButtonSize(CDCHandle dc, CFontHandle font) const {
  SelectObjectScope font_scope(dc, font);
  CRect rect;
  dc.DrawText(L"99", -1, &rect, DT_CALCRECT);

  rect.right = LONG(1.5 * rect.right);
  const double FI = 1.62;
  if (FI * rect.bottom > rect.right) {
    rect.right = LONG(FI * rect.bottom);
  }
  else {
    rect.right = LONG(double(rect.right) / FI);
  }

  return rect.Size();
}

void StatusWnd::on_updates_available(const pfc::list_base_const_t<GUID>& ids) {
  if (ids.get_count() != updates_count_) {
    updates_count_ = ids.get_count();
    ResetToolBar();
  }
}

int StatusWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  static_api_ptr_t<acfu::updates>()->register_callback(this);

  // May already be created by on_updates_available due to register_callback
  if (!toolbar_) {
    ResetToolBar();
  }

  SetMsgHandled(FALSE);
  return 0;
}

void StatusWnd::OnDestroy() {
  static_api_ptr_t<acfu::updates>()->unregister_callback(this);
  SetMsgHandled(FALSE);
}

BOOL StatusWnd::OnEraseBkgnd(CDCHandle dc) {
  CRect rect;
  GetClientRect(&rect);
  dc.FillSolidRect(&rect, GetUiColor(kColorBackground));
  DrawHighlightedBackground(dc);

  return TRUE;
}

void StatusWnd::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
  CSize size;
  toolbar_.GetMaxSize(&size);
  lpMMI->ptMinTrackSize = CPoint(size.cx, size.cy);
}

void StatusWnd::OnPreferences(UINT uNotifyCode, int nID, CWindow wndCtl) {
  static_api_ptr_t<ui_control>()->show_preferences(guid_preferences_page);
}

void StatusWnd::OnSize(UINT nType, CSize size) {
  UpdateToolBarSize();
  SetMsgHandled(FALSE);
}

LRESULT StatusWnd::OnToolTipText(LPNMHDR pnmh) {
  LPNMTTDISPINFO info = (LPNMTTDISPINFO)pnmh;

  CString tooltip;
  if (0 == updates_count_) {
    tooltip.LoadString(IDS_NO_UPDATES);
  }
  else {
    CString format;
    format.LoadString(IDS_UPDATES_AVAILABLE);
    tooltip.Format(format, updates_count_);
  }
  wcsncpy_s(info->szText, tooltip, _TRUNCATE);

  return 0;
}

void StatusWnd::OnUiChanged() {
  ResetToolBar();
}

void StatusWnd::ResetToolBar() {
  if (toolbar_) {
    toolbar_.DestroyWindow();
  }

  CFont font(AtlCreateBoldFont(GetUiFont()));
  CClientDC dc(m_hWnd);
  CSize size = GetButtonSize(dc.m_hDC, font.m_hFont);
  HBITMAP bitmap = CreateButtonBitmap(dc.m_hDC, font.m_hFont, size);

  DWORD style = CControlWinTraits::GetWndStyle(0) | WS_TABSTOP | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | CCS_NODIVIDER | CCS_NORESIZE;
  toolbar_.Create(m_hWnd, CRect(), 0, style);
  toolbar_.SetButtonStructSize(sizeof(TBBUTTON));
  toolbar_.SetBitmapSize(size);
  toolbar_.AddBitmap(1, bitmap);
  toolbar_.InsertButton(0, ID_PREFERENCES, 0, TBSTATE_ENABLED, 0, 0, NULL);

  UpdateToolBarSize();
  Invalidate();
}

void StatusWnd::UpdateToolBarSize() {
  CSize size;
  toolbar_.GetMaxSize(&size);
  toolbar_.SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}
