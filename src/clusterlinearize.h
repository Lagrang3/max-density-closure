#pragma once

#include <iostream>
#include <span>

struct feefrac {
        unsigned int fee{0}, size{0};

        feefrac& operator+=(const feefrac& that) {
                fee += that.fee;
                size += that.size;
                return *this;
        }

        long long cross(const feefrac& that) const {
                return ((long long)fee * that.size) -
                       ((long long)size * that.fee);
        }

        bool operator<(const feefrac& that) const {
                if (size == 0 && that.size == 0) return fee < that.fee;
                if (size == 0) return true;
                if (that.size == 0) return false;
                return (*this).cross(that) < 0;
        }

        bool operator==(const feefrac& that) const {
                return size == that.size && fee == that.fee;
        }
};

std::ostream& operator<<(std::ostream& os, const feefrac& f);

const int MAX_ID = 32;

/* Is element with index i in the set. */
inline bool in_set(int set, int i) { return (set & (1 << i)) > 0; }

/* How many elements are there. */
int set_size(int bitset);

/* Given an indexed vector of feerates and a subset, compute the feerate of the
 * subset. */
feefrac compute_feerate(std::span<const feefrac> rates, int bitset);

/* Given a dependency graph and a subset, answer whether the subset is a
 * closure, ie. doesn't have external dependencies. */
bool is_closure(std::span<const int> dependency, int bitset);
