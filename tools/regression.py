#!/usr/bin/env python3
"""Run the whole testcase suite (or some of it) and report what passes and what does not.

Every testcase is its own binary: `make testcase TC=NNN` relinks ./testcase with that one
testcase, so the suite is a loop of build + run. Doing that loop by hand is where results get
misread, so this tool keeps to the same rules tools/abprobe.py learned the hard way:

  * the testcase source is touched before each build, or make can call ./testcase "up to date"
    and run the PREVIOUS testcase's binary under the new number;
  * every run has a hard timeout, and a run killed by it is TIMEOUT, never a pass;
  * a crash (killed by a signal) is CRASH, not FAIL, because it usually means something else;
  * a run that ends without a "Test ..." verdict line is NO VERDICT, never a pass;
  * always -nointro: the intro otherwise eats the early check() ticks.

The tester's own exit code is inverted (exit 1 on pass), so the verdict is read from the
output, not from the exit status.

Only files named exactly src/tests/testcase_NNN.cpp are testcases.

Usage:
  python3 tools/regression.py [TC ...] [--timeout SECONDS]

  TC         testcase numbers to run, e.g. 041 064 (default: all of them)
  --timeout  per-testcase limit on the RUN, default 120

Each run's full output goes to tmp/regression/testcase_NNN.log (tmp/ is wiped by make clean,
which is fine: the logs belong to one run).

Exit status:
  0  every testcase passed
  1  at least one did not
"""
import argparse
import os
import re
import signal
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(ROOT, "src", "tests")
LOGS = os.path.join(ROOT, "tmp", "regression")


def touch(path):
    # Same as abprobe.py: push the mtime forward rather than trusting it moved.
    now = time.time() + 1
    os.utime(path, (now, now))


def all_testcases():
    names = [f for f in os.listdir(TESTS) if re.fullmatch(r"testcase_\d{3}\.cpp", f)]
    return sorted(n[len("testcase_"):-len(".cpp")] for n in names)


def run_one(tc, timeout):
    """Returns (status, detail, seconds)."""
    src = os.path.join(TESTS, "testcase_%s.cpp" % tc)
    touch(src)

    build = subprocess.run(["make", "testcase", "TC=%s" % tc], cwd=ROOT,
                           capture_output=True, text=True)
    if build.returncode != 0:
        errors = [l.strip() for l in (build.stdout + build.stderr).splitlines() if " error" in l]
        return "BUILD", (errors[0] if errors else "make failed"), 0.0

    start = time.time()
    log = os.path.join(LOGS, "testcase_%s.log" % tc)
    with open(log, "w") as out:
        try:
            proc = subprocess.run(["./testcase", "-d", "-nointro"], cwd=ROOT, stdin=subprocess.DEVNULL,
                                  stdout=out, stderr=subprocess.STDOUT, timeout=timeout)
            code = proc.returncode
        except subprocess.TimeoutExpired:
            return "TIMEOUT", "no verdict after %ds" % timeout, time.time() - start
    elapsed = time.time() - start

    text = open(log, errors="replace").read()
    verdicts = [l for l in text.splitlines() if l.startswith("Test ")]

    if code < 0:
        try:
            name = signal.Signals(-code).name
        except ValueError:
            name = "signal %d" % -code
        return "CRASH", name, elapsed
    if not verdicts:
        return "NO VERDICT", "exit %d" % code, elapsed
    line = verdicts[-1]
    if line.startswith("Test Passed"):
        return "PASS", "", elapsed
    return "FAIL", line, elapsed


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("tc", nargs="*")
    ap.add_argument("--timeout", type=int, default=120)
    args = ap.parse_args(argv)

    known = all_testcases()
    wanted = [t.zfill(3) for t in args.tc] or known
    missing = [t for t in wanted if t not in known]
    if missing:
        print("No such testcase: %s" % " ".join(missing))
        return 1

    os.makedirs(LOGS, exist_ok=True)

    results = []
    for tc in wanted:
        status, detail, secs = run_one(tc, args.timeout)
        results.append((tc, status, detail))
        print("%s  %-10s %5.1fs  %s" % (tc, status, secs, detail), flush=True)

    bad = [r for r in results if r[1] != "PASS"]
    print()
    print("%d testcases: %d passed, %d not." % (len(results), len(results) - len(bad), len(bad)))
    for tc, status, detail in bad:
        print("  %s  %-10s %s" % (tc, status, detail))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
