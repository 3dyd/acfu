#pragma once

struct Tr {
  Tr(unsigned string_id) {
    ATLVERIFY(text.LoadString(string_id));
  }

  operator const wchar_t*() const {
    return static_cast<const wchar_t*>(text);
  }

  CString text;
};

template <class TService, class TExtension, class TFunc>
void for_each_service(TFunc&& func) {
  acfu::for_each_service<TService>([&](auto& ptr) {
    service_ptr_t<TExtension> ext;
    if (ptr->service_query_t(ext)) {
      func(ext);
    }
  });
}
