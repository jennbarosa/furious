#include "furious/application.hpp"
#include "furious/ui/profiler_window.hpp"
#include <cstdio>

int main(int argc, char* argv[]) {
    furious::Application app;

    if (!app.initialize()) {
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        app.set_initial_project(argv[1]);
    }

    app.run();
    return EXIT_SUCCESS;
}
