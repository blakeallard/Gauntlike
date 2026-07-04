#include "scenes.hpp"
#include "tae/app.hpp"

int main() {
    tae::App app;
    app.run(bo::make_title_scene());
    return 0;
}
