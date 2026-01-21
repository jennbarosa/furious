#include "furious/application.hpp"
#include "furious/ui/profiler_window.hpp"

int main() {
    furious::Application app;

    if (!app.initialize()) {
        return EXIT_FAILURE;
    }

    app.run();
    return EXIT_SUCCESS;
}
