#include "furious/ui/main_window.hpp"
#include "furious/core/project_data.hpp"
#include "furious/core/clip_commands.hpp"
#include "furious/core/pattern_commands.hpp"
#include "furious/audio/pitch_estimator.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <algorithm>
#include <cmath>

namespace furious {

MainWindow::MainWindow()
    : project_("FURIOUS Project")
    , timeline_(project_)
    , transport_controls_(project_)
{
    audio_engine_.initialize();
    video_engine_.initialize();
    script_engine_.initialize();

    script_engine_.add_effect_directory("scripts/effects");
    script_engine_.scan_effect_directories();
    script_engine_.set_project(&project_);
    script_engine_.set_timeline_data(&timeline_data_);

    timeline_.set_timeline_data(&timeline_data_);
    timeline_.set_source_library(&source_library_);
    timeline_.set_pattern_library(&pattern_library_);

    viewport_.set_video_engine(&video_engine_);
    viewport_.set_timeline_data(&timeline_data_);
    viewport_.set_source_library(&source_library_);

    patterns_window_.set_pattern_library(&pattern_library_);
    patterns_window_.set_tempo(&project_.tempo());
    patterns_window_.set_command_callback([this](std::unique_ptr<Command> cmd) {
        execute_command(std::move(cmd));
    });

    pitch_editor_.set_pitch_library(&pitch_library_);
    pitch_editor_.set_pattern_library(&pattern_library_);
    pitch_editor_.set_tempo(&project_.tempo());
    pitch_editor_.set_command_callback([this](std::unique_ptr<Command> cmd) {
        execute_command(std::move(cmd));
    });
    pitch_editor_.set_preview_callback([this](int subdivision, int midi_note, float duration, float bgm_vol, float clip_vol) {
        preview_pitch_at_subdivision(subdivision, midi_note, duration, bgm_vol, clip_vol);
    });

    pattern_evaluator_.set_pattern_library(&pattern_library_);
}

MainWindow::~MainWindow() {
    script_engine_.shutdown();
    video_engine_.shutdown();
    audio_engine_.shutdown();
}

void MainWindow::render() {
    auto t0 = std::chrono::high_resolution_clock::now();

    setup_dockspace();
    handle_keyboard_shortcuts();

    auto t1 = std::chrono::high_resolution_clock::now();

    if (cache_building_) {
        render_loading_modal();
        if (!cache_next_clip()) {
            cache_building_ = false;
        }
        return;
    }

    profiler_.update();
    profiler_.set_video_decoder_info(video_engine_.get_active_decoder_info());
    if (ImGui::IsKeyPressed(ImGuiKey_F3)) {
        profiler_.toggle_visible();
    }
    profiler_.render();

    bool is_playing = transport_controls_.is_playing();
    bool has_audio = audio_engine_.has_clip();
    bool metronome_enabled = transport_controls_.metronome_enabled();

    audio_engine_.set_metronome_enabled(metronome_enabled);
    audio_engine_.set_bpm(project_.tempo().bpm());
    timeline_.set_follow_playhead(transport_controls_.follow_playhead());
    timeline_.set_fps(project_.fps());

    video_engine_.set_playing(is_playing);

    if (has_audio) {
        double trimmed_seconds = audio_engine_.trimmed_duration_seconds();
        double trimmed_beats = project_.tempo().time_to_beats(trimmed_seconds);
        timeline_.set_clip_duration_beats(trimmed_beats);
    } else {
        timeline_.set_clip_duration_beats(0.0);
    }

    bool is_seeking = timeline_.is_seeking();
    bool is_previewing = (preview_end_time_ > 0.0 && audio_engine_.playhead_seconds() < preview_end_time_);

    if (is_seeking) {
        double current_seconds = project_.tempo().beats_to_time(timeline_.playhead_position());
        audio_engine_.set_playhead_seconds(current_seconds);
        if (!audio_engine_.is_playing()) {
            audio_engine_.play();
        }
    } else if (is_playing) {
        if (!audio_engine_.is_playing()) {
            audio_engine_.play();
        }
        double audio_seconds = audio_engine_.playhead_seconds();
        double audio_beats = project_.tempo().time_to_beats(audio_seconds);
        timeline_.set_playhead_position(audio_beats);
    } else if (is_previewing) {
        double audio_seconds = audio_engine_.playhead_seconds();
        double audio_beats = project_.tempo().time_to_beats(audio_seconds);
        timeline_.set_playhead_position(audio_beats);
    } else {
        if (preview_end_time_ > 0.0) {
            preview_end_time_ = 0.0;
        }
        if (audio_engine_.is_playing()) {
            audio_engine_.pause();
        }
        double current_beats = timeline_.playhead_position();
        if (current_beats != last_playhead_beats_) {
            double current_seconds = project_.tempo().beats_to_time(current_beats);
            audio_engine_.set_playhead_seconds(current_seconds);
        }
    }

    last_playhead_beats_ = timeline_.playhead_position();

    auto t2 = std::chrono::high_resolution_clock::now();

    video_engine_.set_interactive_mode(timeline_.is_dragging_clip());
    video_engine_.begin_frame();

    double current_beats = timeline_.playhead_position();
    bool playhead_moved = std::abs(current_beats - last_synced_beats_) > 0.0001;
    bool needs_video_sync = is_playing || is_seeking || is_previewing || playhead_moved || !video_synced_once_;

    if (needs_video_sync) {
        sync_video_to_playhead();
        sync_audio_to_playhead();
        last_synced_beats_ = current_beats;
        video_synced_once_ = true;
    }
    video_engine_.update();

    auto t3 = std::chrono::high_resolution_clock::now();

    viewport_.render();
    timeline_.render();
    transport_controls_.render();
    render_audio_panel();
    render_sources_panel();
    render_effects_panel();
    patterns_window_.render();
    pitch_editor_.set_audio_clip(audio_engine_.clip());
    pitch_editor_.set_playhead_beat(compute_pitch_editor_playhead());
    pitch_editor_.render();

    auto t4 = std::chrono::high_resolution_clock::now();

    auto dockspace_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    auto logic_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    auto video_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    auto ui_ms = std::chrono::duration<double, std::milli>(t4 - t3).count();
    auto total_ms = std::chrono::duration<double, std::milli>(t4 - t0).count();

    if (total_ms > 50.0) {
        std::printf("[RENDER] total=%.1fms dockspace=%.1f logic=%.1f video=%.1f ui=%.1f\n",
                    total_ms, dockspace_ms, logic_ms, video_ms, ui_ms);
    }

    if (timeline_.consume_play_toggle_request() || viewport_.consume_play_toggle_request() || pitch_editor_.consume_play_toggle_request()) {
        transport_controls_.set_playing(!transport_controls_.is_playing());
    }

    std::string clip_to_delete;
    if (timeline_.consume_delete_request(clip_to_delete)) {
        execute_command(std::make_unique<RemoveClipCommand>(timeline_data_, clip_to_delete));
        timeline_.clear_selection();
    }

    if (timeline_.consume_data_modified()) {
        dirty_ = true;
    }

    TimelineClip old_clip_state, new_clip_state;
    if (timeline_.consume_clip_modification(old_clip_state, new_clip_state)) {
        execute_command(std::make_unique<ModifyClipCommand>(
            timeline_data_, old_clip_state.id, old_clip_state, new_clip_state, "Move clip"));
    }

    if (viewport_.consume_clip_modification(old_clip_state, new_clip_state)) {
        execute_command(std::make_unique<ModifyClipCommand>(
            timeline_data_, old_clip_state.id, old_clip_state, new_clip_state, "Move clip in viewport"));
    }

    is_playing = transport_controls_.is_playing();

    if (transport_controls_.reset_requested()) {
        timeline_.set_playhead_position(0.0);
        audio_engine_.set_playhead_seconds(0.0);
    }

    if (transport_controls_.save_requested()) {
        std::string filepath = transport_controls_.requested_filepath();
        if (save_project(filepath)) {
            transport_controls_.set_current_project_path(filepath);
        } else {
            std::fprintf(stderr, "Failed to save project: %s\n", filepath.c_str());
        }
    }
    if (transport_controls_.load_requested()) {
        std::string filepath = transport_controls_.requested_filepath();
        if (load_project(filepath)) {
            transport_controls_.set_current_project_path(filepath);
        } else {
            std::fprintf(stderr, "Failed to load project: %s\n", filepath.c_str());
        }
    }

    if (is_playing && transport_controls_.follow_playhead()) {
        timeline_.ensure_playhead_visible();
    }
}

void MainWindow::sync_video_to_playhead() {
    double current_beats = timeline_.playhead_position();

    auto active_clips = timeline_data_.clips_at_beat(current_beats);

    viewport_.clear_transform_overrides();

    // Check if any track is soloed
    bool any_soloed = false;
    for (size_t i = 0; i < timeline_data_.track_count(); ++i) {
        if (timeline_data_.track(i).soloed) {
            any_soloed = true;
            break;
        }
    }

    std::vector<const TimelineClip*> const_clips;
    for (TimelineClip* clip : active_clips) {
        // Apply mute/solo logic
        const Track& track = timeline_data_.track(clip->track_index);
        if (any_soloed) {
            if (!track.soloed) continue;  // Skip non-soloed tracks when solo is active
        } else {
            if (track.muted) continue;    // Skip muted tracks
        }
        double clip_local_beats = current_beats - clip->start_beat;

        PatternEvaluationResult pattern_result = pattern_evaluator_.evaluate(*clip, clip_local_beats);

        const std::vector<ClipEffect>& effects = clip->effects;

        if (!effects.empty()) {
            EffectContext context;
            context.clip = clip;
            context.tempo = &project_.tempo();
            context.current_beats = current_beats;
            context.clip_local_beats = clip_local_beats;

            EffectResult result = script_engine_.evaluate_effects(effects, context);

            ClipTransformOverride override;
            override.position_x = result.position_x.has_value() ? result.position_x : pattern_result.position_x;
            override.position_y = result.position_y.has_value() ? result.position_y : pattern_result.position_y;
            override.scale_x = result.scale_x.has_value() ? result.scale_x : pattern_result.scale_x;
            override.scale_y = result.scale_y.has_value() ? result.scale_y : pattern_result.scale_y;
            override.rotation = result.rotation.has_value() ? result.rotation : pattern_result.rotation;
            override.flip_h = pattern_result.flip_h;
            override.flip_v = pattern_result.flip_v;

            if (override.scale_x.has_value() || override.scale_y.has_value() ||
                override.rotation.has_value() || override.position_x.has_value() ||
                override.position_y.has_value() || override.flip_h.has_value() ||
                override.flip_v.has_value()) {
                viewport_.set_clip_transform_override(clip->id, override);
            }

            if (result.use_looped_frame) {
                video_engine_.request_looped_frame(clip->id, clip->source_id,
                                                   result.loop_start_seconds,
                                                   result.loop_duration_seconds,
                                                   result.position_in_loop_seconds);
            } else if (pattern_result.use_looped_playback) {
                double restart_offset_seconds = project_.tempo().beats_to_time(pattern_result.restart_offset_beats);
                double loop_start = clip->source_start_seconds + restart_offset_seconds;
                double loop_duration = project_.tempo().beats_to_time(pattern_result.loop_duration_beats);
                double position_in_loop = project_.tempo().beats_to_time(pattern_result.position_in_loop_beats);
                video_engine_.request_looped_frame(clip->id, clip->source_id,
                                                   loop_start, loop_duration, position_in_loop);
            } else {
                double clip_local_seconds = project_.tempo().beats_to_time(clip_local_beats);
                clip_local_seconds += clip->source_start_seconds;

                double source_duration = video_engine_.get_source_duration(clip->source_id);
                if (source_duration > 0.0 && clip_local_seconds >= source_duration) {
                    clip_local_seconds = source_duration - 0.001;
                }

                video_engine_.request_frame(clip->id, clip->source_id, clip_local_seconds);
            }
        } else {
            if (pattern_result.scale_x.has_value() || pattern_result.scale_y.has_value() ||
                pattern_result.rotation.has_value() || pattern_result.position_x.has_value() ||
                pattern_result.position_y.has_value() || pattern_result.flip_h.has_value() ||
                pattern_result.flip_v.has_value()) {
                ClipTransformOverride override;
                override.position_x = pattern_result.position_x;
                override.position_y = pattern_result.position_y;
                override.scale_x = pattern_result.scale_x;
                override.scale_y = pattern_result.scale_y;
                override.rotation = pattern_result.rotation;
                override.flip_h = pattern_result.flip_h;
                override.flip_v = pattern_result.flip_v;
                viewport_.set_clip_transform_override(clip->id, override);
            }

            if (pattern_result.use_looped_playback) {
                double restart_offset_seconds = project_.tempo().beats_to_time(pattern_result.restart_offset_beats);
                double loop_start = clip->source_start_seconds + restart_offset_seconds;
                double loop_duration = project_.tempo().beats_to_time(pattern_result.loop_duration_beats);
                double position_in_loop = project_.tempo().beats_to_time(pattern_result.position_in_loop_beats);
                video_engine_.request_looped_frame(clip->id, clip->source_id,
                                                   loop_start, loop_duration, position_in_loop);
            } else {
                double clip_local_seconds = project_.tempo().beats_to_time(clip_local_beats);
                clip_local_seconds += clip->source_start_seconds;

                double source_duration = video_engine_.get_source_duration(clip->source_id);
                if (source_duration > 0.0 && clip_local_seconds >= source_duration) {
                    clip_local_seconds = source_duration - 0.001;
                }

                video_engine_.request_frame(clip->id, clip->source_id, clip_local_seconds);
            }
        }

        const_clips.push_back(clip);
    }

    viewport_.set_active_clips(const_clips);
    viewport_.set_selected_clip_id(timeline_.selected_clip_id());
}

void MainWindow::sync_audio_to_playhead() {
    double current_beats = timeline_.playhead_position();
    uint32_t sample_rate = audio_engine_.sample_rate();

    auto active_clips = timeline_data_.clips_at_beat(current_beats);

    // Check if any track is soloed
    bool any_soloed = false;
    for (size_t i = 0; i < timeline_data_.track_count(); ++i) {
        if (timeline_data_.track(i).soloed) {
            any_soloed = true;
            break;
        }
    }

    std::vector<ClipAudioState> audio_clips;

    for (TimelineClip* clip : active_clips) {
        // Apply mute/solo logic
        const Track& track = timeline_data_.track(clip->track_index);
        if (any_soloed) {
            if (!track.soloed) continue;  // Skip non-soloed tracks when solo is active
        } else {
            if (track.muted) continue;    // Skip muted tracks
        }

        const MediaSource* source = source_library_.find_source(clip->source_id);
        if (!source || !source->has_audio()) {
            continue;
        }

        double clip_start_seconds = project_.tempo().beats_to_time(clip->start_beat);
        double clip_duration_seconds = project_.tempo().beats_to_time(clip->duration_beats);
        double clip_local_beats = current_beats - clip->start_beat;

        ClipAudioState audio_state;
        audio_state.buffer = source->audio_buffer;
        audio_state.timeline_start_frame = static_cast<int64_t>(clip_start_seconds * sample_rate);
        audio_state.duration_frames = static_cast<int64_t>(clip_duration_seconds * sample_rate);
        audio_state.volume = clip->volume;

        PatternEvaluationResult pattern_result = pattern_evaluator_.evaluate(*clip, clip_local_beats);

        if (pattern_result.use_looped_playback) {
            if (pattern_result.freeze_at_end) {
                audio_state.volume = 0.0f;
            } else {
                double restart_offset_seconds = project_.tempo().beats_to_time(pattern_result.restart_offset_beats);
                double loop_start_seconds = clip->source_start_seconds + restart_offset_seconds;
                double loop_duration_seconds = project_.tempo().beats_to_time(pattern_result.loop_duration_beats);

                audio_state.use_looped_audio = true;
                audio_state.loop_start_frames = static_cast<int64_t>(loop_start_seconds * source->audio_buffer->sample_rate());
                audio_state.loop_duration_frames = static_cast<int64_t>(loop_duration_seconds * source->audio_buffer->sample_rate());
                audio_state.loop_phase_offset_frames = 0;
                audio_state.source_offset_frames = 0;
            }
        } else {
            audio_state.source_offset_frames = static_cast<int64_t>(clip->source_start_seconds * source->audio_buffer->sample_rate());
        }

        if (!clip->effects.empty()) {
            EffectContext context;
            context.clip = clip;
            context.tempo = &project_.tempo();
            context.current_beats = current_beats;
            context.clip_local_beats = clip_local_beats;

            EffectResult result = script_engine_.evaluate_effects(clip->effects, context);

            if (result.use_looped_audio && result.audio_loop_duration_seconds > 0.0) {
                audio_state.use_looped_audio = true;
                audio_state.loop_start_frames = static_cast<int64_t>(result.audio_loop_start_seconds * source->audio_buffer->sample_rate());
                audio_state.loop_duration_frames = static_cast<int64_t>(result.audio_loop_duration_seconds * source->audio_buffer->sample_rate());
            }
        }

        if (!clip->pitch_track_id.empty()) {
            const PitchTrack* track = pitch_library_.find_track(clip->pitch_track_id);
            if (track && track->autotune_enabled && source->audio_buffer) {
                constexpr double SUBDIVISIONS_PER_BEAT = 4.0;
                int current_subdiv = static_cast<int>(clip_local_beats * SUBDIVISIONS_PER_BEAT)
                                   % track->length_subdivisions;
                const PitchNote* note = track->note_at(current_subdiv);
                if (note) {
                    audio_state.clip_id = clip->id;
                    audio_state.autotune_enabled = true;
                    audio_state.autotune_target_midi = note->midi_note;
                    audio_state.autotune_amount = 1.0f;

                    int64_t source_frame = audio_state.source_offset_frames;
                    if (audio_state.use_looped_audio && audio_state.loop_duration_frames > 0) {
                        int64_t phase = audio_state.loop_phase_offset_frames % audio_state.loop_duration_frames;
                        if (phase < 0) phase += audio_state.loop_duration_frames;
                        source_frame = audio_state.loop_start_frames + phase;
                    }

                    constexpr size_t ANALYSIS_WINDOW = 2048;
                    constexpr int64_t CACHE_THRESHOLD = 512;
                    constexpr int FRAME_RATE_LIMIT = 4;
                    uint32_t buf_channels = source->audio_buffer->channels();
                    uint32_t buf_sample_rate = source->audio_buffer->sample_rate();

                    auto& cache = pitch_detection_cache_[clip->id];
                    cache.frames_since_detection++;

                    bool position_changed = std::abs(source_frame - cache.last_source_frame) > CACHE_THRESHOLD;
                    bool frame_limit_passed = cache.frames_since_detection >= FRAME_RATE_LIMIT;
                    bool need_detection = position_changed && frame_limit_passed;

                    if (need_detection &&
                        source_frame >= 0 &&
                        source_frame + static_cast<int64_t>(ANALYSIS_WINDOW) < static_cast<int64_t>(source->audio_buffer->frame_count())) {

                        std::vector<float> mono_samples(ANALYSIS_WINDOW);
                        for (size_t j = 0; j < ANALYSIS_WINDOW; ++j) {
                            float sample = source->audio_buffer->sample_at(static_cast<uint64_t>(source_frame) + j, 0);
                            if (buf_channels >= 2) {
                                sample = (sample + source->audio_buffer->sample_at(static_cast<uint64_t>(source_frame) + j, 1)) * 0.5f;
                            }
                            mono_samples[j] = sample;
                        }

                        PitchResult detected = PitchEstimator::estimate(
                            mono_samples.data(), ANALYSIS_WINDOW, buf_sample_rate, PitchAlgorithm::YIN);

                        cache.last_source_frame = source_frame;
                        cache.detected_frequency = detected.frequency_hz;
                        cache.confidence = detected.confidence;
                        cache.frames_since_detection = 0;
                    }

                    if (cache.confidence > 0.5f && cache.detected_frequency > 50.0f) {
                        float target_freq = note->frequency_hz();
                        float raw_shift = 1200.0f * std::log2(target_freq / cache.detected_frequency);
                        constexpr float MAX_SHIFT_CENTS = 3600.0f;
                        float new_shift = std::max(-MAX_SHIFT_CENTS, std::min(MAX_SHIFT_CENTS, raw_shift));

                        auto it = last_pitch_shift_cents_.find(clip->id);
                        if (it != last_pitch_shift_cents_.end()) {
                            float alpha = 1.0f - track->smoothing * 0.99f;
                            audio_state.pitch_shift_cents = it->second + alpha * (new_shift - it->second);
                        } else {
                            audio_state.pitch_shift_cents = new_shift;
                        }
                        last_pitch_shift_cents_[clip->id] = audio_state.pitch_shift_cents;
                    }
                }
            }
        }

        audio_clips.push_back(std::move(audio_state));
    }

    audio_engine_.set_active_clips(std::move(audio_clips));
}

void MainWindow::setup_dockspace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (first_frame_) {
        first_frame_ = false;
        if (!layout_loaded_) {
            build_default_layout(dockspace_id);
        }
    }

    ImGui::End();
}

void MainWindow::build_default_layout(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, nullptr, &dock_main);
    ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.2f, nullptr, &dock_main);

    ImGui::DockBuilderDockWindow("Viewport", dock_main);
    ImGui::DockBuilderDockWindow("Timeline", dock_bottom);
    ImGui::DockBuilderDockWindow("Sources", dock_right);
    ImGui::DockBuilderDockWindow("Audio", dock_right);
    ImGui::DockBuilderDockWindow("Clip", dock_right);
    ImGui::DockBuilderDockWindow("Patterns", dock_right);
    ImGui::DockBuilderDockWindow("Profiler", dock_right);
    ImGui::DockBuilderDockWindow("Project", dock_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

Project& MainWindow::project() {
    return project_;
}

AudioEngine& MainWindow::audio_engine() {
    return audio_engine_;
}

VideoEngine& MainWindow::video_engine() {
    return video_engine_;
}

SourceLibrary& MainWindow::source_library() {
    return source_library_;
}

PatternLibrary& MainWindow::pattern_library() {
    return pattern_library_;
}

PitchLibrary& MainWindow::pitch_library() {
    return pitch_library_;
}

TimelineData& MainWindow::timeline_data() {
    return timeline_data_;
}

Timeline& MainWindow::timeline() {
    return timeline_;
}

TransportControls& MainWindow::transport_controls() {
    return transport_controls_;
}

ScriptEngine& MainWindow::script_engine() {
    return script_engine_;
}

bool MainWindow::load_audio_file(const std::string& filepath) {
    return audio_engine_.load_clip(filepath);
}

std::string MainWindow::import_source(const std::string& filepath) {
    std::string source_id = source_library_.add_source(filepath);

    if (MediaSource* source = source_library_.find_source(source_id)) {
        video_engine_.register_source(*source);

        if (source->type == MediaType::Video) {
            source->duration_seconds = video_engine_.get_source_duration(source_id);
            source->fps = video_engine_.get_source_fps(source_id);
        }
        dirty_ = true;
    }

    return source_id;
}

void MainWindow::render_sources_panel() {
    if (!ImGui::Begin("Sources")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Import Source")) {
        nfdu8char_t* out_path = nullptr;
        nfdu8filteritem_t filters[] = {
            {"Video", "mp4,mkv,avi,mov,webm"},
            {"Image", "png,jpg,jpeg,bmp,gif"}
        };
        nfdopendialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 2;

        if (NFD_OpenDialogU8_With(&out_path, &args) == NFD_OKAY) {
            import_source(out_path);
            NFD_FreePathU8(out_path);
        }
    }

    ImGui::Separator();
    ImGui::Text("Loaded Sources:");

    bool open_remove_popup = false;
    for (const auto& source : source_library_.sources()) {
        ImGui::PushID(source.id.c_str());

        bool is_video = (source.type == MediaType::Video);
        const char* type_str = is_video ? "[V]" : "[I]";

        if (ImGui::SmallButton("+")) {
            TimelineClip clip;
            clip.id = TimelineData::generate_id();
            clip.source_id = source.id;
            clip.start_beat = timeline_.playhead_position();

            if (is_video && source.duration_seconds > 0) {
                clip.duration_beats = project_.tempo().time_to_beats(source.duration_seconds);
            } else {
                clip.duration_beats = 4.0;
            }

            size_t available_track = timeline_data_.find_available_track(clip.start_beat, clip.duration_beats);
            if (available_track >= timeline_data_.track_count()) {
                timeline_data_.add_track();
            }
            clip.track_index = available_track;

            execute_command(std::make_unique<AddClipCommand>(timeline_data_, clip));
            video_engine_.prefetch_clip(clip.id, clip.source_id, clip.source_start_seconds);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("S")) {
            nfdu8char_t* out_path = nullptr;
            nfdu8filteritem_t filters[] = {
                {"Video", "mp4,mkv,avi,mov,webm"},
                {"Image", "png,jpg,jpeg,bmp,gif"}
            };
            nfdopendialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 2;

            if (NFD_OpenDialogU8_With(&out_path, &args) == NFD_OKAY) {
                std::string source_id = source.id;
                video_engine_.unregister_source(source_id);
                if (source_library_.replace_source(source_id, out_path)) {
                    if (MediaSource* new_source = source_library_.find_source(source_id)) {
                        video_engine_.register_source(*new_source);
                        if (new_source->type == MediaType::Video) {
                            new_source->duration_seconds = video_engine_.get_source_duration(source_id);
                            new_source->fps = video_engine_.get_source_fps(source_id);
                        }

                        for (const auto& clip : timeline_data_.clips()) {
                            if (clip.source_id == source_id) {
                                pitch_detection_cache_.erase(clip.id);
                                last_pitch_shift_cents_.erase(clip.id);
                                audio_engine_.reset_pitch_shifter(clip.id);
                            }
                        }
                    }
                    dirty_ = true;
                }
                NFD_FreePathU8(out_path);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Swap source file (updates all clips)");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            if (timeline_data_.has_clips_using_source(source.id)) {
                pending_source_removal_ = source.id;
                open_remove_popup = true;
            } else {
                video_engine_.unregister_source(source.id);
                source_library_.remove_source(source.id);
                dirty_ = true;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s %s", type_str, source.name.c_str());

        ImGui::PopID();
    }

    if (open_remove_popup) {
        ImGui::OpenPopup("Confirm Remove Source");
    }

    if (ImGui::BeginPopupModal("Confirm Remove Source", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you SURE?");
        ImGui::Text("This will delete every single clip that uses this source on the timeline.");
        ImGui::Separator();

        if (ImGui::Button("Yes, Remove")) {
            timeline_data_.remove_clips_by_source(pending_source_removal_);
            video_engine_.unregister_source(pending_source_removal_);
            source_library_.remove_source(pending_source_removal_);
            pending_source_removal_.clear();
            dirty_ = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            pending_source_removal_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (ImGui::Button("Add Track")) {
        timeline_data_.add_track();
    }

    ImGui::SameLine();
    ImGui::Text("Tracks: %zu", timeline_data_.track_count());

    ImGui::End();
}

void MainWindow::render_effects_panel() {
    if (!ImGui::Begin("Clip")) {
        ImGui::End();
        return;
    }

    const std::string& selected_id = timeline_.selected_clip_id();
    TimelineClip* clip = nullptr;
    if (!selected_id.empty()) {
        clip = timeline_data_.find_clip(selected_id);
    }

    if (!clip) {
        ImGui::TextDisabled("No clip selected");
        ImGui::End();
        return;
    }

    const MediaSource* source = source_library_.find_source(clip->source_id);
    if (source) {
        ImGui::Text("Clip: %s", source->name.c_str());
    } else {
        ImGui::Text("Clip: %s", clip->id.c_str());
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        TimelineClip state_before_this_frame = *clip;

        bool any_active = false;
        bool any_deactivated_after_edit = false;

        ImGui::Text("Position");
        ImGui::PushItemWidth(-70);
        ImGui::DragFloat("X##pos", &clip->position_x, 1.0f, -10000.0f, 10000.0f, "%.0f px");
        if (ImGui::IsItemActive()) any_active = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) any_deactivated_after_edit = true;
        ImGui::DragFloat("Y##pos", &clip->position_y, 1.0f, -10000.0f, 10000.0f, "%.0f px");
        if (ImGui::IsItemActive()) any_active = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) any_deactivated_after_edit = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##pos")) {
            TimelineClip old_state = *clip;
            clip->position_x = 0.0f;
            clip->position_y = 0.0f;
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Reset position"));
        }

        ImGui::Spacing();
        ImGui::Text("Scale");
        ImGui::PushItemWidth(-70);
        ImGui::DragFloat("X##scale", &clip->scale_x, 0.01f, 0.01f, 10.0f, "%.2f");
        if (ImGui::IsItemActive()) any_active = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) any_deactivated_after_edit = true;
        ImGui::DragFloat("Y##scale", &clip->scale_y, 0.01f, 0.01f, 10.0f, "%.2f");
        if (ImGui::IsItemActive()) any_active = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) any_deactivated_after_edit = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##scale")) {
            TimelineClip old_state = *clip;
            clip->scale_x = 1.0f;
            clip->scale_y = 1.0f;
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Reset scale"));
        }

        ImGui::Spacing();
        ImGui::Text("Rotation");
        ImGui::PushItemWidth(-70);
        ImGui::DragFloat("##rotation", &clip->rotation, 1.0f, -360.0f, 360.0f, "%.1f deg");
        if (ImGui::IsItemActive()) any_active = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) any_deactivated_after_edit = true;
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##rot")) {
            TimelineClip old_state = *clip;
            clip->rotation = 0.0f;
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Reset rotation"));
        }

        if (any_active && edit_mode_ == EditMode::None) {
            edit_mode_ = EditMode::Transform;
            property_edit_initial_state_ = state_before_this_frame;
        }

        if (any_deactivated_after_edit && edit_mode_ == EditMode::Transform) {
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, property_edit_initial_state_, *clip, "Transform clip"));
            edit_mode_ = EditMode::None;
        }

        if (!any_active && edit_mode_ == EditMode::Transform) {
            edit_mode_ = EditMode::None;
        }

        ImGui::Spacing();
        if (ImGui::Button("Reset All")) {
            TimelineClip old_state = *clip;
            clip->position_x = 0.0f;
            clip->position_y = 0.0f;
            clip->scale_x = 1.0f;
            clip->scale_y = 1.0f;
            clip->rotation = 0.0f;
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Reset all transforms"));
        }
    }

    if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& available_effects = script_engine_.available_effects();

        bool any_effect_drag_active = false;
        bool any_effect_drag_deactivated = false;

        for (const auto& effect_info : available_effects) {
            ImGui::PushID(effect_info.id.c_str());

            auto find_effect = [&clip, &effect_info]() -> ClipEffect* {
                for (auto& effect : clip->effects) {
                    if (effect.effect_id == effect_info.id) {
                        return &effect;
                    }
                }
                return nullptr;
            };

            ClipEffect* active_effect = find_effect();
            bool is_enabled = active_effect != nullptr && active_effect->enabled;

            ImGui::Text("%s", effect_info.name.c_str());

            bool was_enabled = is_enabled;
            if (ImGui::Checkbox("Enabled", &is_enabled)) {
                TimelineClip old_state = *clip;
                if (is_enabled && !active_effect) {
                    ClipEffect new_effect;
                    new_effect.effect_id = effect_info.id;
                    for (const auto& param : effect_info.parameters) {
                        std::string value = param.default_value;
                        if (value == "@clip.position_x") {
                            value = std::to_string(clip->position_x);
                        } else if (value == "@clip.position_y") {
                            value = std::to_string(clip->position_y);
                        } else if (value == "@clip.scale_x") {
                            value = std::to_string(clip->scale_x);
                        } else if (value == "@clip.scale_y") {
                            value = std::to_string(clip->scale_y);
                        } else if (value == "@clip.rotation") {
                            value = std::to_string(clip->rotation);
                        }
                        new_effect.parameters[param.name] = value;
                    }
                    clip->effects.push_back(new_effect);
                    active_effect = &clip->effects.back();
                    execute_command(std::make_unique<ModifyClipCommand>(
                        timeline_data_, clip->id, old_state, *clip, "Enable effect"));
                } else if (!is_enabled && active_effect) {
                    std::string id_to_remove = effect_info.id;
                    clip->effects.erase(
                        std::remove_if(clip->effects.begin(), clip->effects.end(),
                            [&id_to_remove](const ClipEffect& e) { return e.effect_id == id_to_remove; }),
                        clip->effects.end());
                    active_effect = nullptr;

                    if (source && source->duration_seconds > 0) {
                        clip->duration_beats = project_.tempo().time_to_beats(source->duration_seconds);
                    }
                    execute_command(std::make_unique<ModifyClipCommand>(
                        timeline_data_, clip->id, old_state, *clip, "Disable effect"));
                }
            }

            if (active_effect) {
                ImGui::Indent();

                for (const auto& param : effect_info.parameters) {
                    std::string& current_value = active_effect->parameters[param.name];

                    if (param.type == "enum" && !param.enum_values.empty()) {
                        int current_index = 0;
                        for (size_t i = 0; i < param.enum_values.size(); ++i) {
                            if (param.enum_values[i] == current_value) {
                                current_index = static_cast<int>(i);
                                break;
                            }
                        }

                        std::string combo_items;
                        for (const auto& v : param.enum_values) {
                            combo_items += v + '\0';
                        }
                        combo_items += '\0';

                        ImGui::PushItemWidth(100);
                        int old_index = current_index;
                        if (ImGui::Combo(param.name.c_str(), &current_index, combo_items.c_str())) {
                            TimelineClip old_state = *clip;
                            current_value = param.enum_values[current_index];
                            execute_command(std::make_unique<ModifyClipCommand>(
                                timeline_data_, clip->id, old_state, *clip, "Change effect parameter"));
                        }
                        ImGui::PopItemWidth();
                    } else if (param.type == "bool") {
                        bool checked = (current_value == "true");
                        bool old_checked = checked;
                        if (ImGui::Checkbox(param.name.c_str(), &checked)) {
                            TimelineClip old_state = *clip;
                            current_value = checked ? "true" : "false";
                            execute_command(std::make_unique<ModifyClipCommand>(
                                timeline_data_, clip->id, old_state, *clip, "Change effect parameter"));
                        }
                    } else if (param.type == "number") {
                        float value = 0.0f;
                        try {
                            value = std::stof(current_value);
                        } catch (...) {}
                        ImGui::PushItemWidth(100);
                        if (ImGui::DragFloat(param.name.c_str(), &value, 0.1f)) {
                            current_value = std::to_string(value);
                        }
                        if (ImGui::IsItemActive()) any_effect_drag_active = true;
                        if (ImGui::IsItemDeactivatedAfterEdit()) any_effect_drag_deactivated = true;
                        ImGui::PopItemWidth();
                    }
                }

                ImGui::Unindent();
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (any_effect_drag_active && edit_mode_ == EditMode::None) {
            edit_mode_ = EditMode::Effect;
            property_edit_initial_state_ = *clip;
        }

        if (any_effect_drag_deactivated && edit_mode_ == EditMode::Effect) {
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, property_edit_initial_state_, *clip, "Change effect parameter"));
            edit_mode_ = EditMode::None;
        }

        if (!any_effect_drag_active && edit_mode_ == EditMode::Effect) {
            edit_mode_ = EditMode::None;
        }

        if (available_effects.empty()) {
            ImGui::TextDisabled("No effects available");
        }
    }

    if (ImGui::CollapsingHeader("Patterns", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char pattern_search_buf[128] = {};
        ImGui::InputTextWithHint("##pattern_search", "Search patterns...", pattern_search_buf, sizeof(pattern_search_buf));
        std::string search_filter(pattern_search_buf);

        ImGui::Text("Applied:");
        if (clip->patterns.empty()) {
            ImGui::TextDisabled("  (none)");
        } else {
            for (size_t i = 0; i < clip->patterns.size(); ++i) {
                auto& ref = clip->patterns[i];
                const Pattern* pattern = pattern_library_.find_pattern(ref.pattern_id);
                if (!pattern) continue;

                ImGui::PushID(static_cast<int>(i));

                bool enabled = ref.enabled;
                if (ImGui::Checkbox("##enabled", &enabled)) {
                    TimelineClip old_state = *clip;
                    ref.enabled = enabled;
                    execute_command(std::make_unique<ModifyClipCommand>(
                        timeline_data_, clip->id, old_state, *clip, "Toggle pattern"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", pattern->name.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    execute_command(std::make_unique<ToggleClipPatternCommand>(
                        timeline_data_, clip->id, ref.pattern_id, false));
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();
        ImGui::Text("Add Pattern:");

        for (const auto& pattern : pattern_library_.patterns()) {
            bool already_applied = std::ranges::any_of(clip->patterns,
                [&](const ClipPatternReference& r) { return r.pattern_id == pattern.id; });

            if (already_applied) continue;

            if (!search_filter.empty()) {
                std::string name_lower = pattern.name;
                std::string filter_lower = search_filter;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
                if (name_lower.find(filter_lower) == std::string::npos) continue;
            }

            ImGui::PushID(pattern.id.c_str());
            if (ImGui::SmallButton("+")) {
                execute_command(std::make_unique<ToggleClipPatternCommand>(
                    timeline_data_, clip->id, pattern.id, true));
            }
            ImGui::SameLine();
            ImGui::Text("%s", pattern.name.c_str());
            ImGui::PopID();
        }

        if (pattern_library_.patterns().empty()) {
            ImGui::TextDisabled("No patterns available. Create one in the Patterns window.");
        }
    }

    if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
        int vol_pct = static_cast<int>(clip->volume * 100.0f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderInt("Volume", &vol_pct, 0, 200, "%d%%")) {
            TimelineClip old_state = *clip;
            clip->volume = static_cast<float>(vol_pct) / 100.0f;
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Set volume"));
        }
    }

    if (ImGui::CollapsingHeader("Pitch Track")) {
        std::vector<std::string> track_names;
        std::vector<std::string> track_ids;
        track_names.push_back("None");
        track_ids.push_back("");

        for (const auto& track : pitch_library_.tracks()) {
            track_names.push_back(track.name);
            track_ids.push_back(track.id);
        }

        int current = 0;
        for (size_t i = 0; i < track_ids.size(); ++i) {
            if (track_ids[i] == clip->pitch_track_id) {
                current = static_cast<int>(i);
                break;
            }
        }

        std::string combo_items;
        for (const auto& name : track_names) {
            combo_items += name + '\0';
        }
        combo_items += '\0';

        if (ImGui::Combo("Track", &current, combo_items.c_str())) {
            TimelineClip old_state = *clip;
            clip->pitch_track_id = track_ids[current];
            execute_command(std::make_unique<ModifyClipCommand>(
                timeline_data_, clip->id, old_state, *clip, "Set pitch track"));
        }

        if (pitch_library_.tracks().empty()) {
            ImGui::TextDisabled("No pitch tracks available.");
            ImGui::TextDisabled("Create one in the Pitch Editor.");
            if (ImGui::SmallButton("Open Pitch Editor")) {
                pitch_editor_.set_open(true);
                ImGui::SetWindowFocus("Pitch Editor");
            }
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Delete Clip")) {
        execute_command(std::make_unique<RemoveClipCommand>(timeline_data_, selected_id));
        timeline_.clear_selection();
    }

    ImGui::End();
}

void MainWindow::render_audio_panel() {
    if (!ImGui::Begin("Audio")) {
        ImGui::End();
        return;
    }

    if (audio_engine_.has_clip()) {
        const AudioClip* clip = audio_engine_.clip();
        ImGui::Text("Loaded: %s", clip->filepath().c_str());
        ImGui::Text("Duration: %.2f seconds", clip->duration_seconds());
        ImGui::Text("Sample Rate: %u Hz", clip->sample_rate());
        ImGui::Text("Channels: %u", clip->channels());

        double duration_beats = project_.tempo().time_to_beats(clip->duration_seconds());
        ImGui::Text("Duration: %.1f beats", duration_beats);

        ImGui::Separator();
        ImGui::Text("Clip Bounds (Trim)");
        double trimmed = audio_engine_.trimmed_duration_seconds();
        double trimmed_beats = project_.tempo().time_to_beats(trimmed);
        ImGui::Text("Trimmed: %.2f s (%.1f beats)", trimmed, trimmed_beats);

        float clip_duration = static_cast<float>(clip->duration_seconds());
        float start_sec = static_cast<float>(audio_engine_.clip_start_seconds());
        float end_sec = static_cast<float>(audio_engine_.clip_end_seconds());
        if (end_sec <= 0.0f) end_sec = clip_duration;

        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::InputFloat("Start (s)", &start_sec, 0.0f, 0.0f, "%.3f")) {
            start_sec = std::clamp(start_sec, 0.0f, clip_duration);
            if (start_sec < end_sec) {
                audio_engine_.set_clip_start_seconds(static_cast<double>(start_sec));
                dirty_ = true;
            }
        }

        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::InputFloat("End (s)", &end_sec, 0.0f, 0.0f, "%.3f")) {
            end_sec = std::clamp(end_sec, 0.0f, clip_duration);
            if (end_sec > start_sec) {
                audio_engine_.set_clip_end_seconds(static_cast<double>(end_sec));
                dirty_ = true;
            }
        }

        if (ImGui::Button("Reset Bounds")) {
            audio_engine_.reset_clip_bounds();
            dirty_ = true;
        }

        ImGui::Separator();

        if (ImGui::Button("Open Pitch Editor")) {
            pitch_editor_.set_open(true);
            ImGui::SetWindowFocus("Pitch Editor");
        }

        ImGui::Separator();

        if (ImGui::Button("Unload")) {
            audio_engine_.unload_clip();
            dirty_ = true;
        }
    } else {
        ImGui::Text("No audio loaded");

        if (ImGui::Button("Load Audio File")) {
            nfdu8char_t* out_path = nullptr;
            nfdu8filteritem_t filters[] = {
                {"Audio", "wav,mp3,flac,ogg,m4a,aac"}
            };
            nfdopendialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;

            if (NFD_OpenDialogU8_With(&out_path, &args) == NFD_OKAY) {
                if (load_audio_file(out_path)) {
                    dirty_ = true;
                } else {
                    std::fprintf(stderr, "Failed to load audio file: %s\n", out_path);
                }
                NFD_FreePathU8(out_path);
            }
        }
    }

    ImGui::End();
}

bool MainWindow::save_project(const std::string& filepath) {
    ProjectData data;

    data.name = project_.name();
    data.bpm = project_.tempo().bpm();
    data.grid_subdivision = project_.grid_subdivision();
    data.fps = project_.fps();
    data.metronome_enabled = transport_controls_.metronome_enabled();
    data.follow_playhead = transport_controls_.follow_playhead();
    data.loop_enabled = transport_controls_.loop_enabled();
    data.playhead_beat = timeline_.playhead_position();
    data.timeline_zoom = timeline_.zoom();
    data.timeline_zoom_y = timeline_.zoom_y();
    data.timeline_scroll = timeline_.scroll_offset();
    data.timeline_scroll_y = timeline_.scroll_offset_y();

    if (audio_engine_.has_clip()) {
        data.audio_filepath = audio_engine_.clip()->filepath();
        data.clip_start_seconds = audio_engine_.clip_start_seconds();
        data.clip_end_seconds = audio_engine_.clip_end_seconds();
    }

    data.sources = source_library_.sources();
    data.tracks = timeline_data_.tracks();
    data.clips = timeline_data_.clips();
    data.patterns = pattern_library_.patterns();
    data.pitch_tracks = pitch_library_.tracks();
    data.pitch_preview_enabled = pitch_editor_.preview_enabled();
    data.pitch_preview_duration = pitch_editor_.preview_duration();
    data.pitch_editor_open = pitch_editor_.is_open();
    data.pitch_bgm_volume = pitch_editor_.bgm_volume();
    data.pitch_clip_volume = pitch_editor_.clip_volume();
    data.pitch_overlay_pattern_id = pitch_editor_.overlay_pattern_id();

    if (glfw_window_) {
        glfwGetWindowSize(glfw_window_, &data.window_width, &data.window_height);
    }
    const char* ini_data = ImGui::SaveIniSettingsToMemory();
    if (ini_data) {
        data.imgui_layout = ini_data;
    }

    if (data.save_to_file(filepath)) {
        current_project_path_ = filepath;
        transport_controls_.set_current_project_path(filepath);
        dirty_ = false;
        return true;
    }
    return false;
}

bool MainWindow::load_project(const std::string& filepath) {
    ProjectData data;
    if (!ProjectData::load_from_file(filepath, data)) {
        return false;
    }

    project_.set_name(data.name);
    project_.tempo().set_bpm(data.bpm);
    project_.set_grid_subdivision(data.grid_subdivision);
    project_.set_fps(data.fps);
    transport_controls_.set_metronome_enabled(data.metronome_enabled);
    transport_controls_.set_follow_playhead(data.follow_playhead);
    transport_controls_.set_loop_enabled(data.loop_enabled);

    if (!data.audio_filepath.empty()) {
        if (audio_engine_.load_clip(data.audio_filepath)) {
            audio_engine_.set_clip_start_seconds(data.clip_start_seconds);
            audio_engine_.set_clip_end_seconds(data.clip_end_seconds);
        }
    } else {
        audio_engine_.unload_clip();
    }

    source_library_.clear();
    video_engine_.clear_sources();
    for (const auto& source : data.sources) {
        source_library_.add_source_direct(source);
        video_engine_.register_source(source);
    }

    timeline_data_.clear_all();
    if (!data.tracks.empty()) {
        timeline_data_.set_tracks(data.tracks);
    } else {
        timeline_data_.add_track("Track 1");
    }
    timeline_data_.set_clips(data.clips);

    pattern_library_.clear();
    for (const auto& pattern : data.patterns) {
        pattern_library_.add_pattern(pattern);
    }

    pitch_library_.clear();
    for (const auto& track : data.pitch_tracks) {
        pitch_library_.add_track(track);
    }
    pitch_editor_.set_preview_enabled(data.pitch_preview_enabled);
    pitch_editor_.set_preview_duration(data.pitch_preview_duration);
    pitch_editor_.set_open(data.pitch_editor_open);
    pitch_editor_.set_bgm_volume(data.pitch_bgm_volume);
    pitch_editor_.set_clip_volume(data.pitch_clip_volume);
    pitch_editor_.set_overlay_pattern_id(data.pitch_overlay_pattern_id);

    timeline_.set_playhead_position(data.playhead_beat);
    timeline_.set_zoom(data.timeline_zoom);
    timeline_.set_zoom_y(data.timeline_zoom_y);
    timeline_.set_scroll_offset(data.timeline_scroll);
    timeline_.set_scroll_offset_y(data.timeline_scroll_y);
    timeline_.clear_selection();
    audio_engine_.set_playhead_seconds(data.playhead_beat * project_.tempo().beat_duration_seconds());

    if (glfw_window_ && data.window_width > 0 && data.window_height > 0) {
        glfwSetWindowSize(glfw_window_, data.window_width, data.window_height);
    }
    if (!data.imgui_layout.empty()) {
        if (!first_frame_) {
            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockBuilderRemoveNode(dockspace_id);
        }
        ImGui::LoadIniSettingsFromMemory(data.imgui_layout.c_str(), data.imgui_layout.size());
        layout_loaded_ = true;
    }

    start_cache_building();

    pitch_detection_cache_.clear();
    last_pitch_shift_cents_.clear();
    audio_engine_.clear_pitch_shifters();

    current_project_path_ = filepath;
    transport_controls_.set_current_project_path(filepath);
    command_history_.clear();
    dirty_ = false;
    return true;
}

bool MainWindow::needs_continuous_rendering() const {
    return transport_controls_.is_playing() || audio_engine_.is_playing();
}

bool MainWindow::is_playing() const {
    return transport_controls_.is_playing() || audio_engine_.is_playing();
}

std::string MainWindow::window_title() const {
    std::string title = "FURIOUS";
    if (!current_project_path_.empty()) {
        size_t last_slash = current_project_path_.find_last_of("/\\");
        std::string filename = (last_slash != std::string::npos)
            ? current_project_path_.substr(last_slash + 1)
            : current_project_path_;
        title += " - " + filename;
    } else {
        title += " - Untitled";
    }
    if (dirty_) {
        title += "*";
    }
    return title;
}

void MainWindow::mark_dirty() {
    dirty_ = true;
}

void MainWindow::cache_all_clips() {
    for (const auto& clip : timeline_data_.clips()) {
        if (!clip.effects.empty()) {
            EffectContext context;
            context.clip = &clip;
            context.tempo = &project_.tempo();
            context.current_beats = clip.start_beat;
            context.clip_local_beats = 0.0;

            EffectResult result = script_engine_.evaluate_effects(clip.effects, context);

            if (result.use_looped_frame && !video_engine_.is_loop_cache_complete(clip.id)) {
                video_engine_.prebuild_loop_cache(clip.id, clip.source_id,
                                                   result.loop_start_seconds,
                                                   result.loop_duration_seconds);
            }
        }

        if (!video_engine_.is_clip_cached(clip.id)) {
            video_engine_.prefetch_clip(clip.id, clip.source_id, clip.source_start_seconds);
        }
    }
}

void MainWindow::start_cache_building() {
    cache_total_clips_ = timeline_data_.clips().size();
    cache_current_clip_ = 0;
    cache_building_ = cache_total_clips_ > 0;
}

bool MainWindow::cache_next_clip() {
    if (cache_current_clip_ >= cache_total_clips_) {
        return false;
    }

    const auto& clips = timeline_data_.clips();
    const auto& clip = clips[cache_current_clip_];

    if (!clip.effects.empty()) {
        EffectContext context;
        context.clip = &clip;
        context.tempo = &project_.tempo();
        context.current_beats = clip.start_beat;
        context.clip_local_beats = 0.0;

        EffectResult result = script_engine_.evaluate_effects(clip.effects, context);

        if (result.use_looped_frame && !video_engine_.is_loop_cache_complete(clip.id)) {
            video_engine_.prebuild_loop_cache(clip.id, clip.source_id,
                                               result.loop_start_seconds,
                                               result.loop_duration_seconds);
        }
    }

    if (!video_engine_.is_clip_cached(clip.id)) {
        video_engine_.prefetch_clip(clip.id, clip.source_id, clip.source_start_seconds);
    }

    ++cache_current_clip_;
    return cache_current_clip_ < cache_total_clips_;
}

void MainWindow::render_loading_modal() {
    ImGui::OpenPopup("Loading");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 0));

    if (ImGui::BeginPopupModal("Loading", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::Text("Building clip caches...");
        ImGui::Spacing();

        float progress = cache_total_clips_ > 0
            ? static_cast<float>(cache_current_clip_) / static_cast<float>(cache_total_clips_)
            : 0.0f;

        ImGui::ProgressBar(progress, ImVec2(-1, 0));

        ImGui::Text("%zu / %zu clips", cache_current_clip_, cache_total_clips_);

        ImGui::EndPopup();
    }
}

void MainWindow::execute_command(std::unique_ptr<Command> cmd) {
    viewport_.set_active_clips({});
    command_history_.execute(std::move(cmd));
    dirty_ = true;
}

void MainWindow::handle_keyboard_shortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) {
        if (command_history_.can_undo()) {
            viewport_.set_active_clips({});
            command_history_.undo();
            dirty_ = true;
        }
    }

    if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
        (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
        if (command_history_.can_redo()) {
            viewport_.set_active_clips({});
            command_history_.redo();
            dirty_ = true;
        }
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (!current_project_path_.empty()) {
            save_project(current_project_path_);
        } else {
            nfdu8char_t* out_path = nullptr;
            nfdu8filteritem_t filters[] = {{"FURIOUS Project", "furious"}};
            nfdsavedialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;
            std::string default_name = project_.name() + ".furious";
            args.defaultName = default_name.c_str();

            if (NFD_SaveDialogU8_With(&out_path, &args) == NFD_OKAY) {
                if (save_project(out_path)) {
                    transport_controls_.set_current_project_path(out_path);
                }
                NFD_FreePathU8(out_path);
            }
        }
    }
}

double MainWindow::compute_pitch_editor_playhead() const {
    double playhead = timeline_.playhead_position();
    const std::string& sel_id = timeline_.selected_clip_id();
    if (!sel_id.empty()) {
        const TimelineClip* clip = timeline_data_.find_clip(sel_id);
        if (clip && clip->pitch_track_id == pitch_editor_.selected_track_id()) {
            return playhead - clip->start_beat;
        }
    }
    return playhead;
}

void MainWindow::preview_pitch_at_subdivision(int subdivision, int midi_note, float duration, float bgm_vol, float clip_vol) {
    constexpr int SUBDIVISIONS_PER_BEAT = 4;
    double beat = static_cast<double>(subdivision) / static_cast<double>(SUBDIVISIONS_PER_BEAT);
    double seconds = project_.tempo().beats_to_time(beat);

    timeline_.set_playhead_position(beat);
    audio_engine_.set_playhead_seconds(seconds);
    audio_engine_.set_bgm_volume(bgm_vol);
    audio_engine_.set_clip_volume(clip_vol);
    sync_audio_to_playhead();

    preview_end_time_ = seconds + static_cast<double>(duration);

    if (!audio_engine_.is_playing()) {
        audio_engine_.play();
    }
}

} // namespace furious
