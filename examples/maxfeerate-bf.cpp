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

#include "clusterlinearize.h"

/* Max density closure by Brute Force */
int max_density_closure_BF(std::span<const feefrac> rates,
                           std::span<const int> dependency) {
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
        int answer = max_density_closure_BF(txs, dependency);
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
