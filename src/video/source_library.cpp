#include "furious/video/source_library.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace furious {

SourceLibrary::SourceLibrary() = default;
SourceLibrary::~SourceLibrary() = default;

std::string SourceLibrary::add_source(const std::string& filepath) {
    MediaSource source;
    source.id = generate_id();
    source.filepath = filepath;
    source.name = extract_filename(filepath);
    source.type = detect_media_type(filepath);

    if (source.type == MediaType::Image) {
        source.duration_seconds = 0.0;
        source.fps = 0.0;
    }

    sources_.push_back(source);
    return source.id;
}

void SourceLibrary::add_source_direct(const MediaSource& source) {
    sources_.push_back(source);
}

void SourceLibrary::remove_source(std::string_view source_id) {
    sources_.erase(
        std::remove_if(sources_.begin(), sources_.end(),
            [source_id](const MediaSource& s) {
                return s.id == source_id;
            }),
        sources_.end()
    );
}

MediaSource* SourceLibrary::find_source(std::string_view source_id) {
    auto it = std::find_if(sources_.begin(), sources_.end(),
        [source_id](const MediaSource& s) {
            return s.id == source_id;
        });
    return it != sources_.end() ? &(*it) : nullptr;
}

const MediaSource* SourceLibrary::find_source(std::string_view source_id) const {
    auto it = std::find_if(sources_.begin(), sources_.end(),
        [source_id](const MediaSource& s) {
            return s.id == source_id;
        });
    return it != sources_.end() ? &(*it) : nullptr;
}

void SourceLibrary::clear() {
    sources_.clear();
}

std::string SourceLibrary::generate_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;

    std::stringstream ss;
    ss << "src_" << std::hex << std::setfill('0');
    ss << std::setw(8) << dis(gen);
    return ss.str();
}

std::string SourceLibrary::extract_filename(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filepath.substr(pos + 1);
    }
    return filepath;
}

MediaType SourceLibrary::detect_media_type(const std::string& filepath) {
    std::string lower = filepath;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.ends_with(".png") || lower.ends_with(".jpg") ||
        lower.ends_with(".jpeg") || lower.ends_with(".bmp") ||
        lower.ends_with(".gif") || lower.ends_with(".webp")) {
        return MediaType::Image;
    }

    return MediaType::Video;
}

} // namespace furious
