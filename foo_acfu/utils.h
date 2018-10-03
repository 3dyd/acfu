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

template <class t_service, class t_func>
void for_each_service(t_func&& func) {
  service_enum_t<t_service> e;
  for (service_ptr_t<t_service> ptr; e.next(ptr);) {
    func(ptr);
  }
}

template <class t_service, class t_extension, class t_func>
void for_each_service(t_func&& func) {
  for_each_service<t_service>([&](auto& ptr) {
    service_ptr_t<t_extension> ext;
    if (ptr->service_query_t(ext)) {
      func(ext);
    }
  });
}
