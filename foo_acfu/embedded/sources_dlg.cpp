#include "stdafx.h"
#include "resource.h"
#include "sources_dlg.h"
#include "utils.h"

namespace embedded {

void SourceDlg::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl) {
  EndDialog(IDCANCEL);
}

BOOL SourceDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
  CenterWindow();

  checkbox_list_.SubclassWindow(GetDlgItem(IDC_EMBEDDED_SOURCES));
  checkbox_list_.AddColumn(Tr(IDS_SOURCE), 0);

  list_.SubclassWindow(checkbox_list_);
  ::SetWindowTheme(list_, L"explorer", NULL);

  unsigned id = 1;
  for_each_service<acfu::source, Embedded>([&](auto& ptr) {
    sources_[id] = ptr;
    auto index = listview_helper::insert_item(list_, list_.GetItemCount(), ptr->GetName(), id);
    list_.SetCheckState(index, ptr->IsEnabled());
    id ++;
  });

  return TRUE;
}

void SourceDlg::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl) {
  for (int i = list_.GetItemCount() - 1; i >= 0; i --) {
    unsigned id = list_.GetItemData(i);
    sources_[id]->Enable(list_.GetCheckState(i));
  }
  EndDialog(IDOK);
}

} // namespace embedded
