// FURIOUS Integration Tests using ImGui Test Engine

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "imgui_te_ui.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>

void RegisterTimelineTests(ImGuiTestEngine* engine);
void RegisterViewportTests(ImGuiTestEngine* engine);
void RegisterTransportTests(ImGuiTestEngine* engine);
void RegisterSourceTests(ImGuiTestEngine* engine);
void RegisterClipTests(ImGuiTestEngine* engine);
void RegisterSyncTests(ImGuiTestEngine* engine);
void RegisterUserFlowTests(ImGuiTestEngine* engine);

namespace {

GLFWwindow* g_window = nullptr;
bool g_interactive = false;

bool init_glfw() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, g_interactive ? GLFW_TRUE : GLFW_FALSE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    g_window = glfwCreateWindow(1280, 720, "FURIOUS Integration Tests", nullptr, nullptr);
    if (!g_window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(g_interactive ? 1 : 0);  

    return true;
}

bool init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();
}

} // namespace

void print_usage(const char* program) {
    std::printf("Usage: %s [options]\n", program);
    std::printf("Options:\n");
    std::printf("  -i, --interactive   Run with visible window (interactive mode)\n");
    std::printf("  -h, --help          Show this help message\n");
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-i") == 0 || std::strcmp(argv[i], "--interactive") == 0) {
            g_interactive = true;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (g_interactive) {
        std::printf("Running in interactive mode (window visible)\n");
    }

    if (!init_glfw()) {
        return 1;
    }

    if (!init_imgui()) {
        shutdown();
        return 1;
    }

    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunSpeed = g_interactive ? ImGuiTestRunSpeed_Cinematic : ImGuiTestRunSpeed_Fast;

    RegisterTimelineTests(engine);
    RegisterViewportTests(engine);
    RegisterTransportTests(engine);
    RegisterSourceTests(engine);
    RegisterClipTests(engine);
    RegisterSyncTests(engine);
    RegisterUserFlowTests(engine);

    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    if (!g_interactive) {
        ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests);
    }

    bool running = true;
    while (running) {
        glfwPollEvents();

        if (glfwWindowShouldClose(g_window)) {
            running = false;
        }

        if (!g_interactive && ImGuiTestEngine_IsTestQueueEmpty(engine)) {
            running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiTestEngine_ShowTestEngineWindows(engine, nullptr);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_window);

        ImGuiTestEngine_PostSwap(engine);
    }

    int count_tested = 0;
    int count_success = 0;
    ImGuiTestEngine_GetResult(engine, count_tested, count_success);

    std::printf("\n=== Integration Test Results ===\n");
    std::printf("Tests run: %d\n", count_tested);
    std::printf("Tests passed: %d\n", count_success);
    std::printf("Tests failed: %d\n", count_tested - count_success);

    ImGuiTestEngine_Stop(engine);
    shutdown();
    ImGuiTestEngine_DestroyContext(engine);

    return (count_tested == count_success) ? 0 : 1;
}
