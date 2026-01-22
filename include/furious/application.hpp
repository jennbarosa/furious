#pragma once

#include "furious/ui/main_window.hpp"
#include <memory>

struct GLFWwindow;

namespace furious {

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool initialize();
    void run();
    void shutdown();

    void set_initial_project(const std::string& filepath);

    void render_frame();

private:
    GLFWwindow* window_ = nullptr;
    std::unique_ptr<MainWindow> main_window_;
    std::string initial_project_;

    bool init_glfw();
    bool init_imgui();
    void begin_frame();
    void end_frame();
};

} // namespace furious
