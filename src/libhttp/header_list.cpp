#include <http/request.h>

#include <curl/curl.h>
#include <fmt/core.h>

namespace http {
    header_list::~header_list() { curl_slist_free_all(list); }

    auto header_list::add(const char* item) -> void {
        list = curl_slist_append(list, item);
    }

    auto header_list::add(
        std::string_view key,
        std::string_view value
    ) -> void {
        add(fmt::format("{}: {}", key, value).c_str());
    }

    auto header_list::data() -> curl_slist* { return list; }

    auto header_list::empty() const -> bool { return list == nullptr; }
}
