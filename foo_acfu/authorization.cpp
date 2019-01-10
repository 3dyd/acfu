#include "stdafx.h"
#include "utils.h"

// {FD8FEAE0-6E02-4C2A-8AE6-F22F363218BE}
static const GUID guid_branch_updates =
{0xfd8feae0, 0x6e02, 0x4c2a, { 0x8a, 0xe6, 0xf2, 0x2f, 0x36, 0x32, 0x18, 0xbe }};

// {6F895D31-7EA5-4F3B-BF84-2511B1572960}
static const GUID guid_cfg_github_auth =
{0x6f895d31, 0x7ea5, 0x4f3b, { 0xbf, 0x84, 0x25, 0x11, 0xb1, 0x57, 0x29, 0x60 }};
advconfig_string_factory_MT cfg_github_auth("GitHub access token when auto checking for updates", guid_cfg_github_auth, guid_branch_updates, 0, "");

class GitHubAuth: public acfu::authorization {
  virtual void authorize(const char* url, http_request::ptr request, abort_callback& abort) override {
    if (0 == pfc::strcmp_partial(url, "https://api.github.com/")) {
      pfc::string8 token;
      cfg_github_auth.get(token);
      if (!token.is_empty()) {
        token.insert_chars(0, "token ");
        request->add_header("Authorization", token);
      }
    }
  }
};

static service_factory_t<GitHubAuth> g_github_auth;
