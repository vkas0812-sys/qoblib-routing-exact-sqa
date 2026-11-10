# Repository Quality Scorecard (`QUALITY_SCORE.md`)

This document grades each architectural layer and documentation domain of `qoblib-routing-exact-sqa`, tracking verification coverage and known gaps over time.

---

## 1. Layer & Domain Grades

| Layer / Domain | Grade | Automated Verification | Notes & Known Gaps |
| :--- | :---: | :--- | :--- |
| **`TC-Exact` Solver (`src/generate_submission_data.cpp`)** | **A+** | `scripts/test_reproducibility.sh` + `check_cvrp` | 55/55 proven optimal in `0.183 s` total (`3.33 ms/instance`), `< 5 MB` RAM, zero external dependencies. |
| **`SQA-Subspace` Solver (`src/generate_submission_data.cpp`)** | **A** | `scripts/test_reproducibility.sh` | 55/55 BKS (`266/275 = 96.7%` single-run success) in `13.2 ms/run`. Pre-builds $O(\lvert\mathcal{P}_4\rvert^2)$ neighbor lists (fast for $n=20$, needs on-the-fly sampling for $n > 30$). |
| **QOBLIB Submission Packager (`scripts/build_submission.py`)** | **A+** | `QOBLIB/misc/ci/check_submission.py` | Passes 55/55 instance checks + `check_bkv_updates.py` in `--mode tc-exact` (default for PR #93). |
| **Data Synchronization (`campaign_summary.csv` $\leftrightarrow$ `README.md`)** | **A+** | `scripts/check_harness_invariants.py` | All 55 rows and 10 displayed metrics per instance are mechanically compared on every check run. |
| **Knowledge Store (`AGENTS.md`, `ARCHITECTURE.md`, `docs/`)** | **A+** | `scripts/check_harness_invariants.py` | All 32 audit questions (A–G) answered in structured docs; all relative markdown links verified mechanically. |
| **Path & Privacy Hygiene** | **A+** | `scripts/check_harness_invariants.py` | Zero hardcoded `/Volumes/` or `/Users/` paths; private `paper/` directory excluded from git tracking. |
