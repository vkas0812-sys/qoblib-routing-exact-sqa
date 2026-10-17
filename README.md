# Closing the QOBLIB `09-routing` (CVRP) Benchmark: Exact Optimality Certificates (`TC-Exact`) & Subspace Simulated Quantum Annealing (`SQA-Subspace`)

**Author:** Vuong Kha Anh Son (`vkas0812-sys`)  
**Affiliation:** Independent Researcher  
**Benchmark Target:** [ZIB-AOPT/QOBLIB — Class `09-routing` (Capacitated Vehicle Routing Problem)](https://github.com/ZIB-AOPT/QOBLIB/tree/main/09-routing)

---

## Abstract

The Capacitated Vehicle Routing Problem (CVRP) benchmark class (`09-routing`) in the **Quantum Optimization Benchmark Library (QOBLIB)** comprises 55 instances (`XSH-n20-k4-01` to `XSH-n20-k4-55`) with $n = 20$ customers and $K = 4$ identical vehicles. Despite the modest number of customers, standard Mixed-Integer Programming (MIP) and Quadratic Unconstrained Binary Optimization (QUBO) formulations fail to prove optimality within 1 hour on 54 of the 55 instances due to weak LP relaxations and severe quadratic penalty barriers.

In this work, we prove and exploit a universal structural invariant of the QOBLIB `09-routing` benchmark: **every single instance has 100% tight fleet capacity ($\sum_{i=1}^{20} d_i = 4Q$, fleet slack $\Delta = 0$)**. Consequently, every vehicle route in any feasible solution must satisfy the exact equality $\sum_{i \in S_r} d_i = Q$, collapsing the feasible solution space from $4^{20} \approx 1.1 \times 10^{12}$ assignments to between **$300$ and $961$ valid single-vehicle subsets** and between **$372$ and $5,630$ valid 4-vehicle partitions** per instance.

Building on this invariant, this repository provides two complementary solvers in standard C++17:
1. **`TC-Exact` (Tight-Capacity Bitwise Set-Partitioning + Held-Karp DP):** Certifies global optimality (`proven_optimal = TRUE`) across **all 55/55 instances** in a total runtime of **0.21 seconds** (**3.83 ms/instance** on Apple M4), including certifying `Cost = 842` as the unique global optimum on the hardest instance `XSH-n20-k4-21` (where the second-best partition has `Cost = 843`, spectral gap $\Delta E = 1$).
2. **`SQA-Subspace` (Subspace-Guided Simulated Quantum Annealing via Path-Integral Monte Carlo):** Evolves $P = 16$ Trotter replicas directly on the demand-tight partition manifold using multi-route quantum tunneling recombinations, achieving **55/55 (100%) global optimality** across 5 runs of 1,000 sweeps (**22.4 ms/run**), compared to **0/55** for unconstrained penalty-based QUBO/Ising SQA (`SQA-Direct`).

---

## 1. Mathematical Formulation & Structural Invariant

### 1.1. Zero-Slack Tight-Capacity Invariant
Let $N = \{1, \dots, 20\}$ denote the set of customers with integer demands $d_i \in \mathbb{Z}_{>0}$, depot $0$, $K = 4$ vehicles each of capacity $Q \in \{35, 1000\}$, and rounded Euclidean distance matrix:
$$c_{uv} = \left\lfloor \sqrt{(x_u - x_v)^2 + (y_u - y_v)^2} + 0.5 \right\rfloor, \quad \forall u, v \in \{0, 1, \dots, 20\}.$$

**Proposition 1 (Zero-Slack Invariant):** Across all 55 instances in `09-routing`, total customer demand equals total fleet capacity:
$$\sum_{i=1}^{20} d_i = 4Q \implies \text{Fleet Slack } \Delta = 4Q - \sum_{i=1}^{20} d_i = 0.$$
Therefore, any feasible 4-vehicle partition $\{S_1, S_2, S_3, S_4\}$ of $N$ satisfying $\sum_{i \in S_r} d_i \le Q$ must satisfy exact equality on every route:
$$\sum_{i \in S_r} d_i = Q, \quad \forall r \in \{1, 2, 3, 4\}.$$

### 1.2. Why Standard Penalty QUBO/Ising (`SQA-Direct`) Freezes
In unconstrained QUBO/Ising formulations, capacity constraints are enforced via a quadratic penalty $\mathcal{H}_{\text{pen}} = \lambda_{\text{cap}} \sum_{r=1}^4 (\sum_{i=1}^{20} d_i x_{i,r} - Q)^2$. Because $\Delta = 0$:
* Any 1-spin flip (moving customer $i$ from route $r_1$ to $r_2$) incurs an energy spike $\Delta E_{\text{pen}} = 2 \lambda_{\text{cap}} d_i^2 \gg 0$.
* Any 2-customer swap between routes $i \in S_{r_1}$ and $j \in S_{r_2}$ incurs $\Delta E_{\text{pen}} = 2 \lambda_{\text{cap}} (d_i - d_j)^2 > 0$ unless $d_i = d_j$.
This creates isolated energy wells separated by massive penalty barriers, causing standard Simulated Quantum Annealing (`SQA-Direct`) to fail on **0/55** instances.

---

## 2. Algorithms

### 2.1. `TC-Exact`: Bitwise Set-Partitioning & Held-Karp DP
1. **Tight-Subset Bitmask Enumeration:** Enumerates the exact-capacity single-vehicle subset manifold:
   $$\mathcal{F}_1 = \left\{ m \in \{0, \dots, 2^{20}-1\} \;\middle|\; \sum_{i : \text{bit}_i(m)=1} d_i = Q \right\}, \quad |\mathcal{F}_1| \in [300, 961].$$
2. **Held-Karp TSP Evaluation:** For each mask $m \in \mathcal{F}_1$, computes the exact minimum TSP tour cost $c^*(m)$ via bitwise Held-Karp dynamic programming in $O(|m|^2 2^{|m|})$ operations.
3. **Symmetry-Broken Bitwise Exact Cover:** Solves the 4-way partition problem:
   $$\min_{m_1, m_2, m_3, m_4 \in \mathcal{F}_1} \sum_{r=1}^4 c^*(m_r) \quad \text{s.t.} \quad m_a \wedge m_b = 0 \; (a \neq b), \quad \bigvee_{r=1}^4 m_r = 2^{20}-1,$$
   breaking the $4! = 24$ vehicle symmetry by anchoring the lowest set bit (`__builtin_ctz`) of the remaining unvisited customers at each stage $r \in \{1, 2, 3\}$ and querying $m_4$ in $O(1)$ via direct mask lookup.

### 2.2. `SQA-Subspace`: Path-Integral Simulated Quantum Annealing on the Tight Manifold
* **Suzuki-Trotter Action ($P = 16$ Replicas):**
  $$\mathcal{S}_{\text{eff}}\left(\{\mathcal{P}^{(p)}\}_{p=0}^{P-1}\right) = \sum_{p=0}^{P-1} \frac{1}{P} E_{\text{CVRP}}\left(\mathcal{P}^{(p)}\right) - J_{\perp}(t) \sum_{p=0}^{P-1} \mathcal{O}\left(\mathcal{P}^{(p)}, \mathcal{P}^{(p+1 \bmod P)}\right),$$
  where $J_{\perp}(t) = -\frac{1}{2} T \ln \tanh\left(\frac{\Gamma(t)}{P T}\right)$ with transverse field $\Gamma(t)$ annealed from $35.0 \to 0.175$ over $1,000$ sweeps at $T = 2.5$.
* **Multi-Route Recombination Tunneling:** Transitions within each Trotter slice select $k \in \{2, 3, 4\}$ routes, form their union mask $U = \bigvee_{j=1}^k m_{r_j}$ (total demand $kQ$), and tunnel directly to a new $k$-tuple of disjoint tight routes in $\mathcal{F}_1$ partitioning $U$.

---

## 3. Experimental Comparison & Convergence Analysis

| Method | Paradigm | Feasible | Optimal (`55` instances) | Proven Optimal | Mean Runtime / Instance |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `SQA-Direct` (Penalty Ising, $P=16$) | Quantum-Inspired | 0 / 55 | 0 / 55 (0.0%) | No | 30.2 ms |
| `MemeticGA` (`ElYaakoubi`, QOBLIB 2026) | Classical Metaheuristic | 55 / 55 | 54 / 55 (98.2%) | No | 1,300 – 48,490 ms |
| **`SQA-Subspace` (Ours, $P=16$, 1000 sweeps)** | **Quantum-Inspired** | **55 / 55** | **55 / 55 (100.0%)** | **No** | **22.4 ms** |
| **`TC-Exact` (Ours, Bitwise Exact Cover)** | **Classical Exact** | **55 / 55** | **55 / 55 (100.0%)** | **Yes (55/55)** | **3.83 ms** |

### Convergence Trajectory of `SQA-Subspace` on Hardest Instance `XSH-n20-k4-21`
On `XSH-n20-k4-21` ($|\mathcal{F}_1| = 609$ tight subsets, $|\mathcal{P}_4| = 1,701$ valid partitions), the global optimum is `Cost = 842` and the second-best partition is `Cost = 843` (spectral gap $\Delta E = 1$). Below is the recorded incumbent convergence across sweeps:

```text
Objective Cost
  940 | * (Sweep 1: Cost = 935, t = 0.03 ms)
  920 | |
  900 | +---* (Sweep 2: Cost = 890, t = 0.05 ms)
  880 |     |
  860 |     +-----* (Sweep 7: Cost = 865, t = 0.16 ms)
  850 |           +-------* (Sweep 19: Cost = 853 [Paper Baseline], t = 0.42 ms)
  843 |                   +---------* (Sweep 26: Cost = 843 [2nd-Best Trap], t = 0.58 ms)
  842 |                             +===================* (Quantum Tunneling to Global Optimum = 842!)
      +-------------------------------------------------------------------------
        1         5       10        20        30       100      500     1000 Sweeps
```

---

## 4. Complete 55-Instance Benchmark Table

| Instance | Capacity $Q$ | Tight Subsets $|\mathcal{F}_1|$ | Valid Partitions $|\mathcal{P}_4|$ | Proven Optimal | 2nd-Best Cost | `TC-Exact` (ms) | `SQA-Subspace` Best | `SQA-Subspace` Mean (ms) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| `XSH-n20-k4-01` | 35 | 661 | 1,622 | **646** | 656 | 2.91 | **646** | 18.8 |
| `XSH-n20-k4-02` | 35 | 909 | 5,117 | **595** | 602 | 6.13 | **595** | 32.1 |
| `XSH-n20-k4-03` | 35 | 871 | 3,907 | **641** | 645 | 5.29 | **641** | 28.4 |
| `XSH-n20-k4-04` | 35 | 783 | 2,520 | **736** | 762 | 4.02 | **736** | 22.9 |
| `XSH-n20-k4-05` | 35 | 809 | 2,906 | **634** | 637 | 4.35 | **634** | 24.5 |
| `XSH-n20-k4-21` | 1000 | 609 | 1,701 | **842** | 843 | 2.69 | **842** | 17.5 |
| `XSH-n20-k4-24` | 1000 | 961 | 5,630 | **826** | 841 | 6.84 | **826** | 34.8 |
| `XSH-n20-k4-34` | 1000 | 300 | 372 | **765** | 773 | 1.12 | **765** | 9.4 |
| `XSH-n20-k4-55` | 1000 | 557 | 1,354 | **697** | 709 | 2.38 | **697** | 15.9 |
| **All 55 Instances** | — | **300 – 961** | **372 – 5,630** | **55 / 55 Optimal** | — | **3.83 ms (mean)** | **55 / 55 Optimal** | **22.4 ms (mean)** |

*(Full per-instance metrics for all 55 instances are provided in [`campaign_summary.csv`](campaign_summary.csv)).*

---

## 5. Reproducing the Results & Building the QOBLIB Submission

### Prerequisites
* A C++17 compiler (`clang++` or `g++`) and `python3`.

### Compile and Run
```bash
# 1. Compile and execute the C++17 solver (TC-Exact + SQA-Subspace across all 55 instances)
clang++ -O3 -std=c++17 src/generate_submission_data.cpp -o build_campaign
./build_campaign

# 2. Package the QOBLIB-compliant submission directory
python3 scripts/build_submission.py
```

---

## AI Disclosure
In accordance with the [QOBLIB AI Policy](https://github.com/ZIB-AOPT/QOBLIB/blob/main/AI_POLICY.md), this repository and submission are declared as `generated`: solver code (`TC-Exact` and `SQA-Subspace`), benchmarking scripts, and documentation were predominantly generated by the Antigravity AI coding assistant under the direction, experimental execution, and verification of the author (**Vuong Kha Anh Son**) against the official QOBLIB verification suite (`check_submission.py` and `check_cvrp`).
