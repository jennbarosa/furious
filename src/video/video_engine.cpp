#include "furious/video/video_engine.hpp"
#include "furious/video/video_decoder.hpp"
#include "furious/video/gl_functions.hpp"
#include <GLFW/glfw3.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cmath>

namespace furious {

struct SourceState {
    std::unique_ptr<VideoDecoder> decoder;
    int width = 0;
    int height = 0;
    MediaType type = MediaType::Video;
};

constexpr double MAX_DECODE_RATE = 30.0;
constexpr double MIN_DECODE_INTERVAL = 1.0 / MAX_DECODE_RATE;

struct ClipState {
    std::string source_id;
    uint32_t texture_id = 0;
    int width = 0;
    int height = 0;
    std::vector<uint8_t> frame_buffer;
    double last_requested_time = -1.0;
    double last_decode_wall_time = 0.0;
    bool texture_needs_update = false;
    bool requested_this_frame = false;
    bool has_valid_frame = false;
    bool prebuilt = false;

    double loop_source_start = 0.0;
    double loop_duration = 0.0;
    double loop_frame_duration = 0.0;
    double loop_next_decode_time = 0.0;
    bool loop_cache_complete = false;
    std::vector<std::vector<uint8_t>> loop_frames;
    size_t current_loop_frame_index = 0;
    bool use_loop_frame = false;

    // YUV GPU conversion state
    DecodedFrame yuv_frame;
    uint32_t y_texture = 0;
    uint32_t uv_texture = 0;
    uint32_t v_texture = 0;
    bool yuv_needs_update = false;
};

// Shader sources
static const char* yuv_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main() {
    vTexCoord = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* nv12_fragment_shader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D y_tex;
uniform sampler2D uv_tex;
void main() {
    float y = texture(y_tex, vTexCoord).r;
    vec2 uv = texture(uv_tex, vTexCoord).rg;
    float cb = uv.r - 0.5;
    float cr = uv.g - 0.5;
    float r = y + 1.402 * cr;
    float g = y - 0.344136 * cb - 0.714136 * cr;
    float b = y + 1.772 * cb;
    fragColor = vec4(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0), 1.0);
}
)";

static const char* yuv420p_fragment_shader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;
void main() {
    float y = texture(y_tex, vTexCoord).r;
    float cb = texture(u_tex, vTexCoord).r - 0.5;
    float cr = texture(v_tex, vTexCoord).r - 0.5;
    float r = y + 1.402 * cr;
    float g = y - 0.344136 * cb - 0.714136 * cr;
    float b = y + 1.772 * cb;
    fragColor = vec4(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0), 1.0);
}
)";

struct YUVShaderState {
    GLuint nv12_program = 0;
    GLuint yuv420p_program = 0;
    GLuint fbo = 0;
    GLuint quad_vao = 0;
    GLuint quad_vbo = 0;
    bool initialized = false;

    // Uniform locations
    GLint nv12_y_loc = -1;
    GLint nv12_uv_loc = -1;
    GLint yuv420p_y_loc = -1;
    GLint yuv420p_u_loc = -1;
    GLint yuv420p_v_loc = -1;
};

struct VideoEngine::Impl {
    std::unordered_map<std::string, SourceState> sources;
    std::unordered_map<std::string, ClipState> clips;
    std::unordered_set<std::string> active_clip_ids;
    bool initialized = false;

    GLFunctions gl;
    YUVShaderState shader;
    bool gpu_yuv_available = false;
    std::string decode_error;
};

namespace {

GLuint compile_shader(GLFunctions& gl, GLenum type, const char* source) {
    GLuint shader = gl.CreateShader(type);
    gl.ShaderSource(shader, 1, &source, nullptr);
    gl.CompileShader(shader);

    GLint status = 0;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        gl.GetShaderInfoLog(shader, 512, nullptr, log);
        std::fprintf(stderr, "[YUV Shader] Compile error: %s\n", log);
        gl.DeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint create_program(GLFunctions& gl, const char* vert_src, const char* frag_src) {
    GLuint vert = compile_shader(gl, GL_VERTEX_SHADER, vert_src);
    if (!vert) return 0;

    GLuint frag = compile_shader(gl, GL_FRAGMENT_SHADER, frag_src);
    if (!frag) {
        gl.DeleteShader(vert);
        return 0;
    }

    GLuint program = gl.CreateProgram();
    gl.AttachShader(program, vert);
    gl.AttachShader(program, frag);
    gl.LinkProgram(program);
    gl.DeleteShader(vert);
    gl.DeleteShader(frag);

    GLint status = 0;
    gl.GetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        gl.GetProgramInfoLog(program, 512, nullptr, log);
        std::fprintf(stderr, "[YUV Shader] Link error: %s\n", log);
        gl.DeleteProgram(program);
        return 0;
    }
    return program;
}

void init_yuv_shaders(GLFunctions& gl, YUVShaderState& s) {
    s.nv12_program = create_program(gl, yuv_vertex_shader, nv12_fragment_shader);
    s.yuv420p_program = create_program(gl, yuv_vertex_shader, yuv420p_fragment_shader);

    if (s.nv12_program) {
        s.nv12_y_loc = gl.GetUniformLocation(s.nv12_program, "y_tex");
        s.nv12_uv_loc = gl.GetUniformLocation(s.nv12_program, "uv_tex");
    }
    if (s.yuv420p_program) {
        s.yuv420p_y_loc = gl.GetUniformLocation(s.yuv420p_program, "y_tex");
        s.yuv420p_u_loc = gl.GetUniformLocation(s.yuv420p_program, "u_tex");
        s.yuv420p_v_loc = gl.GetUniformLocation(s.yuv420p_program, "v_tex");
    }

    gl.GenFramebuffers(1, &s.fbo);

    // Fullscreen quad: two triangles
    float quad_verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };

    gl.GenVertexArrays(1, &s.quad_vao);
    gl.GenBuffers(1, &s.quad_vbo);
    gl.BindVertexArray(s.quad_vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, s.quad_vbo);
    gl.BufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    gl.BindVertexArray(0);

    s.initialized = (s.nv12_program || s.yuv420p_program) && s.fbo && s.quad_vao;
}

void cleanup_yuv_shaders(GLFunctions& gl, YUVShaderState& s) {
    if (s.nv12_program) gl.DeleteProgram(s.nv12_program);
    if (s.yuv420p_program) gl.DeleteProgram(s.yuv420p_program);
    if (s.fbo) gl.DeleteFramebuffers(1, &s.fbo);
    if (s.quad_vao) gl.DeleteVertexArrays(1, &s.quad_vao);
    if (s.quad_vbo) gl.DeleteBuffers(1, &s.quad_vbo);
    s = {};
}

void create_yuv_texture(GLuint& tex, int width, int height, GLenum format, GLenum internal_format) {
    if (tex == 0) {
        glGenTextures(1, &tex);
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void delete_clip_yuv_textures(ClipState& clip) {
    if (clip.y_texture) { glDeleteTextures(1, &clip.y_texture); clip.y_texture = 0; }
    if (clip.uv_texture) { glDeleteTextures(1, &clip.uv_texture); clip.uv_texture = 0; }
    if (clip.v_texture) { glDeleteTextures(1, &clip.v_texture); clip.v_texture = 0; }
}

void create_clip_texture(ClipState& clip) {
    glGenTextures(1, &clip.texture_id);
    glBindTexture(GL_TEXTURE_2D, clip.texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 clip.width, clip.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ensure_clip_texture(ClipState& clip, int new_width, int new_height) {
    if (new_width != clip.width || new_height != clip.height) {
        clip.width = new_width;
        clip.height = new_height;
        if (clip.texture_id != 0) {
            glDeleteTextures(1, &clip.texture_id);
        }
        create_clip_texture(clip);
        delete_clip_yuv_textures(clip);
    }
}

} // anonymous namespace

VideoEngine::VideoEngine() : impl_(std::make_unique<Impl>()) {}

VideoEngine::~VideoEngine() {
    shutdown();
}

bool VideoEngine::initialize() {
    impl_->initialized = true;

    if (impl_->gl.load()) {
        init_yuv_shaders(impl_->gl, impl_->shader);
        impl_->gpu_yuv_available = impl_->shader.initialized;
        if (impl_->gpu_yuv_available) {
            std::printf("[VideoEngine] GPU YUV conversion enabled\n");
        } else {
            impl_->decode_error = "GPU YUV shader initialization failed. "
                "Please report this at https://github.com/jennbarosa/furious/issues";
        }
    } else {
        impl_->decode_error = "OpenGL function loading failed. "
            "Please report this at https://github.com/jennbarosa/furious/issues";
    }

    return true;
}

void VideoEngine::shutdown() {
    for (auto& [id, state] : impl_->clips) {
        if (state.texture_id != 0) {
            glDeleteTextures(1, &state.texture_id);
        }
        delete_clip_yuv_textures(state);
    }
    impl_->clips.clear();

    for (auto& [id, state] : impl_->sources) {
        if (state.decoder) {
            state.decoder->close();
        }
    }
    impl_->sources.clear();

    if (impl_->gpu_yuv_available) {
        cleanup_yuv_shaders(impl_->gl, impl_->shader);
    }

    impl_->initialized = false;
}

void VideoEngine::register_source(const MediaSource& source) {
    if (impl_->sources.count(source.id) > 0) {
        return;
    }

    SourceState state;
    state.type = source.type;

    if (source.type == MediaType::Video) {
        state.decoder = std::make_unique<VideoDecoder>();
        if (!state.decoder->open(source.filepath)) {
            return;
        }
        state.width = state.decoder->width();
        state.height = state.decoder->height();
        if (state.width <= 0 || state.height <= 0) {
            state.decoder->close();
            return;
        }
    } else {
        state.width = source.width > 0 ? source.width : 256;
        state.height = source.height > 0 ? source.height : 256;
    }

    impl_->sources[source.id] = std::move(state);
}

void VideoEngine::unregister_source(const std::string& source_id) {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return;

    for (auto clip_it = impl_->clips.begin(); clip_it != impl_->clips.end();) {
        if (clip_it->second.source_id == source_id) {
            if (clip_it->second.texture_id != 0) {
                glDeleteTextures(1, &clip_it->second.texture_id);
            }
            delete_clip_yuv_textures(clip_it->second);
            clip_it = impl_->clips.erase(clip_it);
        } else {
            ++clip_it;
        }
    }

    if (it->second.decoder) {
        it->second.decoder->close();
    }
    impl_->sources.erase(it);
}

void VideoEngine::clear_sources() {
    for (auto& [id, state] : impl_->clips) {
        if (state.texture_id != 0) {
            glDeleteTextures(1, &state.texture_id);
        }
        delete_clip_yuv_textures(state);
    }
    impl_->clips.clear();

    for (auto& [id, state] : impl_->sources) {
        if (state.decoder) {
            state.decoder->close();
        }
    }
    impl_->sources.clear();
}

void VideoEngine::begin_frame() {
    for (auto& [id, state] : impl_->clips) {
        state.requested_this_frame = false;
    }
    impl_->active_clip_ids.clear();
}

void VideoEngine::request_frame(const std::string& clip_id, const std::string& source_id, double local_seconds) {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) {
        return;
    }

    SourceState& source = source_it->second;

    if (source.type == MediaType::Image) {
        impl_->active_clip_ids.insert(clip_id);
        return;
    }

    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;
        create_clip_texture(clip_state);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
    }

    ClipState& clip = clip_it->second;
    clip.requested_this_frame = true;
    clip.use_loop_frame = false;
    impl_->active_clip_ids.insert(clip_id);

    double fps = source.decoder ? source.decoder->fps() : 30.0;
    if (fps <= 0.0) fps = 30.0;
    double frame_duration = 1.0 / fps;

    int64_t last_frame = static_cast<int64_t>(clip.last_requested_time / frame_duration);
    int64_t curr_frame = static_cast<int64_t>(local_seconds / frame_duration);

    if (last_frame == curr_frame && clip.has_valid_frame) {
        return;
    }

    clip.last_requested_time = local_seconds;

    if (!source.decoder) return;

    auto t_decode_start = std::chrono::high_resolution_clock::now();

    if (source.decoder->seek_and_decode_yuv(local_seconds, clip.yuv_frame)) {
        auto t_decode_end = std::chrono::high_resolution_clock::now();
        auto decode_ms = std::chrono::duration<double, std::milli>(t_decode_end - t_decode_start).count();
        if (decode_ms > 10.0) {
            std::printf("[PROFILE] request_frame seek_and_decode_yuv: %.2fms clip=%s time=%.3fs\n",
                       decode_ms, clip_id.c_str(), local_seconds);
        }

        ensure_clip_texture(clip, clip.yuv_frame.width, clip.yuv_frame.height);
        clip.yuv_needs_update = true;
        clip.has_valid_frame = true;
    } else if (impl_->decode_error.empty()) {
        std::string details = "Clip: " + clip_id +
            "\nDecoder: " + source.decoder->decoder_type() +
            "\nPixel format: " + std::to_string(source.decoder->pixel_format()) +
            "\nResolution: " + std::to_string(source.decoder->width()) + "x" + std::to_string(source.decoder->height());
        impl_->decode_error = "Video decode failed. The pixel format may be unsupported.\n\n" +
            details +
            "\n\nPlease report this at https://github.com/jennbarosa/furious/issues";
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    if (total_ms > 16.0) {
        std::printf("[PROFILE] request_frame TOTAL: %.2fms clip=%s\n", total_ms, clip_id.c_str());
    }
}

void VideoEngine::prefetch_clip(const std::string& clip_id, const std::string& source_id, double start_seconds) {
    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) return;

    SourceState& source = source_it->second;
    if (source.type == MediaType::Image) return;
    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it != impl_->clips.end()) return;

    ClipState clip_state;
    clip_state.source_id = source_id;
    clip_state.width = source.width;
    clip_state.height = source.height;
    create_clip_texture(clip_state);

    if (source.decoder) {
        if (source.decoder->seek_and_decode_yuv(start_seconds, clip_state.yuv_frame)) {
            clip_state.yuv_needs_update = true;
            clip_state.has_valid_frame = true;
            clip_state.last_requested_time = start_seconds;
        }
    }

    clip_state.prebuilt = true;
    impl_->clips[clip_id] = std::move(clip_state);
}

bool VideoEngine::is_clip_cached(const std::string& clip_id) const {
    return impl_->clips.find(clip_id) != impl_->clips.end();
}

bool VideoEngine::is_loop_cache_complete(const std::string& clip_id) const {
    auto it = impl_->clips.find(clip_id);
    if (it == impl_->clips.end()) return false;
    return it->second.loop_cache_complete;
}

void VideoEngine::prebuild_loop_cache(const std::string& clip_id, const std::string& source_id,
                                       double source_start_seconds, double loop_duration_seconds) {
    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) {
        return;
    }

    SourceState& source = source_it->second;
    if (source.type == MediaType::Image) return;
    if (!source.decoder) return;
    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;
        create_clip_texture(clip_state);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
    }

    ClipState& clip = clip_it->second;

    double fps = source.decoder->fps();
    if (fps <= 0.0) fps = 30.0;

    clip.loop_frames.clear();
    clip.loop_source_start = source_start_seconds;
    clip.loop_duration = loop_duration_seconds;
    clip.loop_frame_duration = 1.0 / fps;
    clip.loop_next_decode_time = source_start_seconds;
    clip.loop_cache_complete = false;

    // Loop cache uses RGBA path (frames are stored and reused)
    size_t buffer_size = static_cast<size_t>(source.width) * static_cast<size_t>(source.height) * 4;
    std::vector<uint8_t> frame_buffer(buffer_size);
    double end_time = source_start_seconds + loop_duration_seconds + clip.loop_frame_duration;

    while (clip.loop_next_decode_time < end_time && clip.loop_frames.size() < 120) {
        if (source.decoder->seek_and_decode(clip.loop_next_decode_time, frame_buffer)) {
            clip.loop_frames.push_back(std::move(frame_buffer));
            frame_buffer.resize(buffer_size);
        }
        clip.loop_next_decode_time += clip.loop_frame_duration;
    }

    clip.loop_cache_complete = true;

    ensure_clip_texture(clip, source.decoder->width(), source.decoder->height());

    if (!clip.loop_frames.empty()) {
        clip.current_loop_frame_index = 0;
        clip.use_loop_frame = true;
        clip.has_valid_frame = true;
        clip.texture_needs_update = true;
    }

    clip.prebuilt = true;
}

void VideoEngine::request_looped_frame(const std::string& clip_id, const std::string& source_id,
                                        double source_start_seconds, double loop_duration_seconds,
                                        double position_in_loop) {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) return;

    SourceState& source = source_it->second;

    if (source.type == MediaType::Image) {
        impl_->active_clip_ids.insert(clip_id);
        return;
    }

    if (!source.decoder) return;

    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;
        create_clip_texture(clip_state);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
    }

    ClipState& clip = clip_it->second;
    clip.requested_this_frame = true;
    impl_->active_clip_ids.insert(clip_id);

    constexpr double EPSILON = 0.0001;
    bool start_changed = std::abs(clip.loop_source_start - source_start_seconds) > EPSILON;

    bool duration_exceeds_cache = loop_duration_seconds > clip.loop_duration + EPSILON;
    bool needs_rebuild = start_changed || (duration_exceeds_cache && clip.loop_cache_complete);

    if (is_interactive_ && needs_rebuild) {
        return;
    }

    if (needs_rebuild) {
        double fps = source.decoder->fps();
        if (fps <= 0.0) fps = 30.0;

        clip.loop_frames.clear();
        clip.loop_source_start = source_start_seconds;
        clip.loop_duration = loop_duration_seconds;
        clip.loop_frame_duration = 1.0 / fps;
        clip.loop_next_decode_time = source_start_seconds;
        clip.loop_cache_complete = false;
    }

    if (!clip.loop_cache_complete && clip.loop_frames.size() < 120) {
        auto t_cache_start = std::chrono::high_resolution_clock::now();
        constexpr size_t MAX_FRAMES_PER_CALL = 5;

        size_t buffer_size = static_cast<size_t>(source.width) * static_cast<size_t>(source.height) * 4;
        std::vector<uint8_t> frame_buffer(buffer_size);
        double end_time = clip.loop_source_start + clip.loop_duration + clip.loop_frame_duration;
        size_t frames_decoded = 0;

        while (clip.loop_next_decode_time < end_time && frames_decoded < MAX_FRAMES_PER_CALL) {
            if (source.decoder->seek_and_decode(clip.loop_next_decode_time, frame_buffer)) {
                clip.loop_frames.push_back(std::move(frame_buffer));
                frame_buffer.resize(buffer_size);
            }
            clip.loop_next_decode_time += clip.loop_frame_duration;
            ++frames_decoded;

            if (clip.loop_frames.size() >= 120) break;
        }
        auto t_cache_end = std::chrono::high_resolution_clock::now();
        auto cache_ms = std::chrono::duration<double, std::milli>(t_cache_end - t_cache_start).count();
        if (cache_ms > 10.0) {
            std::printf("[PROFILE] request_looped_frame cache_decode: %.2fms frames=%zu clip=%s\n",
                       cache_ms, frames_decoded, clip_id.c_str());
        }

        if (clip.loop_next_decode_time >= end_time || clip.loop_frames.size() >= 120) {
            clip.loop_cache_complete = true;
        }

        ensure_clip_texture(clip, source.decoder->width(), source.decoder->height());
    }

    if (!clip.loop_frames.empty() && clip.loop_frame_duration > 0.0) {
        size_t index = static_cast<size_t>(position_in_loop / clip.loop_frame_duration);
        if (index >= clip.loop_frames.size()) index = clip.loop_frames.size() - 1;
        clip.current_loop_frame_index = index;
        clip.use_loop_frame = true;
        clip.texture_needs_update = true;
        clip.has_valid_frame = true;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    if (total_ms > 16.0) {
        std::printf("[PROFILE] request_looped_frame TOTAL: %.2fms clip=%s\n", total_ms, clip_id.c_str());
    }
}

void VideoEngine::update() {
    auto& gl = impl_->gl;

    for (auto& [id, clip] : impl_->clips) {
        // GPU YUV conversion path
        if (clip.yuv_needs_update && impl_->gpu_yuv_available) {
            const DecodedFrame& f = clip.yuv_frame;
            int half_w = f.width / 2;
            int half_h = f.height / 2;

            // Upload Y plane
            if (clip.y_texture == 0) {
                create_yuv_texture(clip.y_texture, f.width, f.height, GL_RED, GL_R8);
            }
            glBindTexture(GL_TEXTURE_2D, clip.y_texture);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, f.y_stride);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f.width, f.height,
                            GL_RED, GL_UNSIGNED_BYTE, f.y_plane.data());
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            if (f.format == YUVFormat::NV12) {
                // Upload UV plane (interleaved)
                if (clip.uv_texture == 0) {
                    create_yuv_texture(clip.uv_texture, half_w, half_h, GL_RG, GL_RG8);
                }
                glBindTexture(GL_TEXTURE_2D, clip.uv_texture);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, f.uv_stride / 2);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, half_w, half_h,
                                GL_RG, GL_UNSIGNED_BYTE, f.uv_plane.data());
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            } else if (f.format == YUVFormat::YUV420P) {
                // Upload U plane
                if (clip.uv_texture == 0) {
                    create_yuv_texture(clip.uv_texture, half_w, half_h, GL_RED, GL_R8);
                }
                glBindTexture(GL_TEXTURE_2D, clip.uv_texture);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, f.uv_stride);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, half_w, half_h,
                                GL_RED, GL_UNSIGNED_BYTE, f.uv_plane.data());
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                // Upload V plane
                if (clip.v_texture == 0) {
                    create_yuv_texture(clip.v_texture, half_w, half_h, GL_RED, GL_R8);
                }
                glBindTexture(GL_TEXTURE_2D, clip.v_texture);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, f.v_stride);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, half_w, half_h,
                                GL_RED, GL_UNSIGNED_BYTE, f.v_plane.data());
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            }

            glBindTexture(GL_TEXTURE_2D, 0);

            // Render YUV→RGBA via FBO
            GLint prev_fbo = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
            GLint prev_viewport[4];
            glGetIntegerv(GL_VIEWPORT, prev_viewport);

            gl.BindFramebuffer(GL_FRAMEBUFFER, impl_->shader.fbo);
            gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, clip.texture_id, 0);

            glViewport(0, 0, clip.width, clip.height);

            GLuint program = 0;
            if (f.format == YUVFormat::NV12 && impl_->shader.nv12_program) {
                program = impl_->shader.nv12_program;
                gl.UseProgram(program);
                gl.ActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, clip.y_texture);
                gl.Uniform1i(impl_->shader.nv12_y_loc, 0);
                gl.ActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, clip.uv_texture);
                gl.Uniform1i(impl_->shader.nv12_uv_loc, 1);
            } else if (f.format == YUVFormat::YUV420P && impl_->shader.yuv420p_program) {
                program = impl_->shader.yuv420p_program;
                gl.UseProgram(program);
                gl.ActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, clip.y_texture);
                gl.Uniform1i(impl_->shader.yuv420p_y_loc, 0);
                gl.ActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, clip.uv_texture);
                gl.Uniform1i(impl_->shader.yuv420p_u_loc, 1);
                gl.ActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, clip.v_texture);
                gl.Uniform1i(impl_->shader.yuv420p_v_loc, 2);
            }

            if (program) {
                gl.BindVertexArray(impl_->shader.quad_vao);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                gl.BindVertexArray(0);
            }

            gl.UseProgram(0);
            gl.ActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            gl.BindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));
            glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);

            clip.yuv_needs_update = false;
            clip.texture_needs_update = false;
            continue;
        }

        // CPU RGBA upload path (fallback + loop frames)
        if (clip.texture_needs_update) {
            const uint8_t* frame_data = nullptr;
            size_t frame_size = 0;

            if (clip.use_loop_frame && clip.current_loop_frame_index < clip.loop_frames.size()) {
                const auto& cached_frame = clip.loop_frames[clip.current_loop_frame_index];
                frame_data = cached_frame.data();
                frame_size = cached_frame.size();
            } else if (!clip.frame_buffer.empty()) {
                frame_data = clip.frame_buffer.data();
                frame_size = clip.frame_buffer.size();
            }

            size_t expected_size = static_cast<size_t>(clip.width * clip.height * 4);
            if (frame_data == nullptr || frame_size != expected_size) {
                clip.texture_needs_update = false;
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, clip.texture_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            clip.width, clip.height,
                            GL_RGBA, GL_UNSIGNED_BYTE,
                            frame_data);
            glBindTexture(GL_TEXTURE_2D, 0);
            clip.texture_needs_update = false;
        }
    }

    for (auto it = impl_->clips.begin(); it != impl_->clips.end();) {
        bool is_active = impl_->active_clip_ids.find(it->first) != impl_->active_clip_ids.end();
        if (!is_active && !it->second.prebuilt) {
            if (it->second.texture_id != 0) {
                glDeleteTextures(1, &it->second.texture_id);
            }
            delete_clip_yuv_textures(it->second);
            it = impl_->clips.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t VideoEngine::get_texture(const std::string& clip_id) const {
    auto it = impl_->clips.find(clip_id);
    if (it == impl_->clips.end()) return 0;
    if (!it->second.has_valid_frame) return 0;
    return it->second.texture_id;
}

int VideoEngine::get_texture_width(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0;
    return it->second.width;
}

int VideoEngine::get_texture_height(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0;
    return it->second.height;
}

double VideoEngine::get_source_duration(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0.0;
    if (!it->second.decoder) return 0.0;
    return it->second.decoder->duration_seconds();
}

double VideoEngine::get_source_fps(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0.0;
    if (!it->second.decoder) return 0.0;
    return it->second.decoder->fps();
}

void VideoEngine::set_playing(bool playing) {
    is_playing_ = playing;
}

void VideoEngine::set_interactive_mode(bool interactive) {
    is_interactive_ = interactive;
}

std::string VideoEngine::consume_error() {
    std::string err = std::move(impl_->decode_error);
    impl_->decode_error.clear();
    return err;
}

std::string VideoEngine::get_active_decoder_info() const {
    for (const auto& [id, state] : impl_->sources) {
        if (state.decoder && state.decoder->is_open()) {
            return state.decoder->decoder_type();
        }
    }
    return "None";
}

} // namespace furious
