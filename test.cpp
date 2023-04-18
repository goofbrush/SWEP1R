#include "Racer.h"

class work : Racer {
public:
    void run() {
        init();
        while(true) {
            update();
        }
    }
};

int main() {
    work w;
    w.run();

    return 0;
}