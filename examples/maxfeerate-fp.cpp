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
#include <queue>
#include <span>
#include <vector>

#include "clusterlinearize.h"

/* Maximum weight closure using Goldberg-Tarjan's Preflow-Push. */
int max_weight_closure(std::span<const long long> weights,
                       std::span<const int> dependency) {
        const int N = std::size(weights);
        std::vector<int> distance(N, 0);
        std::vector<long long> excess(N, 0);

        std::vector<long long> flow(N * N, 0);
        std::vector<long long> cap_to_sink(N, 0);
        std::queue<int> Q;

        // build the graph
        for (int i = 0; i < N; i++) {
                if (weights[i] > 0) {
                        // we initially saturate all arcs from the source
                        excess[i] += weights[i];
                        Q.push(i);
                } else
                        cap_to_sink[i] = -weights[i];
        }

        auto push_to_sink = [&](int node) {
                long long f = std::min(excess[node], cap_to_sink[node]);
                excess[node] -= f;
                cap_to_sink[node] -= f;
        };

        auto push = [&](int node, int next) {
                long long f = excess[node];

                if (in_set(dependency[node], next)) {
                        // infinite capacity on this arc
                        flow[node * N + next] += f;
                } else if (in_set(dependency[next], node)) {
                        // finite residual capacity
                        f = std::min(f, flow[next * N + node]);
                        flow[next * N + node] -= f;
                } else {
                        // no connection at all
                        f = 0;
                }

                excess[node] -= f;
                excess[next] += f;
                if (excess[next] == f && f > 0) Q.push(next);
        };
        auto discharge = [&](int node) {
                while (distance[node] < N + 2 && excess[node] > 0) {
                        // can we push to the sink?
                        push_to_sink(node);
                        if (excess[node] == 0) break;

                        // can we push to another node?
                        for (int next = 0; next < N; next++)
                                if (distance[node] > distance[next])
                                        push(node, next);
                        if (excess[node] == 0) break;

                        // relabel
                        distance[node]++;
                }
        };

        // preflow-push until there are no more active nodes
        while (!Q.empty()) {
                int node = Q.front();
                Q.pop();
                discharge(node);
        }

        // this computes a min-cut of the biggest size possible
        int can_reach_sink = 0;
        // from the sink
        for (int i = 0; i < N; i++)
                if (cap_to_sink[i] > 0) {
                        can_reach_sink |= (1 << i);
                        Q.push(i);
                }
        auto flood = [&](int node) {
                for (int prev = 0; prev < N; prev++)
                        if (!in_set(can_reach_sink, prev) &&
                            (in_set(dependency[prev], node) ||
                             flow[node * N + prev] > 0)) {
                                can_reach_sink |= (1 << prev);
                                Q.push(prev);
                        }
        };
        while (!Q.empty()) {
                int node = Q.front();
                Q.pop();
                flood(node);
        }
        return (~can_reach_sink) & ((1 << N) - 1);
}

/* Max density closure using Fractional Programming and maxflow */
int max_density_closure_FP(std::span<const feefrac> rates,
                           std::span<const int> dependency) {
        const int N = std::size(rates);
        std::vector<long long> weights(N);
        const int max_bitset = (1 << N);
        int best_set = 0;
        feefrac best_fr;

        // start with some initial solution
        for (int i = 0; i < N; i++) {
                if (dependency[i] == 0) {
                        best_set = (1 << i);
                        best_fr = rates[i];
                        break;
                }
        }

        // produce an increasing sequence of rates
        while (1) {
                for (int i = 0; i < N; i++)
                        weights[i] = rates[i].cross(best_fr);

                int x = max_weight_closure(weights, dependency);

                feefrac fr = compute_feerate(rates, x);
                if (best_fr < fr) {
                        best_fr = fr;
                        best_set = x;
                } else {
                        // assert(x == 0);
                        break;
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
        int answer = max_density_closure_FP(txs, dependency);
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
