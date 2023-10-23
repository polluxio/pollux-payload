#include <iostream>
#include <string>
#include <cxxabi.h>
#include "pollux_api.h"
#include <fstream>

using namespace std;

int main() {
    RUN_POLLUX([]() { std::cout << 34; } )
    return 0;
}