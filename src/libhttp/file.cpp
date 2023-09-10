#include <http/file.hpp>

#include <timber/timber>

namespace http {
    auto file_deleter::operator()(FILE* file) const noexcept -> void {
        if (!file) return;

        if (fclose(file) == 0) {
            TIMBER_DEBUG("Closed file stream ({})", fmt::ptr(file));
            return;
        }

        const auto error = std::error_code(errno, std::generic_category());

        TIMBER_ERROR(
            "Failed to close file stream ({}): {}",
            fmt::ptr(file),
            error.message()
        );
    }
}
