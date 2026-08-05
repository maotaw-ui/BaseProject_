#include "Application.hpp"

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int) {

    Application application;
    return application.Run(instance);
}
