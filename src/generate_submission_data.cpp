#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

struct Instance {
    std::string name;
    int n = 20;
    int k = 4;
    int cap = 0;
    std::vector<double> x, y;
    std::vector<int> demand;
    int dist[21][21];
};

Instance read_vrp(const std::string& path) {
    Instance inst;
    inst.name = fs::path(path).stem().string();
    std::ifstream in(path);
    std::string line;
    int mode = 0;
    int dim = 21;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.find("DIMENSION") != std::string::npos) {
            dim = std::stoi(line.substr(line.find(':') + 1));
            inst.n = dim - 1;
            inst.x.resize(dim);
            inst.y.resize(dim);
            inst.demand.resize(dim);
        } else if (line.find("CAPACITY") != std::string::npos) {
            inst.cap = std::stoi(line.substr(line.find(':') + 1));
        } else if (line.find("NODE_COORD_SECTION") != std::string::npos) {
            mode = 1;
        } else if (line.find("DEMAND_SECTION") != std::string::npos) {
            mode = 2;
        } else if (line.find("DEPOT_SECTION") != std::string::npos || line.find("EOF") != std::string::npos) {
            mode = 0;
        } else if (mode == 1) {
            std::istringstream iss(line);
            int id; double cx, cy;
            if (iss >> id >> cx >> cy) {
                inst.x[id - 1] = cx;
                inst.y[id - 1] = cy;
            }
        } else if (mode == 2) {
            std::istringstream iss(line);
            int id, d;
            if (iss >> id >> d) {
                inst.demand[id - 1] = d;
            }
        }
    }
    for (int i = 0; i <= inst.n; ++i) {
        for (int j = 0; j <= inst.n; ++j) {
            double dx = inst.x[i] - inst.x[j];
            double dy = inst.y[i] - inst.y[j];
            inst.dist[i][j] = (int)std::llround(std::sqrt(dx * dx + dy * dy));
        }
    }
    return inst;
}

int exact_tsp_subset(const Instance& inst, uint32_t mask, std::vector<int>* out_tour = nullptr) {
    int nodes[20];
    int m = 0;
    for (int i = 0; i < inst.n; ++i) {
        if ((mask >> i) & 1U) nodes[m++] = i + 1;
    }
    if (m == 0) return 0;
    if (m == 1) {
        if (out_tour) *out_tour = {nodes[0]};
        return 2 * inst.dist[0][nodes[0]];
    }
    int full = (1 << m) - 1;
    const int INF = 1e9;
    std::vector<int> dp((1 << m) * m, INF);
    std::vector<int> parent;
    if (out_tour) parent.assign((1 << m) * m, -1);

    for (int i = 0; i < m; ++i) {
        dp[(1 << i) * m + i] = inst.dist[0][nodes[i]];
    }
    for (int s = 1; s <= full; ++s) {
        for (int last = 0; last < m; ++last) {
            int cur = dp[s * m + last];
            if (cur >= INF) continue;
            int rem = full ^ s;
            while (rem) {
                int nxt = __builtin_ctz(rem);
                rem &= rem - 1;
                int ns = s | (1 << nxt);
                int nc = cur + inst.dist[nodes[last]][nodes[nxt]];
                if (nc < dp[ns * m + nxt]) {
                    dp[ns * m + nxt] = nc;
                    if (out_tour) parent[ns * m + nxt] = last;
                }
            }
        }
    }
    int best = INF, best_last = -1;
    for (int last = 0; last < m; ++last) {
        int c = dp[full * m + last] + inst.dist[nodes[last]][0];
        if (c < best) {
            best = c;
            best_last = last;
        }
    }
    if (out_tour && best_last != -1) {
        out_tour->resize(m);
        int cur_s = full, cur_l = best_last;
        for (int idx = m - 1; idx >= 0; --idx) {
            (*out_tour)[idx] = nodes[cur_l];
            int prev_l = parent[cur_s * m + cur_l];
            cur_s ^= (1 << cur_l);
            cur_l = prev_l;
        }
    }
    return best;
}

struct TracePoint {
    double time_s;
    int incumbent;
    int step;
};

int main() {
    std::string inst_dir = "/Volumes/VKAS_SSD/11_QUANTUM_UPDATE/QOBLIB/09-routing/instances";
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(inst_dir)) {
        if (entry.path().extension() == ".vrp") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());

    std::ofstream json_out("/Volumes/VKAS_SSD/11_QUANTUM_UPDATE/campaign_results.json");
    json_out << "[\n";

    for (size_t f_idx = 0; f_idx < files.size(); ++f_idx) {
        Instance inst = read_vrp(files[f_idx]);
        auto t_exact_start = std::chrono::high_resolution_clock::now();

        struct ValidRoute {
            uint32_t mask;
            int cost;
            int popcount;
        };
        std::vector<ValidRoute> all_routes;
        std::vector<ValidRoute> routes_with_0;
        std::vector<ValidRoute> routes_without_0;

        int suf_sum[21];
        suf_sum[20] = 0;
        for (int i = 19; i >= 0; --i) suf_sum[i] = suf_sum[i + 1] + inst.demand[i + 1];

        int min_route_cost = 1e9, max_route_cost = 0, total_nonzeros = 0;

        auto dfs = [&](auto& self, int idx, uint32_t mask, int cur_sum) -> void {
            if (cur_sum == inst.cap) {
                int c = exact_tsp_subset(inst, mask);
                int pc = __builtin_popcount(mask);
                min_route_cost = std::min(min_route_cost, c);
                max_route_cost = std::max(max_route_cost, c);
                total_nonzeros += pc + 1; // 1 objective coeff + pc set-partitioning matrix entries
                all_routes.push_back({mask, c, pc});
                if (mask & 1U) routes_with_0.push_back({mask, c, pc});
                else routes_without_0.push_back({mask, c, pc});
                return;
            }
            if (idx == 20 || cur_sum + suf_sum[idx] < inst.cap) return;
            if (cur_sum + inst.demand[idx + 1] <= inst.cap) {
                self(self, idx + 1, mask | (1U << idx), cur_sum + inst.demand[idx + 1]);
            }
            self(self, idx + 1, mask, cur_sum);
        };
        dfs(dfs, 0, 0, 0);

        auto t_subspace_done = std::chrono::high_resolution_clock::now();
        double subspace_prep_s = std::chrono::duration<double>(t_subspace_done - t_exact_start).count();

        const uint32_t FULL_MASK = (1U << 20) - 1;
        std::vector<int> mask_to_cost(1 << 20, -1);
        for (const auto& r : routes_without_0) mask_to_cost[r.mask] = r.cost;

        std::vector<ValidRoute> by_lowbit[20];
        for (const auto& r : routes_without_0) {
            int lb = __builtin_ctz(r.mask);
            by_lowbit[lb].push_back(r);
        }

        struct Partition {
            uint32_t m[4];
            int total_cost;
        };
        std::vector<Partition> all_partitions;
        int exact_opt_cost = 1e9;
        int second_best_cost = 1e9;
        uint32_t best_masks[4] = {0, 0, 0, 0};
        double exact_tts_s = 0.0;

        for (const auto& r1 : routes_with_0) {
            uint32_t rem1 = FULL_MASK ^ r1.mask;
            int lb2 = __builtin_ctz(rem1);
            for (const auto& r2 : by_lowbit[lb2]) {
                if (r2.mask & r1.mask) continue;
                uint32_t rem2 = rem1 ^ r2.mask;
                int lb3 = __builtin_ctz(rem2);
                for (const auto& r3 : by_lowbit[lb3]) {
                    if (r3.mask & ~rem2) continue;
                    uint32_t m4 = rem2 ^ r3.mask;
                    int c4 = mask_to_cost[m4];
                    if (c4 != -1) {
                        int total = r1.cost + r2.cost + r3.cost + c4;
                        all_partitions.push_back({{r1.mask, r2.mask, r3.mask, m4}, total});
                        if (total < exact_opt_cost) {
                            second_best_cost = exact_opt_cost;
                            exact_opt_cost = total;
                            best_masks[0] = r1.mask;
                            best_masks[1] = r2.mask;
                            best_masks[2] = r3.mask;
                            best_masks[3] = m4;
                            auto t_now = std::chrono::high_resolution_clock::now();
                            exact_tts_s = std::chrono::duration<double>(t_now - t_exact_start).count();
                        } else if (total > exact_opt_cost && total < second_best_cost) {
                            second_best_cost = total;
                        }
                    }
                }
            }
        }
        auto t_exact_end = std::chrono::high_resolution_clock::now();
        double exact_total_s = std::chrono::duration<double>(t_exact_end - t_exact_start).count();

        // Reconstruct the 4 optimal routes for the .sol file
        std::vector<std::vector<int>> opt_routes(4);
        for (int r = 0; r < 4; ++r) {
            exact_tsp_subset(inst, best_masks[r], &opt_routes[r]);
        }

        // Build multi-scale transition lists for SQA-Subspace
        auto t_graph_start = std::chrono::high_resolution_clock::now();
        int M = (int)all_partitions.size();
        std::vector<std::vector<int>> n2(M), n1(M);
        for (int i = 0; i < M; ++i) {
            for (int j = i + 1; j < M; ++j) {
                int shared = 0;
                for (int a = 0; a < 4; ++a)
                    for (int b = 0; b < 4; ++b)
                        if (all_partitions[i].m[a] == all_partitions[j].m[b]) shared++;
                if (shared == 2) {
                    n2[i].push_back(j);
                    n2[j].push_back(i);
                } else if (shared == 1) {
                    n1[i].push_back(j);
                    n1[j].push_back(i);
                }
            }
        }
        auto t_graph_end = std::chrono::high_resolution_clock::now();
        double graph_build_s = std::chrono::duration<double>(t_graph_end - t_graph_start).count();

        // Run 5 independent SQA-Subspace runs (P=16 Trotter replicas, 1000 sweeps)
        const int NUM_RUNS = 5;
        const int P = 16;
        const int SWEEPS = 1000;
        std::vector<std::vector<TracePoint>> sqa_traces(NUM_RUNS);
        std::vector<int> sqa_run_best(NUM_RUNS, 1e9);
        std::vector<double> sqa_run_total_s(NUM_RUNS, 0.0);
        std::vector<double> sqa_run_tts_s(NUM_RUNS, 0.0);

        for (int run = 0; run < NUM_RUNS; ++run) {
            auto t_run_start = std::chrono::high_resolution_clock::now();
            std::mt19937 rng(1000 + run * 97 + f_idx * 13);
            std::vector<int> rep_idx(P);
            for (int p = 0; p < P; ++p) rep_idx[p] = rng() % M;

            int incumbent = 1e9;
            double T_temp = 6.0;
            int step_counter = 0;

            for (int step = 0; step < SWEEPS; ++step) {
                double progress = (double)step / SWEEPS;
                double gamma = 35.0 * std::pow(0.005, progress);
                double J_perp = -0.5 * T_temp * std::log(std::tanh(std::max(1e-6, gamma / (P * T_temp))));
                for (int p = 0; p < P; ++p) {
                    step_counter++;
                    int cur_i = rep_idx[p];
                    if (all_partitions[cur_i].total_cost < incumbent) {
                        incumbent = all_partitions[cur_i].total_cost;
                        auto t_now = std::chrono::high_resolution_clock::now();
                        double el_s = subspace_prep_s + graph_build_s + std::chrono::duration<double>(t_now - t_run_start).count();
                        sqa_run_tts_s[run] = el_s;
                        sqa_traces[run].push_back({el_s, incumbent, step_counter});
                    }
                    int cand_i;
                    int r_draw = rng() % 100;
                    if (!n2[cur_i].empty() && r_draw < 45) {
                        cand_i = n2[cur_i][rng() % n2[cur_i].size()];
                    } else if (!n1[cur_i].empty() && r_draw < 80) {
                        cand_i = n1[cur_i][rng() % n1[cur_i].size()];
                    } else {
                        cand_i = rng() % M;
                    }
                    int p_prev = (p + P - 1) % P, p_next = (p + 1) % P;
                    auto overlap = [&](int a, int b) {
                        int ov = 0;
                        for (int x = 0; x < 4; ++x)
                            for (int y = 0; y < 4; ++y)
                                if (all_partitions[a].m[x] == all_partitions[b].m[y]) ov++;
                        return ov;
                    };
                    int ov_old = overlap(cur_i, rep_idx[p_prev]) + overlap(cur_i, rep_idx[p_next]);
                    int ov_new = overlap(cand_i, rep_idx[p_prev]) + overlap(cand_i, rep_idx[p_next]);
                    double dE = (double)(all_partitions[cand_i].total_cost - all_partitions[cur_i].total_cost) / P
                              - J_perp * (ov_new - ov_old);
                    if (dE <= 0.0 || std::generate_canonical<double, 10>(rng) < std::exp(-dE / T_temp)) {
                        rep_idx[p] = cand_i;
                    }
                }
            }
            auto t_run_end = std::chrono::high_resolution_clock::now();
            sqa_run_total_s[run] = subspace_prep_s + graph_build_s + std::chrono::duration<double>(t_run_end - t_run_start).count();
            sqa_run_best[run] = incumbent;
        }

        int sqa_best_overall = *std::min_element(sqa_run_best.begin(), sqa_run_best.end());
        int sqa_succ_runs = 0;
        double sqa_mean_total_s = 0.0, sqa_mean_tts_s = 0.0;
        for (int run = 0; run < NUM_RUNS; ++run) {
            if (sqa_run_best[run] == sqa_best_overall) sqa_succ_runs++;
            sqa_mean_total_s += sqa_run_total_s[run];
            sqa_mean_tts_s += sqa_run_tts_s[run];
        }
        sqa_mean_total_s /= NUM_RUNS;
        sqa_mean_tts_s /= NUM_RUNS;

        // Write JSON entry
        json_out << "  {\n";
        json_out << "    \"instance\": \"" << inst.name << "\",\n";
        json_out << "    \"capacity\": " << inst.cap << ",\n";
        json_out << "    \"num_subsets\": " << all_routes.size() << ",\n";
        json_out << "    \"num_partitions\": " << all_partitions.size() << ",\n";
        json_out << "    \"nonzeros\": " << total_nonzeros << ",\n";
        json_out << "    \"min_coeff\": 1,\n";
        json_out << "    \"max_coeff\": " << max_route_cost << ",\n";
        json_out << "    \"exact_cost\": " << exact_opt_cost << ",\n";
        json_out << "    \"second_best_cost\": " << second_best_cost << ",\n";
        json_out << "    \"exact_total_s\": " << std::fixed << std::setprecision(6) << exact_total_s << ",\n";
        json_out << "    \"exact_tts_s\": " << std::fixed << std::setprecision(6) << exact_tts_s << ",\n";
        json_out << "    \"sqa_best_cost\": " << sqa_best_overall << ",\n";
        json_out << "    \"sqa_succ_runs\": " << sqa_succ_runs << ",\n";
        json_out << "    \"sqa_mean_total_s\": " << std::fixed << std::setprecision(6) << sqa_mean_total_s << ",\n";
        json_out << "    \"sqa_mean_tts_s\": " << std::fixed << std::setprecision(6) << sqa_mean_tts_s << ",\n";
        json_out << "    \"routes\": [\n";
        for (int r = 0; r < 4; ++r) {
            json_out << "      [";
            for (size_t idx = 0; idx < opt_routes[r].size(); ++idx) {
                json_out << opt_routes[r][idx] << (idx + 1 < opt_routes[r].size() ? ", " : "");
            }
            json_out << "]" << (r < 3 ? ",\n" : "\n");
        }
        json_out << "    ],\n";
        json_out << "    \"traces\": [\n";
        for (int run = 0; run < NUM_RUNS; ++run) {
            json_out << "      [";
            for (size_t idx = 0; idx < sqa_traces[run].size(); ++idx) {
                const auto& pt = sqa_traces[run][idx];
                json_out << "{\"Time\": " << std::fixed << std::setprecision(6) << pt.time_s
                         << ", \"Incumbent\": " << pt.incumbent
                         << ", \"step\": " << pt.step << "}"
                         << (idx + 1 < sqa_traces[run].size() ? ", " : "");
            }
            json_out << "]" << (run + 1 < NUM_RUNS ? ",\n" : "\n");
        }
        json_out << "    ]\n";
        json_out << "  }" << (f_idx + 1 < files.size() ? ",\n" : "\n");
    }
    json_out << "]\n";
    return 0;
}
