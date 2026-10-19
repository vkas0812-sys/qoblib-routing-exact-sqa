# QOBLIB `09-routing` (CVRP) Benchmark: Exact Optimality Certificates (`TC-Exact`) & Subspace Simulated Quantum Annealing (`SQA-Subspace`)

**Author:** Vuong Kha Anh Son (`vkas0812-sys`)  
**Affiliation:** Independent Researcher  
**Benchmark Target:** [ZIB-AOPT/QOBLIB — Class `09-routing` (Capacitated Vehicle Routing Problem)](https://github.com/ZIB-AOPT/QOBLIB/tree/main/09-routing)

---

## Abstract

The Capacitated Vehicle Routing Problem (CVRP) benchmark class (`09-routing`) in the **Quantum Optimization Benchmark Library (QOBLIB)** comprises 55 instances (`XSH-n20-k4-01` to `XSH-n20-k4-55`) with $n = 20$ customers and $K = 4$ identical vehicles of capacity $Q \in [180, 299]$. Despite the modest number of customers, standard Mixed-Integer Programming (MIP) and Quadratic Unconstrained Binary Optimization (QUBO) formulations fail to prove optimality within 1 hour on 54 of the 55 instances due to weak LP relaxations and severe quadratic penalty barriers.

In this work, we prove and exploit a universal structural invariant of the QOBLIB `09-routing` benchmark: **every single instance has 100% tight fleet capacity ($\sum_{i=1}^{20} d_i = 4Q$, fleet slack $\Delta = 0$)**. Consequently, every vehicle route in any feasible solution must satisfy the exact equality $\sum_{i \in S_r} d_i = Q$, collapsing the feasible solution space from $4^{20} \approx 1.1 \times 10^{12}$ assignments to between **$300$ and $961$ valid single-vehicle subsets** and between **$372$ and $5,630$ valid 4-vehicle partitions** per instance.

Building on this invariant, this repository provides two complementary solvers in standard C++17:

1. **`TC-Exact` (Tight-Capacity Bitwise Set-Partitioning + Held-Karp DP):** Certifies global optimality (`proven_optimal = TRUE`) across **all 55/55 instances** in a total runtime of **0.18 seconds** (**3.33 ms/instance** on Apple M4), including certifying `Cost = 842` as the unique global optimum on the hardest instance `XSH-n20-k4-21` ($\lvert\mathcal{F}_1\rvert = 458$, $\lvert\mathcal{P}_4\rvert = 1,090$, where the second-best partition has `Cost = 843`, spectral gap $\Delta E = 1$).
2. **`SQA-Subspace` (Subspace-Guided Simulated Quantum Annealing via Path-Integral Monte Carlo):** Evolves $P = 16$ Trotter replicas directly on the demand-tight partition manifold using multi-route quantum tunneling recombinations, achieving **55/55 (100%) global optimality** across 5 runs (**13.2 ms/run**, mean time-to-solution **12.77 ms**), compared to **0/55** for unconstrained penalty-based QUBO/Ising SQA (`SQA-Direct`).

---

## 1. Mathematical Formulation & Structural Invariant

### 1.1. Zero-Slack Tight-Capacity Invariant

Let $N = \{1, \dots, 20\}$ denote the set of customers with integer demands $d_i \in \mathbb{Z}_{>0}$, depot $0$, $K = 4$ vehicles each of capacity $Q \in [180, 299]$, and rounded Euclidean distance matrix:

$$c_{uv} = \left\lfloor \sqrt{(x_u - x_v)^2 + (y_u - y_v)^2} + 0.5 \right\rfloor, \quad \forall u, v \in \{0, 1, \dots, 20\}.$$

**Proposition 1 (Zero-Slack Invariant):** Across all 55 instances in `09-routing`, total customer demand equals total fleet capacity:

$$\sum_{i=1}^{20} d_i = 4Q \implies \text{Fleet Slack } \Delta = 4Q - \sum_{i=1}^{20} d_i = 0.$$

Therefore, any feasible 4-vehicle partition $\{S_1, S_2, S_3, S_4\}$ of $N$ satisfying $\sum_{i \in S_r} d_i \le Q$ must satisfy exact equality on every route:

$$\sum_{i \in S_r} d_i = Q, \quad \forall r \in \{1, 2, 3, 4\}.$$

### 1.2. Why Standard Penalty QUBO/Ising (`SQA-Direct`) Freezes

In unconstrained QUBO/Ising formulations, capacity constraints are enforced via a quadratic penalty:

$$\mathcal{H}_{\text{pen}} = \lambda_{\text{cap}} \sum_{r=1}^4 \left(\sum_{i=1}^{20} d_i x_{i,r} - Q\right)^2$$

Because $\Delta = 0$:
- Any 1-spin flip (moving customer $i$ from route $r_1$ to $r_2$) incurs an energy spike $\Delta E_{\text{pen}} = 2 \lambda_{\text{cap}} d_i^2 \gg 0$.
- Any 2-customer swap between routes $i \in S_{r_1}$ and $j \in S_{r_2}$ incurs $\Delta E_{\text{pen}} = 2 \lambda_{\text{cap}} (d_i - d_j)^2 > 0$ unless $d_i = d_j$.

This creates isolated energy wells separated by massive penalty barriers, causing standard Simulated Quantum Annealing (`SQA-Direct`) to fail on **0/55** instances.

---

## 2. Algorithms

### 2.1. `TC-Exact`: Bitwise Set-Partitioning & Held-Karp DP

1. **Tight-Subset Bitmask Enumeration:** Enumerates the exact-capacity single-vehicle subset manifold:

$$
\mathcal{F}_1 = \left\lbrace m \in \lbrace 0, \dots, 2^{20}-1 \rbrace \;\middle\vert\; \sum_{i : \text{bit}_i(m)=1} d_i = Q \right\rbrace, \quad \lvert\mathcal{F}_1\rvert \in [300, 961].
$$

2. **Held-Karp TSP Evaluation:** For each mask $m \in \mathcal{F}_1$, computes the exact minimum TSP tour cost $c^*(m)$ via bitwise Held-Karp dynamic programming in $\mathcal{O}\left(\lvert m \rvert^2 2^{\lvert m \rvert}\right)$ operations.

3. **Symmetry-Broken Bitwise Exact Cover:** Solves the 4-way partition problem:

$$\min_{m_1, m_2, m_3, m_4 \in \mathcal{F}_1} \sum_{r=1}^4 c^*(m_r) \quad \text{s.t.} \quad m_a \wedge m_b = 0 \; (a \neq b), \quad \bigvee_{r=1}^4 m_r = 2^{20}-1,$$

breaking the $4! = 24$ vehicle symmetry by anchoring the lowest set bit (`__builtin_ctz`) of the remaining unvisited customers at each stage $r \in \{1, 2, 3\}$ and querying $m_4$ in $\mathcal{O}(1)$ via direct mask lookup.

### 2.2. `SQA-Subspace`: Path-Integral Simulated Quantum Annealing on the Tight Manifold
- **Suzuki-Trotter Action ($P = 16$ Replicas):**

$$\mathcal{S}_{\text{eff}}\left(\{\mathcal{P}^{(p)}\}_{p=0}^{P-1}\right) = \sum_{p=0}^{P-1} \frac{1}{P} E_{\text{CVRP}}\left(\mathcal{P}^{(p)}\right) - J_{\perp}(t) \sum_{p=0}^{P-1} \mathcal{O}\left(\mathcal{P}^{(p)}, \mathcal{P}^{(p+1 \bmod P)}\right),$$

where the inter-replica coupling is given by:

$$J_{\perp}(t) = -\frac{1}{2} T \ln \tanh\left(\frac{\Gamma(t)}{P T}\right)$$

with transverse field $\Gamma(t)$ annealed from $35.0 \to 0.175$ over $1,000$ sweeps at $T = 6.0$.

- **Multi-Route Recombination Tunneling:** Transitions within each Trotter slice select $k \in \{2, 3, 4\}$ routes, form their union mask $U = \bigvee_{j=1}^k m_{r_j}$ (total demand $kQ$), and tunnel directly to a new $k$-tuple of disjoint tight routes in $\mathcal{F}_1$ partitioning $U$.

---

## 3. Experimental Comparison & Convergence Analysis

| Method | Paradigm | Feasible | Optimal (`55` instances) | Proven Optimal | Mean Runtime / Instance |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `SQA-Direct` (Penalty Ising, $P=16$) | Quantum-Inspired | 0 / 55 | 0 / 55 (0.0%) | No | 30.2 ms |
| `MemeticGA` (`ElYaakoubi`, QOBLIB 2026) | Classical Metaheuristic | 55 / 55 | 54 / 55 (98.2%) | No | 1,300 – 48,490 ms |
| **`SQA-Subspace` (Ours, $P=16$)** | **Quantum-Inspired** | **55 / 55** | **55 / 55 (100.0%)** | **No** | **13.2 ms (TTS: 12.77 ms)** |
| **`TC-Exact` (Ours, Bitwise Exact Cover)** | **Classical Exact** | **55 / 55** | **55 / 55 (100.0%)** | **Yes (55/55)** | **3.33 ms** |

### Recorded Convergence Trace of `SQA-Subspace` (Run #1) on Hardest Instance `XSH-n20-k4-21`
On `XSH-n20-k4-21` ($Q = 275$, $\lvert\mathcal{F}_1\rvert = 458$ tight subsets, $\lvert\mathcal{P}_4\rvert = 1,090$ valid partitions), the global optimum is `Cost = 842` and the second-best partition is `Cost = 843` (spectral gap $\Delta E = 1$). One sweep = $P = 16$ replica updates; the recorded counter is the cumulative replica-update index (sweep $= \lceil \text{update}/16 \rceil$). Subspace preprocessing (enumeration of $\mathcal{F}_1$, $\mathcal{P}_4$ and the neighbour graph) takes $3.718$ ms; times below are measured from the start of annealing:

```text
  Update    1  (Sweep  1, +0.000 ms)  -->  Incumbent Cost = 870
  Update    2  (Sweep  1, +0.000 ms)  -->  Incumbent Cost = 860
  Update   17  (Sweep  2, +0.001 ms)  -->  Incumbent Cost = 847
  Update  160  (Sweep 10, +0.006 ms)  -->  Incumbent Cost = 846
  Update  296  (Sweep 19, +0.011 ms)  -->  Incumbent Cost = 845
  Update  694  (Sweep 44, +0.025 ms)  -->  Incumbent Cost = 843
  Update 1154  (Sweep 73, +0.040 ms)  -->  Incumbent Cost = 842  (global optimum, of 1,000 sweeps)
```

---

## 4. Complete 55-Instance Benchmark Results (100% Synced from `campaign_summary.csv`)

| Instance | Capacity $Q$ | Tight Subsets $\lvert\mathcal{F}_1\rvert$ | Valid Partitions $\lvert\mathcal{P}_4\rvert$ | Proven Optimal | 2nd-Best Cost | `TC-Exact` (ms) | `SQA-Subspace` Best | `SQA` Succ Runs | `SQA` Mean (ms) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| `XSH-n20-k4-01` | 231 | 546 | 2,520 | **646** | 648 | 3.28 | **646** | 5/5 | 13.5 |
| `XSH-n20-k4-02` | 194 | 917 | 2,612 | **650** | 652 | 14.64 | **650** | 5/5 | 25.1 |
| `XSH-n20-k4-03` | 281 | 413 | 1,067 | **508** | 510 | 1.87 | **508** | 5/5 | 4.1 |
| `XSH-n20-k4-04` | 299 | 359 | 2,034 | **776** | 782 | 0.87 | **776** | 5/5 | 6.4 |
| `XSH-n20-k4-05` | 270 | 435 | 1,730 | **702** | 706 | 1.77 | **702** | 4/5 | 6.2 |
| `XSH-n20-k4-06` | 210 | 536 | 4,012 | **690** | 697 | 1.69 | **690** | 4/5 | 19.4 |
| `XSH-n20-k4-07` | 221 | 546 | 1,077 | **730** | 734 | 3.03 | **730** | 5/5 | 5.1 |
| `XSH-n20-k4-08` | 180 | 961 | 2,282 | **718** | 720 | 12.17 | **718** | 5/5 | 19.1 |
| `XSH-n20-k4-09` | 180 | 938 | 2,817 | **707** | 711 | 11.57 | **707** | 5/5 | 22.1 |
| `XSH-n20-k4-10` | 239 | 541 | 2,506 | **737** | 738 | 2.28 | **737** | 5/5 | 9.7 |
| `XSH-n20-k4-11` | 251 | 459 | 1,249 | **914** | 922 | 1.71 | **914** | 5/5 | 4.3 |
| `XSH-n20-k4-12` | 249 | 507 | 2,222 | **709** | 732 | 2.05 | **709** | 4/5 | 9.0 |
| `XSH-n20-k4-13` | 194 | 827 | 5,011 | **628** | 636 | 7.08 | **628** | 4/5 | 36.4 |
| `XSH-n20-k4-14` | 267 | 455 | 1,473 | **696** | 697 | 2.29 | **696** | 5/5 | 5.7 |
| `XSH-n20-k4-15` | 257 | 500 | 2,895 | **780** | 782 | 2.56 | **780** | 5/5 | 12.2 |
| `XSH-n20-k4-16` | 230 | 602 | 5,187 | **830** | 831 | 2.76 | **830** | 5/5 | 32.9 |
| `XSH-n20-k4-17` | 253 | 454 | 2,311 | **605** | 612 | 1.45 | **605** | 5/5 | 8.9 |
| `XSH-n20-k4-18` | 257 | 530 | 2,472 | **997** | 1003 | 2.86 | **997** | 5/5 | 10.3 |
| `XSH-n20-k4-19` | 269 | 448 | 1,675 | **976** | 987 | 1.48 | **976** | 5/5 | 5.5 |
| `XSH-n20-k4-20` | 276 | 426 | 1,668 | **648** | 650 | 1.45 | **648** | 5/5 | 5.6 |
| `XSH-n20-k4-21` | 275 | 458 | 1,090 | **842** | 843 | 2.03 | **842** | 5/5 | 4.1 |
| `XSH-n20-k4-22` | 253 | 506 | 1,834 | **760** | 769 | 2.50 | **760** | 5/5 | 6.9 |
| `XSH-n20-k4-23` | 258 | 474 | 3,137 | **851** | 860 | 1.53 | **851** | 5/5 | 13.6 |
| `XSH-n20-k4-24` | 205 | 769 | 5,630 | **725** | 739 | 4.59 | **725** | 5/5 | 39.1 |
| `XSH-n20-k4-25` | 240 | 551 | 2,864 | **462** | 463 | 2.51 | **462** | 5/5 | 13.2 |
| `XSH-n20-k4-26` | 299 | 359 | 1,308 | **688** | 692 | 1.01 | **688** | 5/5 | 4.2 |
| `XSH-n20-k4-27` | 244 | 536 | 2,279 | **540** | 542 | 2.70 | **540** | 5/5 | 9.6 |
| `XSH-n20-k4-28` | 211 | 716 | 2,506 | **669** | 675 | 5.54 | **669** | 5/5 | 14.1 |
| `XSH-n20-k4-29` | 229 | 555 | 2,334 | **658** | 661 | 2.91 | **658** | 5/5 | 10.9 |
| `XSH-n20-k4-30` | 262 | 413 | 2,158 | **685** | 688 | 1.08 | **685** | 5/5 | 7.3 |
| `XSH-n20-k4-31` | 259 | 492 | 3,371 | **445** | 448 | 2.04 | **445** | 4/5 | 16.8 |
| `XSH-n20-k4-32` | 197 | 828 | 3,008 | **882** | 884 | 6.35 | **882** | 5/5 | 17.7 |
| `XSH-n20-k4-33` | 250 | 491 | 2,413 | **568** | 569 | 1.97 | **568** | 5/5 | 10.1 |
| `XSH-n20-k4-34` | 270 | 478 | 372 | **891** | 893 | 1.81 | **891** | 5/5 | 2.4 |
| `XSH-n20-k4-35` | 284 | 418 | 1,776 | **707** | 713 | 1.46 | **707** | 5/5 | 5.9 |
| `XSH-n20-k4-36` | 226 | 556 | 3,722 | **610** | 614 | 2.46 | **610** | 5/5 | 19.5 |
| `XSH-n20-k4-37` | 245 | 475 | 2,800 | **650** | 655 | 1.53 | **650** | 5/5 | 11.3 |
| `XSH-n20-k4-38` | 234 | 552 | 3,513 | **991** | 993 | 2.94 | **991** | 4/5 | 18.5 |
| `XSH-n20-k4-39` | 234 | 595 | 2,885 | **717** | 718 | 3.45 | **717** | 5/5 | 14.6 |
| `XSH-n20-k4-40` | 234 | 586 | 2,624 | **700** | 732 | 3.06 | **700** | 5/5 | 19.4 |
| `XSH-n20-k4-41` | 245 | 538 | 2,069 | **736** | 739 | 3.20 | **736** | 5/5 | 9.1 |
| `XSH-n20-k4-42` | 232 | 597 | 1,978 | **998** | 1005 | 3.16 | **998** | 5/5 | 8.9 |
| `XSH-n20-k4-43` | 240 | 625 | 2,279 | **465** | 472 | 4.72 | **465** | 5/5 | 13.2 |
| `XSH-n20-k4-44` | 235 | 544 | 2,283 | **973** | 979 | 2.46 | **973** | 5/5 | 10.1 |
| `XSH-n20-k4-45` | 279 | 355 | 3,406 | **1071** | 1078 | 0.68 | **1071** | 5/5 | 14.5 |
| `XSH-n20-k4-46` | 210 | 824 | 4,197 | **812** | 814 | 6.53 | **812** | 2/5 | 27.9 |
| `XSH-n20-k4-47` | 266 | 471 | 1,174 | **583** | 585 | 1.95 | **583** | 5/5 | 4.4 |
| `XSH-n20-k4-48` | 220 | 676 | 2,192 | **544** | 545 | 5.14 | **544** | 5/5 | 11.8 |
| `XSH-n20-k4-49` | 233 | 524 | 2,871 | **1045** | 1049 | 1.93 | **1045** | 5/5 | 12.2 |
| `XSH-n20-k4-50` | 245 | 540 | 2,105 | **627** | 629 | 2.59 | **627** | 5/5 | 8.2 |
| `XSH-n20-k4-51` | 228 | 639 | 2,940 | **491** | 494 | 5.83 | **491** | 5/5 | 15.4 |
| `XSH-n20-k4-52` | 210 | 713 | 3,112 | **622** | 625 | 5.70 | **622** | 5/5 | 18.5 |
| `XSH-n20-k4-53` | 297 | 300 | 1,554 | **967** | 972 | 0.53 | **967** | 5/5 | 3.8 |
| `XSH-n20-k4-54` | 198 | 749 | 4,850 | **791** | 797 | 5.39 | **791** | 5/5 | 41.8 |
| `XSH-n20-k4-55` | 289 | 306 | 1,386 | **617** | 618 | 0.79 | **617** | 5/5 | 3.8 |
| **All 55 Instances** | **180 – 299** | **300 – 961** | **372 – 5,630** | **55 / 55 Optimal** | — | **3.33 ms (mean)** | **55 / 55 Optimal** | **266 / 275 (96.7%)** | **13.2 ms (mean)** |

---

## 5. Reproducing the Results & Building the QOBLIB Submission

### Prerequisites
- A C++17 compiler (`clang++` or `g++`) and `python3`.
- A checkout of [ZIB-AOPT/QOBLIB](https://github.com/ZIB-AOPT/QOBLIB) next to this repository (default layout: `../QOBLIB`).

### Compile and Run

```bash
git clone https://github.com/ZIB-AOPT/QOBLIB.git ../QOBLIB   # if not already present

# 1. Compile and execute the C++17 solver (TC-Exact + SQA-Subspace across all 55 instances)
#    args: [instances dir] [output JSON]  (defaults shown)
clang++ -O3 -std=c++17 src/generate_submission_data.cpp -o build_campaign
./build_campaign ../QOBLIB/09-routing/instances campaign_results.json

# 2. Package the QOBLIB-compliant submission directory (defaults to TC-Exact for PR #93)
python3 scripts/build_submission.py --results campaign_results.json \
    --out ../QOBLIB/09-routing/submissions/20261017_TCExact_Vuong

# 3. Validate with the official QOBLIB checker
(cd ../QOBLIB && python3 misc/ci/check_submission.py 09-routing/submissions/20261017_TCExact_Vuong)
```

All random seeds are fixed, so objective values, success counts and traces are reproduced exactly; only wall-clock timings vary with hardware.

---

## AI Disclosure

In accordance with the [QOBLIB AI Policy](https://github.com/ZIB-AOPT/QOBLIB/blob/main/AI_POLICY.md), this repository and submission are declared as `generated`: solver code (`TC-Exact` and `SQA-Subspace`), benchmarking scripts, and documentation were predominantly generated by the Antigravity AI coding assistant under the direction, experimental execution, and verification of the author (**Vuong Kha Anh Son**) against the official QOBLIB verification suite (`check_submission.py` and `check_cvrp`).
