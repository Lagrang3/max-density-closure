// https://open.kattis.com/problems/maxflow

#include <cassert>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>

typedef long long flow_t;

/* FIFO variant of Goldbert-Tarjan's Preflow-Push algorithm.
 * O(n^3) */
int preflow_push(const std::vector<std::vector<int>>& node_arcs,
		 std::vector<flow_t>& residual_capacity,
		 const std::vector<int>& next_node, const int source,
		 const int sink) {
	const int N_nodes = node_arcs.size();
	std::vector<int> distance(N_nodes, 0);
	distance[sink] = 0;
	distance[source] = N_nodes;
	std::vector<flow_t> excess(N_nodes, 0);
	std::queue<int> q;

	excess[source] = std::numeric_limits<flow_t>::max();

	auto push = [&](int arc) {
		int dual = arc ^ 1;
		int a = next_node[dual], b = next_node[arc];
		flow_t flow = std::min(excess[a], residual_capacity[arc]);

		residual_capacity[arc] -= flow;
		residual_capacity[dual] += flow;
		excess[a] -= flow;
		excess[b] += flow;

		if (excess[b] == flow && flow > 0 && b != source && b != sink)
			q.push(b);
	};
	auto relabel = [&](int node) {
		int dmin = std::numeric_limits<int>::max();

		for (auto arc : node_arcs[node]) {
			int next = next_node[arc];
			if (residual_capacity[arc] > 0)
				dmin = std::min(dmin, distance[next]);
		}

		if (dmin < std::numeric_limits<int>::max())
			distance[node] = dmin + 1;
		else
			distance[node]++;
	};
	auto discharge = [&](int node) {
		while (true) {
			for (int arc : node_arcs[node]) {
				int next = next_node[arc];
				if (residual_capacity[arc] > 0 &&
				    distance[node] > distance[next])
					push(arc);
			}

			if (excess[node] == 0) break;

			// relabel(node);
			distance[node]++;
		}
	};

	for (int arc : node_arcs[source]) push(arc);
	while (!q.empty()) {
		int node = q.front();
		q.pop();

		assert(node != source && node != sink);
		discharge(node);
	}
	return excess[sink];
}

int main() {
	int N, M, S, T;
	std::cin >> N >> M >> S >> T;

	std::vector<int> next_node(2 * M);
	std::vector<flow_t> residual_capacity(2 * M);
	std::vector<std::vector<int>> node_arcs(N);

	for (int i = 0; i < M; ++i) {
		int a, b, c;
		std::cin >> a >> b >> c;
		int arc = 2 * i, dual = arc + 1;

		next_node[arc] = b;
		next_node[dual] = a;

		residual_capacity[arc] = c;
		residual_capacity[dual] = 0;

		node_arcs[a].push_back(arc);
		node_arcs[b].push_back(dual);
	}

	const flow_t max_flow =
	    preflow_push(node_arcs, residual_capacity, next_node, S, T);

	int M_count = 0;
	for (int i = 0; i < M; ++i) {
		int arc = 2 * i, dual = arc + 1;
		flow_t my_f = residual_capacity[dual];
		M_count += (my_f > 0 ? 1 : 0);
	}

	std::cout << N << ' ' << max_flow << ' ' << M_count << '\n';

	for (int i = 0; i < M; ++i) {
		int arc = 2 * i, dual = arc + 1;
		flow_t my_f = residual_capacity[dual];
		if (my_f == 0) continue;
		int a = next_node[dual], b = next_node[arc];
		std::cout << a << ' ' << b << ' ' << my_f << '\n';
	}

	return 0;
}
