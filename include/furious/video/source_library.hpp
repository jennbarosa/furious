#pragma once

#include "furious/core/media_source.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <memory>

namespace furious {

class SourceLibrary {
public:
    SourceLibrary();
    ~SourceLibrary();

    std::string add_source(const std::string& filepath);
    void add_source_direct(const MediaSource& source);
    void remove_source(std::string_view source_id);

    [[nodiscard]] MediaSource* find_source(std::string_view source_id);
    [[nodiscard]] const MediaSource* find_source(std::string_view source_id) const;

    [[nodiscard]] const std::vector<MediaSource>& sources() const { return sources_; }
    [[nodiscard]] size_t source_count() const { return sources_.size(); }

    void clear();

private:
    std::vector<MediaSource> sources_;

    static std::string generate_id();
    static std::string extract_filename(const std::string& filepath);
    static MediaType detect_media_type(const std::string& filepath);
};

} // namespace furious
