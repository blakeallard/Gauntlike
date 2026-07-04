#include "scenes.hpp"
#include "tae/app.hpp"

int main() {
    tae::App app;
    app.run(gl::make_title_scene());
    return 0;
}
