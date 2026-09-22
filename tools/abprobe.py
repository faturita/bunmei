#!/usr/bin/env python3
"""Run one A/B verification of a testcase: break the code on purpose, prove the test notices.

A testcase that passes only tells you the code works TODAY. What it does not tell you is
whether the test would notice if the code stopped working -- and a test that cannot notice is
worse than no test, because it reads like coverage. The A/B is the second half: disable the
thing under test, confirm the testcase fails, put it back.

Doing that by hand is where the mistakes live, and this tool exists because every one of these
actually happened while writing the command-migration testcases:

  * the probe silently did not match (a shell helper dropped its arguments), so nothing was
    modified and FOUR "A/B verified" results in a row were meaningless;
  * the probed .cpp was edited but only the testcase was touched, so make considered the
    object up to date and the run used the PREVIOUS probe's binary, reporting the wrong
    failure message;
  * the probe was applied and the test still passed -- which is the one result that matters --
    and it was easy to read past in a batch of output;
  * the source was restored but the BINARY was not rebuilt, so the next run started from a
    poisoned ./testcase and reported the previous probe's answer.

So: it asserts the probe matched, touches what it changed, always restores, and treats
"Test Passed" as the FAILURE of the verification (exit 1), because it means the code you
disabled was not load-bearing and the assertion you trusted is blind.

Usage:
  python3 tools/abprobe.py <TC> <file> <old> <new> [--label TEXT]

  TC     testcase number, e.g. 074
  file   the source file to probe, e.g. src/units/Scout.cpp
  old    exact text to replace (must appear; first occurrence is used)
  new    what to replace it with -- "" to delete the line(s)

Example:
  python3 tools/abprobe.py 074 src/units/Scout.cpp \\
      "    visionRange = 2;" "    visionRange = 1;" --label "Scout radius back to 1"

Exit status:
  0  the testcase FAILED or crashed with the probe applied -- the code is load-bearing (good)
  1  the testcase still PASSED -- the probe was not load-bearing, or the assertion is blind
  2  the probe did not match, or the build failed -- no A/B was performed
"""
import argparse
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)


def touch(path):
    # The build keys off mtimes, and a probe applied within the same timestamp tick as the
    # last build leaves the object looking current -- which silently runs the previous
    # binary. Push the mtime forward rather than trusting write() to have moved it.
    now = time.time() + 1
    os.utime(os.path.join(ROOT, path), (now, now))


def main(argv):
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("tc")
    ap.add_argument("file")
    ap.add_argument("old")
    ap.add_argument("new")
    ap.add_argument("--label", default=None, help="what this probe is testing, for the report")
    args = ap.parse_args(argv)

    label = args.label or ("%s: %.40s" % (args.file, args.old.strip()))
    path = os.path.join(ROOT, args.file)
    testcase_src = os.path.join("src", "tests", "testcase_%s.cpp" % args.tc)

    if not os.path.exists(path):
        print("%-44s NO A/B: %s does not exist" % (label, args.file))
        return 2
    if not os.path.exists(os.path.join(ROOT, testcase_src)):
        print("%-44s NO A/B: %s does not exist" % (label, testcase_src))
        return 2

    original = open(path).read()

    if args.old not in original:
        # The failure that produces fake green results. Never continue past it.
        print("%-44s NO A/B: probe text not found in %s" % (label, args.file))
        print("   looked for: %r" % (args.old[:120],))
        return 2

    open(path, "w").write(original.replace(args.old, args.new, 1))

    try:
        touch(args.file)
        touch(testcase_src)          # force the relink too, not just the probed object

        build = run(["make", "testcase", "TC=%s" % args.tc])
        if build.returncode != 0:
            errors = [l for l in (build.stdout + build.stderr).splitlines() if " error" in l]
            print("%-44s NO A/B: build failed with the probe applied" % label)
            for line in errors[:3]:
                print("   %s" % line.strip())
            return 2

        # Always -nointro: the intro otherwise eats the early check() ticks and buries output.
        proc = run(["./testcase", "-d", "-nointro"])
        out = proc.stdout + proc.stderr
        verdict = [l for l in out.splitlines() if l.startswith("Test ")]

        if proc.returncode < 0 or proc.returncode == 139:
            # A crash is a legitimate A/B signal: the guard was preventing one.
            print("%-44s OK (crashed with the probe applied -- the guard prevents it)" % label)
            return 0
        if not verdict:
            print("%-44s NO A/B: the testcase produced no verdict (exit %d)" % (label, proc.returncode))
            return 2

        line = verdict[-1]
        if line.startswith("Test Passed"):
            print("%-44s NOT LOAD-BEARING: the testcase still passed" % label)
            return 1

        print("%-44s OK -- %s" % (label, line))
        return 0

    finally:
        open(path, "w").write(original)
        touch(args.file)

        # Rebuild before leaving. Restoring the SOURCE is not enough: ./testcase on disk still
        # holds the probe until something relinks it, so the next thing to run it -- another
        # probe, or a person checking by hand -- would silently get the broken binary and a
        # result belonging to the previous experiment. Each invocation leaves the tree
        # consistent with what is on disk.
        touch(testcase_src)
        run(["make", "testcase", "TC=%s" % args.tc])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
