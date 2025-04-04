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

int can_reach_sink(std::span<const int> dependency,
                   std::span<const long long> cap_to_sink,
                   std::span<const long long> flow,
                   std::span<const long long> flow_to_sink) {
        const int N = dependency.size();
        int answer = 0;
        std::queue<int> Q;

        /* starting from the sink, which nodes can reach it in the residual
         * network? */
        for (int i = 0; i < N; i++)
                if (cap_to_sink[i] > flow_to_sink[i]) {
                        answer |= (1 << i);
                        Q.push(i);
                }

        auto walk = [&](int node) {
                for (int prev = 0; prev < N; prev++)
                        /* scan all nodes that can reach me in the residual
                         * network */
                        if (!in_set(answer, prev) &&
                            (in_set(dependency[prev], node) ||
                             flow[node * N + prev] > 0)) {
                                answer |= (1 << prev);
                                Q.push(prev);
                        }
        };

        while (!Q.empty()) {
                int node = Q.front();
                Q.pop();
                walk(node);
        }

        return answer;
}

/* FIXME: too many arguments, we might need fewer */
int compute_min_cut(std::span<const long long> cap_to_source,
                    std::span<const long long> cap_to_sink,
                    std::span<const int> dependency, std::span<long long> flow,
                    std::span<long long> flow_to_source,
                    std::span<long long> flow_to_sink,
                    std::span<long long> excess, std::span<int> distance) {
        const int N = std::size(dependency);
        std::queue<int> Q;

        /* normal push */
        auto push = [&](int node, int next) {
                long long f = excess[node];
                if (in_set(dependency[node], next)) {
                        /* arc with infinite capacity */
                        flow[node * N + next] += f;
                } else if (in_set(dependency[next], node)) {
                        /* finite residual capacity */
                        f = std::min(f, flow[next * N + node]);
                        flow[next * N + node] -= f;
                } else {
                        /* they are not connected */
                        f = 0;
                }

                excess[node] -= f;
                excess[next] += f;
                if (excess[next] == f && f > 0) Q.push(next);
        };

        /* a push towards the sink */
        auto push_to_sink = [&](int node) {
                long long f = std::min(excess[node],
                                       cap_to_sink[node] - flow_to_sink[node]);
                excess[node] -= f;
                flow_to_sink[node] += f;
        };

        /* pushes towards the source are not required since we are interested in
         * the min-cut and not the state of the maxflow. */

        /* discharge = push/relabel while node is active */
        auto discharge = [&](int node) {
                /* a node with distance >= N+2 cannot reach the sink */
                while (distance[node] < N + 2 && excess[node] > 0) {
                        /* can we push to the sink? */
                        push_to_sink(node);
                        if (excess[node] == 0) break;

                        /* can we push to another node? */
                        for (int next = 0; next < N; next++)
                                if (distance[node] > distance[next])
                                        push(node, next);
                        if (excess[node] == 0) break;

                        /* cannot push any more but we are still active,
                         * relabel.
                         * FIXME: we may save a bit of time if we collect the
                         * minimum label of all neighboring nodes and use
                         * relabel to that + 1. */
                        distance[node]++;
                }
        };

        /* identify the first active nodes and queue them */
        for (int i = 0; i < N; i++)
                if (distance[i] < N + 1 && excess[i] > 0) Q.push(i);

        /* push/relabel until there are no more active nodes */
        while (!Q.empty()) {
                int node = Q.front();
                Q.pop();
                discharge(node);
        }

        return can_reach_sink(dependency, cap_to_sink, flow, flow_to_sink);
}

/* Max density closure using "A Fast Parametric Maximum Flow Algorithm" by
 * Gallo, Grigoriadis and Tarjan. */
int max_density_closure_ggt(std::span<const feefrac> rates,
                            std::span<const int> dependency) {
        const int N = std::size(rates);

        /* weights on the nodes weight[i] = fee[i] - size[i] * target_rate */
        std::vector<long long> weights(N);

        /* capacity on the network */
        std::vector<long long> cap_to_sink(N, 0), cap_to_source(N, 0);

        auto build_graph = [&]() {
                for (int i = 0; i < N; i++) {
                        /* Notice this is the reversed graph. */
                        if (weights[i] > 0) {
                                cap_to_sink[i] = weights[i];
                        } else {
                                cap_to_source[i] = -weights[i];
                        }
                }
        };

        /* state of the flow */
        std::vector<long long> excess(N, 0), flow(N * N, 0), flow_to_sink(N, 0),
            flow_to_source(N, 0);

        /* a valid labeling the source and sink are not explicity here */
        std::vector<int> distance(N, 0);

        /* how we update the weights of nodes */
        auto compute_weights = [&](feefrac target) {
                const long long FRACTION_LIFT = 1000000;
                for (int i = 0; i < N; i++)
                        weights[i] =
                            FRACTION_LIFT * rates[i].fee -
                            rates[i].size *
                                ((FRACTION_LIFT * target.fee) / target.size);
        };

        /* saturate the source and reduce the flow on the sink side, it is
         * assumed that the capacity on the source arcs cannot decrease and
         * the capacity on the sink arcs cannot increase. */
        auto saturate_source = [&]() {
                long long df;
                for (int i = 0; i < N; i++) {
                        // saturate source arcs
                        if (distance[i] < N + 2) {
                                df = cap_to_source[i] - flow_to_source[i];
                                assert(df >= 0);
                                excess[i] += df;
                                flow_to_source[i] += df;
                                assert(cap_to_source[i] >= flow_to_source[i]);
                        }

                        // reduce the flow on the sink arcs
                        df = flow_to_sink[i] -
                             std::min(flow_to_sink[i], cap_to_sink[i]);
                        assert(df >= 0);
                        excess[i] += df;
                        flow_to_sink[i] -= df;
                        assert(cap_to_sink[i] >= flow_to_sink[i]);
                }
        };

        /* arcs directions inverted */
        std::vector<int> rev_dependency(N, 0);
        for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++) {
                        if (in_set(dependency[i], j))
                                rev_dependency[j] |= (1 << i);
                }

        int best_set = 0;
        feefrac best_fr;

        /* start with some initial possible solution */
        for (int i = 0; i < N; i++) {
                if (dependency[i] == 0) {
                        best_set = (1 << i);
                        best_fr = rates[i];
                        break;
                }
        }

        /* first min-cut */
        compute_weights(best_fr);
        build_graph();
        saturate_source();
        int x =
            compute_min_cut(cap_to_source, cap_to_sink, rev_dependency, flow,
                            flow_to_source, flow_to_sink, excess, distance);
        feefrac fr = compute_feerate(rates, x);

        /* x is at least as good as best_set */
        assert(!(fr < best_fr));
        best_fr = fr;
        best_set = x;

        /* produce an increasing sequence of rates, we re-use the flow and
         * labels from every iterations. */
        while (true) {
                compute_weights(best_fr);
                build_graph();
                saturate_source();
                x = compute_min_cut(cap_to_source, cap_to_sink, rev_dependency,
                                    flow, flow_to_source, flow_to_sink, excess,
                                    distance);

                /* verify the nesting property */
                assert((best_set & x) == best_set);

                fr = compute_feerate(rates, x);
                if (best_fr < fr) {
                        best_fr = fr;
                        best_set = x;
                } else {
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
        int answer = max_density_closure_ggt(txs, dependency);
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
