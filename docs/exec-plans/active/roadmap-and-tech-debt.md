# Active Execution Plan: Roadmap & Technical Debt Tracker

- **Status:** Active
- **Last Updated:** November 10, 2026

---

## 1. Active Milestones

### Milestone 1: Merge QOBLIB PR #93 (`20261017_TCExact_Vuong`)
- **Status:** Open on [`ZIB-AOPT/QOBLIB#93`](https://github.com/ZIB-AOPT/QOBLIB/pull/93), passing local `check_submission.py` (`55/55 OK`) and `check_bkv_updates.py`.
- **Next Action:** Await maintainer workflow approval and merge on `ZIB-AOPT/QOBLIB`; once merged, optionally open a second standalone PR for `SQA-Subspace` (`20261017_SQASubspace_Vuong`) if desired.

### Milestone 2: On-the-Fly $k$-Route Recombination for Large-$n$ CVRP
- **Current Technical Debt:** In [`src/generate_submission_data.cpp`](../../../src/generate_submission_data.cpp), `SQA-Subspace` constructs explicit neighbor lists `n2` and `n1` over all pairs in $\mathcal{P}_4$ in $O(\lvert\mathcal{P}_4\rvert^2)$ time (`~9 ms` for $n=20$).
- **Planned Upgrade:** For instances with $n > 30$ where $\lvert\mathcal{P}_K\rvert$ is too large to pre-enumerate, sample 2-route recombinations dynamically by indexing pairs $(m_a, m_b) \in \mathcal{F}_1 \times \mathcal{F}_1$ by their disjoint union mask $U = m_a \vee m_b$ in a hash table.

### Milestone 3: Extending Subspace Reduction to QOBLIB `02-labs`
- **Context:** In QOBLIB `02-labs` (Low Autocorrelation Binary Sequences), 34 instances (`labs067` to `labs100`) remain unproven.
- **Planned Upgrade:** Apply skew-symmetry and modular energy residue congruence filters to restrict PIMC-SQA and branch-and-bound to high-merit invariant subspaces.
