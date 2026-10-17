"""Regenerate paper/figures/fig1_sqa_analysis.pdf from campaign_results.json.

Usage: python3 scripts/plot_figure1.py [--results campaign_results.json]
"""
import argparse
import json
import math
import os
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/mpl_cache")
import matplotlib.pyplot as plt

P = 16          # Trotter replicas: one sweep = P replica updates
SWEEPS = 1000   # sweeps per SQA run

parser = argparse.ArgumentParser()
parser.add_argument("--results", default="campaign_results.json")
parser.add_argument("--out", default="fig1_sqa_analysis.pdf")
args = parser.parse_args()

with open(args.results, encoding="utf-8") as f:
    data = json.load(f)
inst21 = next(d for d in data if d["instance"] == "XSH-n20-k4-21")

plt.rcParams.update({"font.size": 9, "font.family": "serif"})
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10.5, 3.6), dpi=300)

colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd", "#8c564b"]
for idx, trace in enumerate(inst21["traces"]):
    # The solver records the cumulative replica-update index; convert to sweeps.
    sweeps = [max(1, math.ceil(pt["step"] / P)) for pt in trace]
    objs = [pt["Incumbent"] for pt in trace]
    sweeps.append(SWEEPS)
    objs.append(objs[-1])
    ax1.step(sweeps, objs, where="post", lw=1.7, alpha=0.88, color=colors[idx],
             label=f"SQA Run #{idx+1} (opt. at sweep {sweeps[-2]})")

ax1.axhline(843, color="#7f7f7f", ls="-.", lw=1.2, label="2nd-best partition = MemeticGA best (843)")
ax1.axhline(842, color="#000000", ls="-", lw=1.8, label="TC-Exact proven optimum (842)")
ax1.set_xscale("log")
ax1.set_xlim(1, SWEEPS)
ax1.set_xlabel(f"Monte Carlo sweep (log scale, 1 sweep = {P} replica updates)")
ax1.set_ylabel("Objective Value (Total Tour Distance)")
ax1.set_title("(a) SQA-Subspace Tunneling on XSH-n20-k4-21")
ax1.set_ylim(839, 885)
ax1.grid(True, which="both", ls=":", alpha=0.5)
ax1.legend(loc="upper right", fontsize=6.9, framealpha=0.92)

ids = list(range(1, len(data) + 1))
ax2.bar(ids, [d["num_partitions"] for d in data], color="#aec7e8", edgecolor="#1f77b4", lw=0.5,
        label="Valid 4-Vehicle Partitions |P_4|")
ax2.plot(ids, [d["num_subsets"] for d in data], color="#d62728", marker="o", ms=2.8, lw=1.2,
         label="Tight 1-Vehicle Subsets |F_1|")
ax2.set_xlabel("QOBLIB Instance Index (XSH-n20-k4-01 to 55)")
ax2.set_ylabel("Manifold Cardinality (Count)")
ax2.set_title("(b) Zero-Slack Manifold Collapse Across All 55 Instances")
ax2.set_xlim(0, len(data) + 1)
ax2.grid(True, ls=":", alpha=0.5)
ax2.legend(loc="upper right", fontsize=7.8, framealpha=0.92)

plt.tight_layout()
Path(args.out).parent.mkdir(parents=True, exist_ok=True)
plt.savefig(args.out, bbox_inches="tight")
print("wrote", args.out)
