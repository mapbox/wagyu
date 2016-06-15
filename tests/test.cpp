// https://github.com/philsquared/Catch/blob/master/docs/own-main.md
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* const argv[]) {
    int result = Catch::Session().run(argc, argv);
    if (!result)
        printf("\x1b[1;32m ✓ \x1b[0m\n");
    return result;
}
