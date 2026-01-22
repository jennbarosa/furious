#include "furious/application.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <nfd.h>

namespace furious {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    if (NFD_Init() != NFD_OKAY) {
        std::fprintf(stderr, "Failed to initialize NFD\n");
        return false;
    }

    if (!init_glfw()) {
        return false;
    }

    if (!init_imgui()) {
        return false;
    }

    main_window_ = std::make_unique<MainWindow>();
    main_window_->set_glfw_window(window_);
    return true;
}

bool Application::init_glfw() {
    // FIXME: Force X11 on Linux to avoid Wayland resize issues.
    // There is very horrible lag on KDE wayland when resizing the window.
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(1280, 720, "FURIOUS", nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    return true;
}

bool Application::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void Application::run() {
    if (!initial_project_.empty()) {
        if (!main_window_->load_project(initial_project_)) {
            std::fprintf(stderr, "Failed to load project: %s\n", initial_project_.c_str());
        }
    }

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        render_frame();
    }
}

void Application::set_initial_project(const std::string& filepath) {
    initial_project_ = filepath;
}

void Application::render_frame() {
    begin_frame();
    main_window_->render();
    glfwSetWindowTitle(window_, main_window_->window_title().c_str());
    end_frame();
}

void Application::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::end_frame() {
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_context);
    }

    glfwSwapBuffers(window_);
}

void Application::shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
    NFD_Quit();
}

} // namespace furious
