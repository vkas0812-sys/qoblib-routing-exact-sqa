#!/usr/bin/env python3
"""Mechanical Harness Linter for qoblib-routing-exact-sqa.

Validates:
1. Knowledge store structure (AGENTS.md, ARCHITECTURE.md, CITATION.cff, docs/*).
2. Relative Markdown link integrity across all .md files.
3. 100% numerical synchronization between campaign_summary.csv and README.md.
4. Path & privacy hygiene (no hardcoded /Volumes/ or /Users/ paths, no tracked paper/ files).

When any check fails, prints explicit [REMEDIATION FOR AGENT] instructions.
"""
import csv
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

REQUIRED_FILES = [
    "AGENTS.md",
    "ARCHITECTURE.md",
    "README.md",
    "CITATION.cff",
    "campaign_summary.csv",
    "docs/index.md",
    "docs/core-beliefs.md",
    "docs/references.md",
    "docs/background.md",
    "docs/methods.md",
    "docs/reproducibility.md",
    "docs/results.md",
    "docs/QUALITY_SCORE.md",
    "docs/exec-plans/completed/2026-10-19-qoblib-pr93-tc-exact.md",
    "docs/exec-plans/active/roadmap-and-tech-debt.md",
    "src/generate_submission_data.cpp",
    "src/qoblib_routing_experiments.cpp",
    "scripts/build_submission.py",
    "scripts/plot_figure1.py",
    "scripts/test_reproducibility.sh",
]


def fail(msg: str, remediation: str) -> None:
    print(f"\n[HARNESS VIOLATION] {msg}", file=sys.stderr)
    print(f"[REMEDIATION FOR AGENT] {remediation}\n", file=sys.stderr)
    sys.exit(1)


def check_required_files() -> None:
    for rel in REQUIRED_FILES:
        p = REPO_ROOT / rel
        if not p.exists():
            fail(
                f"Missing required repository file: {rel}",
                f"Create or restore '{rel}' and ensure it is indexed in AGENTS.md and docs/index.md.",
            )


def check_markdown_links() -> int:
    md_files = [REPO_ROOT / "README.md", REPO_ROOT / "AGENTS.md", REPO_ROOT / "ARCHITECTURE.md"]
    md_files.extend(
        sorted(p for p in (REPO_ROOT / "docs").rglob("*.md") if not p.name.startswith("._"))
    )
    link_pattern = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    checked = 0
    for md in md_files:
        text = md.read_text(encoding="utf-8")
        # Strip fenced code blocks before scanning links
        text_no_code = re.sub(r"```.*?```", "", text, flags=re.DOTALL)
        for match in link_pattern.finditer(text_no_code):
            raw_target = match.group(1).strip()
            if (
                raw_target.startswith("http://")
                or raw_target.startswith("https://")
                or raw_target.startswith("mailto:")
                or raw_target.startswith("#")
            ):
                continue
            target_path_str = raw_target.split("#", 1)[0].strip()
            if not target_path_str:
                continue
            resolved = (md.parent / target_path_str).resolve()
            checked += 1
            if not resolved.exists():
                rel_md = md.relative_to(REPO_ROOT)
                fail(
                    f"Broken relative link in {rel_md}: '{raw_target}' -> {resolved} does not exist.",
                    f"Fix the relative link '{raw_target}' inside '{rel_md}' so it points to a valid file in the repository.",
                )
    return checked


def check_csv_readme_sync() -> int:
    csv_path = REPO_ROOT / "campaign_summary.csv"
    readme_path = REPO_ROOT / "README.md"
    with open(csv_path, encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if len(rows) != 55:
        fail(
            f"campaign_summary.csv has {len(rows)} rows (expected 55).",
            "Regenerate campaign_summary.csv using: python3 scripts/build_submission.py --results campaign_results.json",
        )

    readme_text = readme_path.read_text(encoding="utf-8")
    for r in rows:
        inst = r["Instance"]
        cap = r["Capacity"]
        subsets = r["TightSubsets"]
        parts = f"{int(r['FeasiblePartitions']):,}"
        opt = r["ProvenOptimal"]
        sec = r["SecondBest"]
        tc_ms = f"{float(r['TC_Exact_TimeMs']):.2f}"
        sqa_best = r["SQA_Subspace_Best"]
        sqa_succ = r["SQA_SuccRuns"]
        sqa_ms = f"{float(r['SQA_MeanTimeMs']):.1f}"

        expected_row = (
            f"| `{inst}` | {cap} | {subsets} | {parts} | **{opt}** | {sec} | "
            f"{tc_ms} | **{sqa_best}** | {sqa_succ} | {sqa_ms} |"
        )
        if expected_row not in readme_text:
            fail(
                f"README.md table drift detected for instance {inst}.\nExpected row:\n  {expected_row}",
                "Update the 55-instance table in README.md so every cell matches campaign_summary.csv.",
            )
    return len(rows)


def check_path_and_privacy_hygiene() -> int:
    proc = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "ls-files"],
        capture_output=True,
        text=True,
        check=True,
    )
    tracked_files = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
    forbidden_patterns = ["/Volumes/" + "VKAS_SSD", "/Users/" + "vkas"]
    for rel in tracked_files:
        if rel.startswith("paper/"):
            fail(
                f"Private manuscript file tracked in git: {rel}",
                "Remove 'paper/' from git tracking ('git rm -r --cached paper') to keep the manuscript draft private.",
            )
        p = REPO_ROOT / rel
        if p.name == "check_harness_invariants.py":
            continue
        try:
            content = p.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for pat in forbidden_patterns:
            if pat in content:
                fail(
                    f"Hardcoded local filesystem path '{pat}' found in tracked file '{rel}'.",
                    f"Remove '{pat}' from '{rel}' and use relative repository paths or CLI arguments instead.",
                )
    return len(tracked_files)


def main() -> None:
    check_required_files()
    num_links = check_markdown_links()
    num_rows = check_csv_readme_sync()
    num_tracked = check_path_and_privacy_hygiene()
    print(
        f"[HARNESS OK] Verified {len(REQUIRED_FILES)} required files, "
        f"{num_links} relative markdown links, {num_rows}/55 CSV<->README rows, "
        f"and {num_tracked} tracked files for path/privacy hygiene."
    )


if __name__ == "__main__":
    main()
