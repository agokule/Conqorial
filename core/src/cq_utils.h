#include <array>
#include <stdint.h>


template<size_t Size>
struct ScrollingHistory {
    std::array<int, Size> data;

    int push(int x) {
        int previous_val {x};
        for (auto it = data.rbegin(); it != data.rend(); it++) {
            int new_val = *it;
            *it = previous_val;
            previous_val = new_val;
        }
        return previous_val;
    }
};

