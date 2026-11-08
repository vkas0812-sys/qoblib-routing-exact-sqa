# Reproducibility & Execution Guide

This document provides step-by-step instructions to compile, execute, and independently verify all results in this repository (covering **Questions E.19–E.24**).

---

## 1. System & Environment Requirements (Question E.19)

| Component | Requirement | Tested Version (Reference Hardware) |
| :--- | :--- | :--- |
| **Operating System** | macOS or Linux (POSIX compliant) | macOS (Apple Silicon M4, 16 GB RAM) |
| **C++ Compiler** | Any C++17 compliant compiler (`clang++` or `g++`) supporting `<filesystem>` and `__builtin_ctz` / `__builtin_popcount` | `AppleClang 21.0.0` (`-O3 -std=c++17`) |
| **External C++ Libraries** | **None** (100% standard C++17 STL only) | Standard Library (`libc++` / `libstdc++`) |
| **Python** | Python `>= 3.8` | Python `3.11+` |
| **Python Packages** | Standard library only (`json`, `csv`, `pathlib`, `argparse`) for submission packaging; `matplotlib` only if regenerating Figure 1 (`plot_figure1.py`) | `matplotlib >= 3.7` (optional) |
| **Rust / Cargo (Optional)** | Only required if compiling QOBLIB's native Rust solution verifier `09-routing/check` from source | `cargo 1.75+` |

---

## 2. Step-by-Step Build & Run Commands (Question E.20)

Assume this repository (`qoblib-routing-exact-sqa`) and the official [`ZIB-AOPT/QOBLIB`](https://github.com/ZIB-AOPT/QOBLIB) repository are cloned side-by-side:

```bash
# 0. Clone repositories side-by-side (if not already present)
git clone https://github.com/ZIB-AOPT/QOBLIB.git ../QOBLIB
git clone https://github.com/vkas0812-sys/qoblib-routing-exact-sqa.git
cd qoblib-routing-exact-sqa

# 1. Compile the main campaign generator (TC-Exact + 5-seed SQA-Subspace)
clang++ -O3 -std=c++17 src/generate_submission_data.cpp -o build_campaign

# 2. Run across all 55 QOBLIB 09-routing instances and output JSON metrics
./build_campaign ../QOBLIB/09-routing/instances campaign_results.json

# 3. Package the QOBLIB submission directory (standalone TC-Exact for PR #93)
python3 scripts/build_submission.py \
    --results campaign_results.json \
    --mode tc-exact \
    --out ../QOBLIB/09-routing/submissions/20261017_TCExact_Vuong \
    --csv campaign_summary.csv

# 4. Validate the generated submission with the official QOBLIB CI checker
(cd ../QOBLIB && python3 misc/ci/check_submission.py 09-routing/submissions/20261017_TCExact_Vuong)

# 5. (Optional) Compile & run the 3-way comparison table (SQA-Direct vs SQA-Subspace vs TC-Exact)
clang++ -O3 -std=c++17 src/qoblib_routing_experiments.cpp -o run_experiments
./run_experiments ../QOBLIB/09-routing/instances ../QOBLIB/09-routing/solutions

# 6. (Optional) Regenerate Figure 1 (fig1_sqa_analysis.pdf)
python3 scripts/plot_figure1.py --results campaign_results.json --out fig1_sqa_analysis.pdf
```

---

## 3. Automated End-to-End Test: `scripts/test_reproducibility.sh` (Question E.21)

### 3.1. What `test_reproducibility.sh` Checks
Running `bash scripts/test_reproducibility.sh` performs a completely isolated 4-step verification inside a temporary directory (`/tmp/qoblib_test_XXXXXX`) that is automatically cleaned up on exit:
1. **Step 1/4 (Compilation):** Compiles `src/generate_submission_data.cpp` from scratch using `clang++ -O3 -std=c++17`.
2. **Step 2/4 (Execution):** Solves all 55 `.vrp` instances from `../QOBLIB/09-routing/instances` and writes `$TMP_DIR/results.json`.
3. **Step 3/4 (Packaging):** Runs `scripts/build_submission.py --mode tc-exact` to build all 55 instance folders (`XSH-n20-k4-01/` .. `55/`), 55 `.sol` files, 55 instance `README.md` files, and the top-level `README.md` + `solutions_summary.csv`.
4. **Step 4/4 (Official QOBLIB Verification):** Invokes QOBLIB's official checker `misc/ci/check_submission.py` (which in turn invokes the compiled Rust binary `09-routing/check/target/release/check_cvrp`) to verify every `.sol` route, capacity constraint, customer coverage, and objective value against the `.vrp` files.

### 3.2. Expected Output When Passing
```text
=================================================================
   TC-Exact Independent Reproducibility & Validation Suite       
=================================================================
Repo Root:   /path/to/qoblib-routing-exact-sqa
QOBLIB Root: /path/to/QOBLIB

[Step 1/4] Compiling TC-Exact solver from source...
           -> Compilation successful.

[Step 2/4] Solving all 55 instances...
           -> Solved 55/55 instances to global optimality.

[Step 3/4] Packaging submission directory (TC-Exact standalone)...
           -> Generated 55 instance directories + campaign summaries.

[Step 4/4] Validating with official QOBLIB submission checker...
  XSH-n20-k4-50: OK
  XSH-n20-k4-51: OK
  XSH-n20-k4-52: OK
  XSH-n20-k4-53: OK
  XSH-n20-k4-54: OK
  XSH-n20-k4-55: OK

Summary: 55/55 instances passed, 0 failed. Overall: OK

=================================================================
   ALL TESTS PASSED: 100% Deterministic Reproducibility Confirmed! 
=================================================================
```

---

## 4. Random Seeds & Deterministic Reproducibility (Question E.22)

All random number generators use the C++17 standard Mersenne Twister (`std::mt19937`) with **explicitly hardcoded deterministic seeds**:
1. **In `src/generate_submission_data.cpp` (line 286):**
   ```cpp
   std::mt19937 rng(1000 + run * 97 + f_idx * 13);
   ```
   where `f_idx` $\in \{0, \dots, 54\}$ is the lexicographically sorted instance index (`XSH-n20-k4-01` to `55`) and `run` $\in \{0, 1, 2, 3, 4\}$ is the SQA run index.
2. **In `src/qoblib_routing_experiments.cpp` (lines 478 & 576):**
   - `run_tight_subspace_and_sqa`: fixed seed `42` (`std::mt19937 rng(seed);`).
   - `run_sqa_direct`: 3 deterministic seeds `s * 1337` for `s = 1, 2, 3`.
3. **Exact Reproducibility Guarantee:**
   - **`TC-Exact`:** 100% deterministic algorithm (no randomness). Optimal costs (`exact_cost`), second-best costs (`second_best_cost`), subset counts (`num_subsets`), partition counts (`num_partitions`), and optimal `.sol` tours are **100% identical across all platforms and compilers**.
   - **`SQA-Subspace`:** Because `std::mt19937` is fully specified by the C++17 standard and `std::generate_canonical<double, 10>(rng)` is used for Metropolis acceptance, all trajectory steps (`step`), incumbent values (`Incumbent`), best costs (`sqa_best_cost`), and success counts (`266/275` runs, `55/55` instances) are **100% deterministically reproducible**. Only wall-clock timings (`*_s`, `*_ms`) vary slightly with CPU clock speed.

---

## 5. Comparison of the Two C++ Source Files (Question E.23)

| Aspect | `src/generate_submission_data.cpp` | `src/qoblib_routing_experiments.cpp` |
| :--- | :--- | :--- |
| **Primary Purpose** | Official QOBLIB submission & campaign JSON generator | Interactive console benchmark comparing 3 paradigms (`SQA-Direct` vs. `SQA-Subspace` vs. `TC-Exact`) |
| **Solvers Included** | `TC-Exact` + `SQA-Subspace` (5 runs $\times$ 1,000 sweeps) | `SQA-Direct` (3 runs $\times$ 2,000 sweeps) + `SQA-Subspace` (1 run $\times$ 600 sweeps) + `TC-Exact` |
| **Outputs** | Structured JSON (`campaign_results.json`) with optimal 4-vehicle `.sol` routes, model nonzeros, and step-by-step SQA traces | Formatted ASCII comparison table printed to `stdout` comparing against QOBLIB reference `.opt.sol`/`.bst.sol` |
| **Used by `build_submission.py`?** | **Yes** (produces the exact input JSON for `build_submission.py` and `plot_figure1.py`) | No (standalone comparative experiment script) |

---

## 6. What `scripts/build_submission.py` Generates (Question E.24)

`scripts/build_submission.py` reads `campaign_results.json` and builds a QOBLIB-compliant submission tree following [`QOBLIB/CONTRIBUTING.md`](https://github.com/ZIB-AOPT/QOBLIB/blob/main/CONTRIBUTING.md):
- **Mode 1 (`--mode tc-exact`, default — used in QOBLIB PR #93):**
  Generates `20261017_TCExact_Vuong/` with **167 files**:
  - Top-level `README.md` (overview, methodology, full 55-instance markdown table, AI disclosure).
  - Top-level `solutions_summary.csv` (summary CSV for the submission).
  - `campaign_summary.csv` in the repository root.
  - 55 instance directories `XSH-n20-k4-01/` .. `XSH-n20-k4-55/`, each containing:
    1. `README.md` — 36-field QOBLIB metadata table with `Value 1` reporting `TC-Exact` (`Best Objective Value == Optimality Bound`, `# Decision Variables = |F_1|`, hardware, runtime, and link to this repo).
    2. `XSH-n20-k4-XX.sol` — CVRPLIB-formatted solution file:
       ```text
       Route #1: 1 5 12 15 19
       Route #2: 4 7 9 11 16
       Route #3: 2 3 8 10 14 18
       Route #4: 6 13 17 20
       Cost 646
       ```
       *(Note: Customer indices in `.sol` are 1-based customer IDs `1..20`, matching QOBLIB's `check_cvrp` convention.)*
- **Mode 2 (`--mode both`):**
  Generates a two-column (`Value 1` = `TC-Exact`, `Value 2` = `SQA-Subspace`) submission tree that additionally writes `XSH-n20-k4-XX/traces/XSH-n20-k4-XX_trace.csv` containing the `Time,Incumbent` convergence trace of `SQA-Subspace`.
