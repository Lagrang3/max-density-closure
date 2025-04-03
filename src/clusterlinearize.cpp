
#include "clusterlinearize.h"

std::ostream& operator<<(std::ostream& os, const feefrac& f) {
        if (f.size == 0)
                os << "NAN";
        else
                os << double(f.fee) / f.size;
        os << " " << f.fee << " " << f.size;
        return os;
}

int set_size(int bitset) {
        int size = 0;
        for (int i = 0; i < MAX_ID; i++) size += in_set(bitset, i) ? 1 : 0;
        return size;
}

feefrac compute_feerate(std::span<const feefrac> rates, int bitset) {
        feefrac r;
        for (int i = 0; i < MAX_ID; i++)
                if (in_set(bitset, i)) {
                        r += rates[i];
                }
        return r;
}

bool is_closure(std::span<const int> dependency, int bitset) {
        int dep = bitset;
        for (int i = 0; i < MAX_ID; i++)
                if (in_set(bitset, i)) {
                        dep |= dependency[i];
                }
        return dep == bitset;
}

