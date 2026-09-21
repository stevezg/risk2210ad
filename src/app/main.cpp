#include <cstring>
#include <string>

#include "App.h"

int main(int argc, char** argv) {
    app::App app;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "--shot") == 0) app.screenshotAfter(150, argv[++i]);
    app.run();
    return 0;
}
