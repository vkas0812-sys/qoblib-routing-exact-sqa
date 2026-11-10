# Knowledge Store Index (`docs/`)

Welcome to the in-repository knowledge store for **`qoblib-routing-exact-sqa`**. Following **Harness Engineering** principles, all domain knowledge, mathematical proofs, parameter justifications, design history, and verification procedures live inside versioned Markdown files in this directory and are mechanically validated by [`scripts/check_harness_invariants.py`](../scripts/check_harness_invariants.py).

---

## 1. Core Documentation Suite

| Document | Status | Scope & Questions Answered |
| :--- | :---: | :--- |
| **[`docs/core-beliefs.md`](core-beliefs.md)** | Verified | 6 Golden Principles governing solver design, determinism, dependencies, and privacy |
| **[`docs/references.md`](references.md)** | Verified | **Questions A.1–A.6, G.31–G.32:** Bibliography, `XSH-n20-k4` provenance, QOBLIB & PR #93 links, private `paper/` history, BibTeX citation |
| **[`docs/background.md`](background.md)** | Verified | **Questions B.7–B.9:** CVRP `09-routing` specification, MIP/QUBO formulations, quadratic penalty barriers under $\Delta=0$, BKS sources, PIMC-SQA physics |
| **[`docs/methods.md`](methods.md)** | Verified | **Questions C.10–C.14, D.15–D.18, G.29–G.30:** `TC-Exact` optimality proof & scaling, PR #93 standalone packaging, `SQA-Subspace` hyperparameters & tunneling, limitations |
| **[`docs/reproducibility.md`](reproducibility.md)** | Verified | **Questions E.19–E.24:** Environment requirements, step-by-step build/run commands, `test_reproducibility.sh`, fixed `std::mt19937` seeds, script architecture |
| **[`docs/results.md`](results.md)** | Verified | **Questions F.25–F.28:** Column-by-column guide to [`campaign_summary.csv`](../campaign_summary.csv), Figure 1 (`fig1_sqa_analysis.pdf`) analysis, BKS comparison, Apple M4 timings |
| **[`docs/QUALITY_SCORE.md`](QUALITY_SCORE.md)** | Verified | Layer-by-layer quality grades, test coverage, and invariant status |

---

## 2. Execution Plans & Decision Logs (`docs/exec-plans/`)

| Plan / Log | State | Summary |
| :--- | :---: | :--- |
| **[`completed/2026-10-19-qoblib-pr93-tc-exact.md`](exec-plans/completed/2026-10-19-qoblib-pr93-tc-exact.md)** | Completed | Full history of solver implementation, removal of `paper/` from public `main`, and splitting `TC-Exact` for QOBLIB PR #93 per maintainer feedback |
| **[`active/roadmap-and-tech-debt.md`](exec-plans/active/roadmap-and-tech-debt.md)** | Active | Next steps: follow-up PR for `SQA-Subspace`, dynamic on-the-fly $k$-route sampling for $n > 30$, and QOBLIB `02-labs` subspace extension |

---

## 3. Quick Mapping: 32 Audit Questions (A–G) to Exact Locations

- **A. Sources & Literature (Q1–Q6):** [`docs/references.md` (§1–§3)](references.md)
- **B. Problem & QUBO Formulation (Q7–Q9):** [`docs/background.md` (§1–§3)](background.md)
- **C. `TC-Exact` Solver (Q10–Q14):** [`docs/methods.md` (Part I, §1–§5)](methods.md)
- **D. `SQA-Subspace` Solver (Q15–Q18):** [`docs/methods.md` (Part II, §6–§9)](methods.md)
- **E. Code & Reproducibility (Q19–Q24):** [`docs/reproducibility.md` (§1–§6)](reproducibility.md)
- **F. Experimental Results & Figure 1 (Q25–Q28):** [`docs/results.md` (§1–§4)](results.md)
- **G. Limitations, Lineage & Citation (Q29–Q32):** [`docs/methods.md` (Part III, §10–§11)](methods.md) & [`docs/references.md` (§4)](references.md)
