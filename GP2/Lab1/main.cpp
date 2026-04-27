#include "MainGame.h"

// SDL needs to wrap main() on some platforms to handle startup correctly.
// The argc/argv signature is required for that to work, even though we don't use them.
int main(int argc, char** argv)
{
    MainGame mainGame;
    mainGame.run();

    return 0;
}
