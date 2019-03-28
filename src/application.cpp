#include "application.h"

namespace xi {

Application::Application(int &argc, char **argv) : QApplication(argc, argv) {
}

Application::~Application() = default;

} // namespace xi
