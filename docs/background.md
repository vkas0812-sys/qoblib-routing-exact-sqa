# Theoretical Background: CVRP, QUBO Penalty Barriers & Simulated Quantum Annealing

This document explains the mathematical background of QOBLIB Problem Class `09-routing`, its Mixed-Integer Programming (MIP) and Quadratic Unconstrained Binary Optimization (QUBO) formulations, the origin of Best-Known Solutions (BKS), and the physics of Simulated Quantum Annealing (covering **Questions B.7–B.9**).

---

## 1. Problem Class `09-routing` in QOBLIB (Question B.7)

### 1.1. Problem Variant & Instance Specifications
In QOBLIB, Class `09-routing` represents the **Symmetric Capacitated Vehicle Routing Problem (CVRP)** with **100% Tight Fleet Capacity**:
- **Dataset:** 55 benchmark instances named `XSH-n20-k4-01` through `XSH-n20-k4-55` (contributed by Eduardo Queiroga and Eduardo Uchoa, 2024).
- **Graph Size:** $n = 20$ customers (nodes $\{2, \dots, 21\}$ in TSPLIB 1-based indexing, shifted to $\{1, \dots, 20\}$ internally) plus $1$ central depot (node $1$ in `.vrp`, index $0$ internally), giving `DIMENSION : 21`.
- **Fleet Size:** Fixed at $K = 4$ identical vehicles starting and ending at the depot.
- **Vehicle Capacity ($Q$):** Ranges from $Q = 180$ (`XSH-n20-k4-08`, `09`) to $Q = 299$ (`XSH-n20-k4-04`, `26`).
- **Distance Metric (`EUC_2D`):** 2D Euclidean coordinates $(x_u, y_u) \in [0, 100]^2$ rounded to the nearest integer (`nint` / `std::llround`):
  $$c_{uv} = \left\lfloor \sqrt{(x_u - x_v)^2 + (y_u - y_v)^2} + 0.5 \right\rfloor, \quad \forall u, v \in \{0, 1, \dots, 20\}.$$

### 1.2. The Universal 100% Tight-Capacity Property
As documented in `QOBLIB/09-routing/instances/README.md` and verified across all 55 `.vrp` files:
$$\sum_{i=1}^{20} d_i = 4Q \implies \text{Fleet Slack } \Delta = 4Q - \sum_{i=1}^{20} d_i = 0.$$
Because no vehicle can exceed capacity $Q$ ($\sum_{i \in S_r} d_i \le Q$) and the sum across all 4 vehicles is $4Q$, **every single vehicle route $S_r$ in every feasible solution must be 100% full**:
$$\sum_{i \in S_r} d_i = Q, \quad \forall r \in \{1, 2, 3, 4\}.$$

---

## 2. Standard MIP & QUBO Formulations and Penalty Barriers (Question B.8)

### 2.1. QOBLIB Reference ILP Formulation (`cvrp_ilp.zpl`)
In `QOBLIB/09-routing/models/integer_linear/cvrp_ilp.zpl`, QOBLIB models CVRP as a two-index arc-flow Integer Linear Program with Miller-Tucker-Zemlin (MTZ) capacity-flow variables:
- **Binary arc variables:** $x_{ij} \in \{0, 1\}$ for $i, j \in \{0, \dots, n+1\}$ indicating whether a vehicle travels directly from node $i$ to node $j$.
- **Integer load variables:** $y_i \in \{0, \dots, Q\}$ tracking cumulative demand upon visiting customer $i$:
  $$y_j \ge y_i + d_j x_{ij} - Q(1 - x_{ij}), \quad d_i \le y_i \le Q.$$

### 2.2. Conversion to QUBO (`.qs` Files in QOBLIB)
To convert the constrained ILP into a **Quadratic Unconstrained Binary Optimization (QUBO)** problem:
$$\min_{z \in \{0, 1\}^N} z^T Q_{\text{qubo}} z + c^T z,$$
standard QUBO compilers (and QOBLIB's `metrics_qs_files.csv`) perform two steps:
1. **Binary Expansion of Integer Variables & Slacks:** Integer load variables $y_i \in [0, Q]$ and inequality slack variables $s$ are expanded in binary encoding ($y_i = \sum_{b=0}^{\lfloor \log_2 Q \rfloor} 2^b u_{i,b}$), inflating the number of binary decision variables in QOBLIB's `.qs` files to **$N \in [4,521,\; 4,977]$ binary variables** per instance.
2. **Quadratic Penalty Augmentation:** Every linear equality/inequality constraint $A_m z = b_m$ is squared and added to the objective with a penalty multiplier $\lambda_m \gg \max c_{ij}$:
   $$\mathcal{H}_{\text{QUBO}}(z) = \mathcal{H}_{\text{cost}}(z) + \lambda_{\text{visit}} \sum_{i=1}^{n} \left(\sum_{r=1}^{K} v_{i,r} - 1\right)^2 + \lambda_{\text{cap}} \sum_{r=1}^{K} \left(\sum_{i=1}^{n} d_i v_{i,r} - Q\right)^2 + \lambda_{\text{tour}} \mathcal{H}_{\text{subtour}}(z).$$

In QOBLIB's reference QUBO files (`09-routing/models/integer_linear/metrics_qs_files.csv`), this binary slack + penalty expansion yields extreme coefficient dynamic ranges:
- **Minimum QUBO coefficient:** $-2.98 \times 10^{10}$ (`XSH-n20-k4-34`)
- **Maximum QUBO coefficient:** $+1.83 \times 10^{10}$ (`XSH-n20-k4-34`)

### 2.3. Penalty Selection in `SQA-Direct` & Why Unconstrained QUBO/Ising Fails
In our baseline experiment `SQA-Direct` (`src/qoblib_routing_experiments.cpp`), we even eliminate the $O(n^2)$ tour-position variables by evaluating intra-route TSP tours directly via 2-opt, leaving only the $n = 20$ four-state vehicle assignment spins $v_i \in \{0, 1, 2, 3\}$ with adaptive capacity penalty $\lambda_{\text{cap}}(t) = 2.0 + 35.0 \cdot (t / T_{\text{sweeps}})$:
$$E_{\text{direct}}(v) = \sum_{r=0}^{3} \text{TSP}_{\text{2opt}}(S_r) + \lambda_{\text{cap}}(t) \sum_{r=0}^{3} \left(\sum_{i \in S_r} d_i - Q\right)^2.$$

Even in this drastically simplified representation, **zero fleet slack ($\Delta = 0$) creates an impassable penalty barrier**:
- Moving 1 customer $i$ from route $r_1$ to $r_2$ changes capacity violation by $\Delta E_{\text{pen}} = 2\lambda_{\text{cap}} d_i^2 \gg 0$.
- Swapping 2 customers $i \in S_{r_1}$ and $j \in S_{r_2}$ changes capacity violation by $\Delta E_{\text{pen}} = 2\lambda_{\text{cap}} (d_i - d_j)^2 > 0$ whenever $d_i \neq d_j$.
Because feasible configurations are isolated points separated by wide Hamming-distance barriers where $\sum_{i \in S_r} d_i \neq Q$, `SQA-Direct` finds **0/55 feasible solutions**.

---

## 3. Source of Best-Known Solutions (BKS) in QOBLIB (Question B.9)

According to `QOBLIB/09-routing/solutions/README.md`:
1. **Reference Heuristic Runs:** All 55 instances were solved using **HGS-CVRP** (Vidal, 2022) across 10 independent seeds with a 10-second time limit per run.
2. **Exact Certification:** The solutions were then verified for global optimality using **VRPSolverEasy** (Branch-Cut-and-Price by Erraid, Queiroga, Sadykov, Uchoa, & Vanderbeck, 2023).
3. **Prior Status in QOBLIB (54 `.opt.sol` + 1 `.bst.sol`):**
   - **54 instances** (`01`–`20` and `22`–`55`) were certified as optimal (`XSH-n20-k4-XX.opt.sol`).
   - **1 instance (`XSH-n20-k4-21`)** could **not** be proven optimal by the reference pipeline and was stored only as a best-known heuristic solution (`XSH-n20-k4-21.bst.sol` with `Cost = 842`). Moreover, the prior community submission `MemeticGA` (`ElYaakoubi`, July 2026) reached only `Cost = 843` on `XSH-n20-k4-21`.
   - Our `TC-Exact` solver closes this gap by proving mathematically that `Cost = 842` is indeed the global optimum of `XSH-n20-k4-21` (with `Cost = 843` being the second-best partition).

---

## 4. Simulated Quantum Annealing (SQA) via Path-Integral Monte Carlo

In **Quantum Annealing (QA)**, a problem Hamiltonian $\mathcal{H}_{\text{prob}}$ is combined with a non-commuting transverse-field driver $\mathcal{H}_{\text{driver}} = -\Gamma(t) \sum_i \sigma_i^x$:
$$\mathcal{H}(t) = \mathcal{H}_{\text{prob}} - \Gamma(t) \sum_{i} \sigma_i^x.$$
Via the **Suzuki-Trotter decomposition**, the partition function $Z = \operatorname{Tr} e^{-\mathcal{H}(t)/T}$ of the $d$-dimensional quantum system at temperature $T$ is mapped onto a $(d+1)$-dimensional classical system consisting of $P$ coupled **Trotter replicas** (slices along the imaginary-time axis) with effective action:
$$\mathcal{S}_{\text{eff}} = \sum_{p=0}^{P-1} \frac{1}{P} E_{\text{prob}}\left(\mathcal{C}^{(p)}\right) - J_{\perp}(t) \sum_{p=0}^{P-1} \mathcal{O}\left(\mathcal{C}^{(p)}, \mathcal{C}^{(p+1 \bmod P)}\right),$$
where the ferromagnetic inter-replica coupling is:
$$J_{\perp}(t) = -\frac{1}{2} T \ln \tanh\left(\frac{\Gamma(t)}{P T}\right).$$
- **High $\Gamma(t)$ (Early Annealing):** $J_{\perp}(t)$ is small; the $P$ replicas explore the manifold largely independently, mimicking quantum delocalization across energy barriers.
- **Low $\Gamma(t)$ (Late Annealing):** $J_{\perp}(t)$ grows large; adjacent replicas lock onto the lowest-action configuration via imaginary-time worldline alignment, collapsing the ensemble into the ground state.
