# Core Beliefs & Golden Principles

This document defines the opinionated, mechanically enforced operating principles of `qoblib-routing-exact-sqa`. Every human or AI agent contributing to this repository must uphold these six invariants.

---

## Principle 1: Structural Mathematics Before Brute-Force Compute
Before tuning heuristic penalty weights or scaling hardware threads, inspect the mathematical structure of the benchmark instances.
- In QOBLIB `09-routing`, proving the **Zero-Slack Tight-Capacity Invariant** ($\sum_{i=1}^{20} d_i = 4Q \implies \Delta = 0$) reduces the search space from $4^{20} \approx 1.1 \times 10^{12}$ assignments to $\lvert\mathcal{P}_4\rvert \in [372, 5,630]$ feasible partitions—turning a problem that stalls MIP/QUBO solvers for $>1$ hour into a **3.33 ms** exact calculation.

## Principle 2: Zero External C++ Dependencies ("Boring, Internalized Tech")
Solver sources ([`src/generate_submission_data.cpp`](../src/generate_submission_data.cpp) and [`src/qoblib_routing_experiments.cpp`](../src/qoblib_routing_experiments.cpp)) must compile with a single standard C++17 invocation (`clang++ -O3 -std=c++17`) using **only the C++17 Standard Library**.
- Do not introduce external JSON libraries, Boost, OpenMP, or third-party MIP/TSP solvers. Keeping the entire Held-Karp DP, bitwise Exact Cover, and Suzuki-Trotter PIMC engine self-contained in $< 650$ lines of C++17 guarantees instant portability and full agent legibility.

## Principle 3: Bit-for-Bit Deterministic Reproducibility
Stochastic algorithms (`SQA-Subspace` and `SQA-Direct`) must be **100% deterministically reproducible** across runs:
- Always seed `std::mt19937` with explicit arithmetic formulas (`1000 + run * 97 + f_idx * 13` in [`src/generate_submission_data.cpp`](../src/generate_submission_data.cpp)).
- Never use `std::random_device` or wall-clock timestamps to seed benchmark runs.

## Principle 4: Single Source of Numerical Truth (`campaign_summary.csv`)
[`campaign_summary.csv`](../campaign_summary.csv) is the canonical record of per-instance metrics.
- All summary tables in [`README.md`](../README.md) and [`docs/results.md`](results.md) must match [`campaign_summary.csv`](../campaign_summary.csv) exactly.
- [`scripts/check_harness_invariants.py`](../scripts/check_harness_invariants.py) mechanically verifies that all 55 rows in [`README.md`](../README.md) match [`campaign_summary.csv`](../campaign_summary.csv) so documentation never drifts from data.

## Principle 5: Strict Public/Private & Path Hygiene Boundary
1. **No Local Path Leaks:** Tracked files must never contain machine-specific absolute paths (`/Volumes/...`, `/Users/...`). All file paths must be relative or passed via CLI flags.
2. **Private Manuscript Isolation:** The LaTeX paper draft (`paper/`) is stored outside the repository (`../paper_local_backup/`) and must never be committed to public branches until publication.
3. **No Staged Binary Artifacts:** Compiled binaries (`build_campaign`, `run_experiments`, `check_cvrp`) and temporary `.json`/`.sol` test outputs are listed in [`.gitignore`](../.gitignore) and must not be committed.

## Principle 6: Mechanical Enforcement with Agent-Legible Remediation
Whenever a rule or invariant is important enough to document, encode it into [`scripts/check_harness_invariants.py`](../scripts/check_harness_invariants.py) with error messages that print explicit `[REMEDIATION]` instructions into the agent's context.
