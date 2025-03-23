/* Maximum feerate closure.
 *
 * Input:
 * N M // N: number of transactions, M: number of dependencies
 * f_i, z_i // N lines, one for each transaction, f_i: the fee of transaction i,
 * z_i: the size of transaction i
 * a_i b_i // M lines, one for each dependency, a_i depends on b_i.
 * */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include <vector>

const int MAX_ID = 32;

bool in_set(int set, int i) { return (set & (1 << i)) > 0; }

struct feefrac {
        unsigned int fee{0}, size{0};

        feefrac& operator+=(const feefrac& that) {
                fee += that.fee;
                size += that.size;
                return *this;
        }

        bool operator<(const feefrac& that) const {
                if (size == 0 && that.size == 0) return fee < that.fee;
                if (size == 0) return true;
                if (that.size == 0) return false;
                return fee * that.size < size * that.fee;
        }
};

std::ostream& operator<<(std::ostream& os, const feefrac& f) {
        if (f.size == 0)
                os << "NAN";
        else
                os << double(f.fee) / f.size;
        os << " " << f.fee << " " << f.size;
        return os;
}

feefrac compute_feerate(std::span<feefrac> rates, int bitset) {
        feefrac r;
        for (int i = 0; i < MAX_ID; i++)
                if (in_set(bitset, i)) {
                        r += rates[i];
                }
        return r;
}

bool is_closure(const std::vector<int>& dependency, int bitset) {
        int dep = bitset;
        for (int i = 0; i < MAX_ID; i++)
                if (in_set(bitset, i)) {
                        dep |= dependency[i];
                }
        return dep == bitset;
}

int set_size(int bitset) {
        int size = 0;
        for (int i = 0; i < MAX_ID; i++) size += in_set(bitset, i) ? 1 : 0;
        return size;
}

/* Max density closure using Gallo-Grigoriadis-Tarjan */
int max_density_closure_GGT(std::span<feefrac> rates,
                            const std::vector<int>& dependency) {
        const int N = std::size(rates);
        const int max_bitset = (1 << N);
        int best_set = 0;
        feefrac best_fr;

        for (int bs = 1; bs < max_bitset; bs++)
                if (is_closure(dependency, bs)) {
                        feefrac fr = compute_feerate(rates, bs);
                        if (best_fr < fr) {
                                best_fr = fr;
                                best_set = bs;
                        }
                }
        return best_set;
}

int main() {
        int N, M;
        std::cin >> N >> M;
        std::vector<feefrac> txs(N);
        std::vector<int> dependency(N, 0);
        for (int i = 0; i < N; i++) std::cin >> txs[i].fee >> txs[i].size;
        for (int i = 0; i < M; i++) {
                int a, b;
                /* a->b, ie. a is a child tx of b */
                std::cin >> a >> b;
                dependency[a] |= (1 << b);
        }
        int answer = max_density_closure_GGT(txs, dependency);
        auto fr = compute_feerate(txs, answer);
        std::cout << fr << "\n";
        std::cout << set_size(answer) << " ";
        for (int i = 0; i < MAX_ID; i++)
                if (in_set(answer, i)) {
                        std::cout << i << " ";
                }
        std::cout << std::endl;
        return 0;
}
