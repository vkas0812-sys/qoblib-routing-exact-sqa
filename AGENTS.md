# AGENTS.md — Repository Map & Operating Guide

> **Principle:** This file is the **table of contents** (~85 lines), not an encyclopedia. Treat [`docs/`](docs/index.md) as the versioned system of record and follow the links below via progressive disclosure.

## 1. Mission & Scope
This repository (`qoblib-routing-exact-sqa`) contains two C++17 solvers for **QOBLIB Class `09-routing`** (55 tight-capacity CVRP instances `XSH-n20-k4-01` .. `55`):
1. **`TC-Exact`:** Deterministic bitwise Set-Partitioning + Held-Karp DP solver certifying global optimality on **55/55 instances** in **3.33 ms/instance** (submitted in [ZIB-AOPT/QOBLIB PR #93](https://github.com/ZIB-AOPT/QOBLIB/pull/93)).
2. **`SQA-Subspace`:** Path-Integral Monte Carlo Simulated Quantum Annealing ($P=16$ Trotter slices) evolving directly on the zero-slack partition manifold $\mathcal{P}_4$, reaching **55/55 (100%) BKS** in **13.2 ms/run**.

---

## 2. Knowledge Base Navigation (`docs/` System of Record)

| Topic / Question Domain | Primary Source of Truth | Summary of Contents |
| :--- | :--- | :--- |
| **System Architecture & Layers** | [`ARCHITECTURE.md`](ARCHITECTURE.md) | 5-layer pipeline, dependency rules, data schemas, and boundary invariants |
| **Catalog & Verification Status** | [`docs/index.md`](docs/index.md) | Master index of all documentation, verification status, and 32-question FAQ map |
| **Golden Principles & Invariants** | [`docs/core-beliefs.md`](docs/core-beliefs.md) | Non-negotiable mathematical, reproducibility, dependency, and privacy rules |
| **A. Sources, Papers & Citation (Q1–Q6, Q31–Q32)** | [`docs/references.md`](docs/references.md) | Full bibliography (QOBLIB, Queiroga & Uchoa 2024, Uchoa 2017, Held-Karp, PIMC-SQA), PR #93, private `paper/` history, BibTeX |
| **B. Problem, QUBO & BKS (Q7–Q9)** | [`docs/background.md`](docs/background.md) | `09-routing` specs, MIP/QUBO formulations, penalty barrier proof under $\Delta=0$, BKS origins, Suzuki-Trotter physics |
| **C & D. `TC-Exact` & `SQA-Subspace` (Q10–Q18, Q29–Q30)** | [`docs/methods.md`](docs/methods.md) | Exhaustive optimality certificate proof, scaling/memory limits, PR #93 standalone mode, SQA hyperparameters, limitations |
| **E. Build, Seeds & Scripts (Q19–Q24)** | [`docs/reproducibility.md`](docs/reproducibility.md) | Compiler/OS requirements, ordered CLI commands, deterministic `std::mt19937` seed formulas, `.cpp` & `.py` script guide |
| **F. CSV Columns & Figure 1 (Q25–Q28)** | [`docs/results.md`](docs/results.md) | 12-column dictionary for [`campaign_summary.csv`](campaign_summary.csv), Figure 1 (`fig1_sqa_analysis.pdf`) analysis, `XSH-n20-k4-21` closure |
| **Quality Grades & Layer Health** | [`docs/QUALITY_SCORE.md`](docs/QUALITY_SCORE.md) | Quality scorecard across solver code, verification coverage, and documentation |
| **Execution Plans & Decision Log** | [`docs/exec-plans/`](docs/exec-plans/active/roadmap-and-tech-debt.md) | Completed PR #93 decision history and active research/engineering roadmap |

---

## 3. Golden Rules for Agents Working in This Repository
1. **Zero External C++ Dependencies:** Keep [`src/generate_submission_data.cpp`](src/generate_submission_data.cpp) and [`src/qoblib_routing_experiments.cpp`](src/qoblib_routing_experiments.cpp) pure standard C++17 STL (`clang++ -O3 -std=c++17`).
2. **Never Hardcode Local Paths:** Always accept instance/output paths via CLI arguments (`argv[1]`, `argv[2]`). Never commit `/Volumes/...` or `/Users/...` strings.
3. **Keep `paper/` Private:** The LaTeX manuscript draft is stored locally outside git (`../paper_local_backup/`). Never re-add `paper/` to public branches.
4. **Preserve Deterministic Seeds:** Do not alter the fixed `std::mt19937` seed formulas unless explicitly instructed; [`campaign_summary.csv`](campaign_summary.csv) and [`README.md`](README.md) must remain 100% synchronized.
5. **Default `build_submission.py` to `tc-exact`:** Per QOBLIB maintainer review on PR #93, each submission directory must represent a single standalone method (`--mode tc-exact` by default).

---

## 4. Mechanical Verification Commands (Run Before Opening/Updating Any PR)

```bash
# 1. Fast Doc-Gardening & Structural Invariant Linter (< 0.2s)
python3 scripts/check_harness_invariants.py

# 2. Full End-to-End Compile + 55-Instance Solve + Official QOBLIB Check (~1.5s)
bash scripts/test_reproducibility.sh
```
