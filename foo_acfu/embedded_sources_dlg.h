#pragma once

#include <atlctrlx.h>
#include "embedded_source.h"
#include "list_column_auto_size.h"

class EmbeddedSourceDlg: public CDialogImpl<EmbeddedSourceDlg> {
 public:
  enum { IDD = IDD_EMBEDDED_SOURCES };

  BEGIN_MSG_MAP_EX(EmbeddedSourceDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOK)
  END_MSG_MAP()

 private:
  void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
  BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
  void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);

 private:
  CCheckListViewCtrl checkbox_list_;
  CListColumnAutoSize list_;
  pfc::map_t<unsigned, embedded::Embedded::ptr> sources_;
};
