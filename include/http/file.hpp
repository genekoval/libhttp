#pragma once

#include <memory>

namespace http {
    struct file_deleter {
        auto operator()(FILE* file) const noexcept -> void;
    };

    using file_stream = std::unique_ptr<FILE, file_deleter>;

    struct file {
        file_stream stream;
        std::size_t size;
    };
}
