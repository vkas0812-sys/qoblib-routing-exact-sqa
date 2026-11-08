# Methodology: `TC-Exact` and `SQA-Subspace`

This document details the mathematical design, algorithmic steps, parameter choices, scaling limits, and known limitations of the two solvers implemented in this repository: **`TC-Exact`** and **`SQA-Subspace`** (covering **Questions C.10–C.14, D.15–D.18, and G.29–G.30**).

---

## Part I: `TC-Exact` — Tight-Capacity Bitwise Set-Partitioning Solver (Questions C.10–C.14)

### 1. What is `TC-Exact` and What Does "TC" Stand For? (Question C.10)
- **`TC-Exact`** stands for **T**ight-**C**apacity **Exact** Set-Partitioning Solver.
- It is a deterministic, single-threaded C++17 exact algorithm that exploits the universal zero-slack property ($\sum_{i=1}^{20} d_i = 4Q$) of QOBLIB `09-routing` to decompose the 4-vehicle CVRP into three exact bitwise stages:
  1. **Stage 1 — Tight-Subset Bitmask Enumeration ($\mathcal{F}_1$):** Uses recursive depth-first subset-sum search with suffix-sum pruning over $20$-bit integers (`uint32_t mask`) to find all customer subsets whose total demand equals exactly $Q$:
     $$\mathcal{F}_1 = \left\{ m \in \{0, \dots, 2^{20}-1\} \;\middle|\; \sum_{i : \text{bit}_i(m)=1} d_i = Q \right\}.$$
     Across all 55 instances, $|\mathcal{F}_1|$ contains only **$300$ to $961$ subsets** (out of $2^{20} = 1,048,576$ possible subsets).
  2. **Stage 2 — Exact Held-Karp TSP DP ($c^*(m)$):** For each valid subset $m \in \mathcal{F}_1$ (typically $|m| \in [3, 8]$ customers), computes the exact minimum TSP tour cost $c^*(m)$ starting and ending at depot $0$ in $O(|m|^2 2^{|m|})$ bitwise operations.
  3. **Stage 3 — Symmetry-Broken Bitwise Exact Cover ($\mathcal{P}_4$):** Finds 4 pairwise-disjoint masks $m_1, m_2, m_3, m_4 \in \mathcal{F}_1$ whose bitwise OR covers all 20 customers ($m_1 \vee m_2 \vee m_3 \vee m_4 = 2^{20}-1$) minimizing $\sum_{r=1}^4 c^*(m_r)$.

### 2. How is the Exact Optimality Certificate Proven? (Question C.11)
Unlike LP-based Branch-and-Bound (which relies on fractional dual lower bounds), `TC-Exact` proves global optimality via **complete, exhaustive enumeration of the mathematically equivalent feasible Set-Partitioning space $\mathcal{P}_4$**:
1. **Necessary & Sufficient Feasibility Equivalence (Proposition 1):** By $\sum_{i=1}^{20} d_i = 4Q$ and $\sum_{i \in S_r} d_i \le Q$, *every* feasible 4-vehicle solution to the CVRP must partition $\{1, \dots, 20\}$ into four disjoint subsets $S_1, S_2, S_3, S_4 \in \mathcal{F}_1$. No feasible solution can exist outside $\mathcal{P}_4$.
2. **Exact Within-Route Optimality:** For any subset $m \in \mathcal{F}_1$, Held-Karp dynamic programming evaluates all possible customer permutations and returns the exact minimum tour cost $c^*(m)$.
3. **Symmetry-Free Complete Partition Enumeration:**
   - Vehicle 1 is anchored to contain customer $0$ (`routes_with_0`), eliminating $4 \times$ symmetry.
   - Given $m_1$, the remaining uncovered customers are `rem1 = FULL_MASK ^ m1`. Vehicle 2 is anchored to contain the lowest-indexed uncovered customer `lb2 = __builtin_ctz(rem1)` (`by_lowbit[lb2]`), eliminating the remaining $3 \times$ symmetry.
   - Given $m_1, m_2$, Vehicle 3 is anchored to the lowest set bit `lb3 = __builtin_ctz(rem2)` of `rem2 = rem1 ^ m2`, eliminating the final $2 \times$ symmetry ($4 \times 3 \times 2 = 24 = 4!$).
   - Vehicle 4 is uniquely determined as `m4 = rem2 ^ m3` and checked in $O(1)$ time via `mask_to_cost[m4]`.
4. **Certificate:** Because every single valid 4-partition in $\mathcal{P}_4$ ($|\mathcal{P}_4| \in [372, 5630]$) is generated and scored with its exact TSP cost, the minimum over $\mathcal{P}_4$ is a **strict mathematical certificate of global optimality** (`Best Objective Value == Optimality Bound`), simultaneously yielding the exact second-best partition cost $C^{(2)}$.

### 3. Foundational Algorithms & Papers Behind `TC-Exact` (Question C.12)
- **Set-Partitioning CVRP Formulation:** Balinski & Quandt (1964), *Operations Research*.
- **Bitmask Dynamic Programming for TSP:** Held & Karp (1962), *J. SIAM*.
- **Lowest-Bit Anchored Exact Cover:** Knuth (2000), *Dancing Links* (anchoring the first uncovered column to prevent duplicate permutations of identical sets).

### 4. Scaling Limits, Runtime & Memory Footprint (Question C.13)
- **Current Benchmark ($n = 20, K = 4$, $\Delta = 0$):**
  - **Runtime:** Mean **3.33 ms/instance** (min `0.53 ms` on `XSH-n20-k4-53`, max `14.64 ms` on `XSH-n20-k4-02`, total **0.183 s** for all 55 instances on Apple M4).
  - **Memory Footprint:** **`< 5 MB` RAM** (dominated by `std::vector<int> mask_to_cost(1 << 20, -1)` which occupies exactly $2^{20} \times 4\text{ bytes} = 4\text{ MB}$, plus $< 100\text{ KB}$ for $\mathcal{F}_1$ and $\mathcal{P}_4$).
- **Practical Scaling Limits of `TC-Exact`:**
  - **Customer count $n$:** With bitwise exact cover and Held-Karp DP, `TC-Exact` scales cleanly up to **$n \approx 25\text{--}30$ customers** (where $2^n$ lookup tables can be replaced by a hash table `ankerl::unordered_dense` or sorted binary search, and route lengths remain $|m| \le 10$). Beyond $n > 35$, $|\mathcal{F}_1|$ and $|\mathcal{P}_K|$ grow combinatorially, requiring LP column generation (Branch-Cut-and-Price).
  - **Fleet slack $\Delta = KQ - \sum d_i$:** `TC-Exact` is sharpest when $\Delta = 0$ (100% tight) or when $\Delta$ is small ($\Delta \ll Q$). If $\Delta$ is large, the number of valid single-vehicle subsets $\sum_{i \in S} d_i \le Q$ approaches $\sum_{j=1}^{\lfloor n/K \rfloor} \binom{n}{j}$, weakening the subset-sum filter.

### 5. What Does "Standalone Packaging" in PR #93 Mean? (Question C.14)
- Initially, our submission to QOBLIB PR #93 combined both `TC-Exact` and `SQA-Subspace` in a single folder (`20261017_TightSubspaceSQA_TCExact_Vuong`), reporting `TC-Exact` as `Value 1` and `SQA-Subspace` as `Value 2` in each README table.
- Following review feedback from QOBLIB maintainer **Maximilian Schicker (`@mlschicker`, ZIB)**—who requested that each distinct solver/method be submitted in its own separate submission folder and Pull Request so the QOBLIB leaderboard parser can track one method per submission—we updated `scripts/build_submission.py` to support `--mode tc-exact` (default).
- **"Standalone packaging"** means generating the submission directory `20261017_TCExact_Vuong` containing **exclusively `TC-Exact`** (a single `Value 1` column per README, `55/55` proven optimal `.sol` files, no `traces/` subfolder needed since `TC-Exact` is a deterministic exact solver).

---

## Part II: `SQA-Subspace` — Subspace Simulated Quantum Annealing (Questions D.15–D.18)

### 6. How Does `SQA-Subspace` Differ from Standard SQA? What is the "Subspace"? (Question D.15)
| Feature | Standard `SQA-Direct` (Penalty Ising/QUBO) | Our `SQA-Subspace` (Manifold PIMC-SQA) |
| :--- | :--- | :--- |
| **State Space** | Unconstrained spin space $\{0,1,2,3\}^{20}$ ($4^{20} \approx 1.1 \times 10^{12}$ states) or QUBO $\{0,1\}^{4527}$ | **Tight-Capacity Partition Manifold $\mathcal{P}_4$** ($|\mathcal{P}_4| \in [372, 5630]$ states) |
| **Capacity Handling** | Soft quadratic penalty $\lambda_{\text{cap}} \sum_r (\sum_{i \in S_r} d_i - Q)^2$ | **Hard structural projection** ($\sum_{i \in S_r} d_i = Q$ identically satisfied) |
| **Elementary Moves** | 1-customer relocate / 2-customer swap (violates capacity when $\Delta = 0$) | **Multi-route recombination tunneling** ($k \in \{2, 3, 4\}$ routes remixed simultaneously while preserving $\sum d_i = kQ$) |
| **Inter-Replica Coupling** | Spin-wise Kronecker delta $\sum_{i=1}^{20} \delta(v_i^{(p)}, v_i^{(p+1)})$ | **Route-subset overlap** $\mathcal{O}(\mathcal{P}^{(p)}, \mathcal{P}^{(p+1)}) \in \{0, 1, 2, 4\}$ |
| **Feasible / Optimal Rate** | **0 / 55 Feasible (0.0%)** | **55 / 55 Optimal (100.0%)** |

- **What is the "Subspace"?** It is the **Zero-Slack Set-Partitioning Manifold $\mathcal{P}_4$**: the discrete space of all 4-tuples of pairwise-disjoint customer subsets $\{m_1, m_2, m_3, m_4\} \subset \mathcal{F}_1$ that partition $\{1, \dots, 20\}$ with exact vehicle load $\sum_{i \in m_r} d_i = Q$ on every route.

### 7. Justification of `SQA-Subspace` Hyperparameters (Question D.16)
In `src/generate_submission_data.cpp` (lines 276–333), `SQA-Subspace` uses the following fixed parameters across all 55 instances:
1. **Number of Trotter Slices ($P = 16$):**
   - Balances quantum worldline resolution against per-sweep CPU cost. At $P = 16$, adjacent slices provide smooth imaginary-time correlation while executing 1,000 sweeps ($16,000$ replica updates) in $\sim 0.4\text{ ms}$ of pure Monte Carlo time.
2. **Transverse Field Schedule ($\Gamma(t) = 35.0 \times 0.005^{t/1000}$, from $\Gamma_0 = 35.0 \to \Gamma_f = 0.175$):**
   - Typical partition cost gaps between local minima are $\Delta E \in [1, 30]$.
   - At $t = 0$ ($\Gamma_0 = 35.0$, $T = 6.0$, $P = 16$), $\frac{\Gamma_0}{PT} \approx 0.365 \implies J_{\perp}(0) = -\frac{1}{2}(6.0)\ln\tanh(0.365) \approx 3.16$. This moderate initial coupling allows the 16 replicas to spread across distinct basins of $\mathcal{P}_4$.
   - At $t = 1000$ ($\Gamma_f = 0.175$), $\frac{\Gamma_f}{PT} \approx 0.00182 \implies J_{\perp}(1000) \approx 18.93 \gg \frac{\Delta E}{P}$, strongly locking all 16 Trotter replicas onto the ground-state partition.
3. **Effective Temperature ($T = 6.0$):**
   - Because classical energy differences in the Suzuki-Trotter action are scaled by $1/P$ ($\Delta E_{\text{slice}} = \Delta E_{\text{CVRP}} / 16$), a route cost difference of $\Delta E_{\text{CVRP}} = 16$ corresponds to $\Delta E_{\text{slice}} = 1.0$. Setting $T = 6.0$ prevents premature thermal freezing in the first half of the schedule while letting $J_{\perp}(t)$ drive convergence in the second half.
4. **Number of Sweeps ($\text{SWEEPS} = 1,000$, i.e., $16,000$ replica updates per run):**
   - Empirically sufficient for 96.7% single-run global optimality ($266 / 275$ runs across all 55 instances) and 100% 5-run optimality ($55/55$ instances). On the hardest instance `XSH-n20-k4-21`, Run #1 reaches the global optimum at sweep 73 (update 1,154).
5. **Multi-Scale Tunneling Proposal Probabilities (`45% / 35% / 20%`):**
   - `45%` 2-route recombination (`n2[cur_i]`, sharing 2 routes, remixing 2 routes of total demand $2Q$).
   - `35%` 3-route recombination (`n1[cur_i]`, sharing 1 route, remixing 3 routes of total demand $3Q$).
   - `20%` 4-route global tunneling (sharing 0 routes, jumping across disconnected components where `n2` and `n1` are empty).

### 8. Foundational Papers & Code Origin for `SQA-Subspace` (Question D.17)
- **Theoretical Basis:** Suzuki-Trotter Path-Integral Monte Carlo (PIMC) formulation of Santoro et al. (2002, *Science*) and Martoňák, Santoro, & Tosatti (2002, *Phys. Rev. B*), combined with constraint-preserving subspace transitions inspired by Quantum Alternating Operator Ansatz mixers (Hadfield et al., 2019).
- **Code Origin:** Written from scratch in standard C++17 (`src/generate_submission_data.cpp` and `src/qoblib_routing_experiments.cpp`) without external third-party libraries.

### 9. Comparisons Against Classical & Quantum-Inspired Baselines (Question D.18)
1. **Direct Experimental Comparison in This Repo (`src/qoblib_routing_experiments.cpp`):**
   - Compares **`TC-Exact`** vs. **`SQA-Subspace`** vs. **`SQA-Direct`** (standard penalty Ising/QUBO SQA with 2-opt routing and adaptive capacity penalty).
2. **Comparison Against Published QOBLIB Baselines:**
   - **MIP Solvers (Gurobi / SCIP / CPLEX on `cvrp_ilp.zpl`) & Unconstrained QUBO Solvers:** Reported in the QOBLIB paper (Koch et al., 2025/2026, `QOBLIB/paper/data/mip_instances_july25.csv` and `qubo_instances_aug25.csv`), where standard MIP/QUBO formulations fail to close 54/55 instances within 1 hour.
   - **Memetic Genetic Algorithm (`ElYaakoubi`, QOBLIB 2026 submission):** Reaches 54/55 optimal in `1.3 s – 48.5 s` per instance, missing `XSH-n20-k4-21` (`Cost = 843` vs. `842`).
   - **HGS-CVRP + VRPSolverEasy (QOBLIB Reference):** Certified 54/55 instances, leaving `XSH-n20-k4-21` as uncertified `.bst.sol`.
   - *(Note: Direct runs on physical D-Wave QPU hardware or Google OR-Tools were not executed locally because QOBLIB's `.qs` QUBO instances require $4,521\text{--}4,977$ logical binary variables, exceeding current native Pegasus/Zephyr clique embeddings without subspace reduction.)*

---

## Part III: Known Limitations & Open Questions (Questions G.29–G.30)

### 10. Known Limitations (Question G.29)
1. **Dependence on Tight Fleet Capacity ($\Delta \approx 0$):** Both `TC-Exact` and `SQA-Subspace` rely on the fact that $\sum_{i \in S_r} d_i = Q$ (or lies in a narrow band $[Q - \Delta, Q]$) to keep $|\mathcal{F}_1|$ small. For loosely constrained CVRP instances where $\sum_{i=1}^n d_i \ll KQ$, $|\mathcal{F}_1|$ grows too large for naive full enumeration.
2. **Exponential Scaling with Customer Count $n$:** Bitmask enumeration over $2^n$ states and Held-Karp TSP DP over subsets of size $|m|$ scale exponentially when $n > 30$ without column generation (pricing subproblems).
3. **Explicit Partition Graph Construction in `SQA-Subspace`:** In `generate_submission_data.cpp`, `SQA-Subspace` pre-builds the partition adjacency lists `n2` and `n1` in $O(|\mathcal{P}_4|^2)$ time (`~9 ms` mean). For larger instances where $|\mathcal{P}_4| \gg 10^5$, transitions should be sampled on-the-fly by picking 2 routes $(m_a, m_b)$ and querying a hash bucket of route pairs summing to $m_a \vee m_b$, avoiding explicit enumeration of $\mathcal{P}_4$.

### 11. Unverified Aspects & Open Research Questions (Question G.30)
1. **On-the-Fly $k$-Route Recombination Without Pre-Enumerating $\mathcal{P}_4$:** While mathematically equivalent, running `SQA-Subspace` using only $\mathcal{F}_1$ (sampling 2-route and 3-route recombinations dynamically without pre-enumerating $\mathcal{P}_4$) has not yet been benchmarked on larger $n \in [40, 100]$ instances.
2. **Gate-Model QAOA / Hardware Execution on the Reduced Subspace:** Because $|\mathcal{F}_1| \le 961 < 2^{10}$ and $|\mathcal{P}_4| \le 5,630 < 2^{13}$, the reduced problem fits in $10\text{--}13$ qubits, but actual circuit execution on IBM Heron or neutral-atom hardware remains future work.
