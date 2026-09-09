#!/usr/bin/env python3
"""Own the relationship between README.md's tech table and the game's tech data.

README.md's "Science" section is the SOURCE for the technology graph: one row per
technology (name, code, and its dependencies with a weight factor). Three places in
the code have to agree with it -- src/codes.h's TECH_* defines, and
src/technologies.cpp's DEFAULT_TECHS[] and DEFAULT_DEPS[] arrays -- and all three are
easy to get subtly wrong by hand (118 dependency rows, each with a weight).

So: generate them from the README instead of transcribing them, and keep a `check`
mode that fails loudly when the doc and the code drift apart.

Usage:
  python3 tools/techtable.py check              # do README, codes.h and technologies.cpp agree?
  python3 tools/techtable.py emit codes         # the codes.h TECH_* block
  python3 tools/techtable.py emit techs         # the DEFAULT_TECHS[] rows
  python3 tools/techtable.py emit deps          # the DEFAULT_DEPS[] rows
  python3 tools/techtable.py costs              # depth / bias / SCIENCE cost per technology

`emit` prints to stdout for pasting into the corresponding file -- deliberately not
written in place, since both files are hand-maintained around those arrays. `check`
is the guard that makes that safe.

`costs` is the balancing view. Firing a technology needs

    science >= (logit(TECH_FIRING_THRESHOLD) + TECH_BIAS_BASE ^ depth) / weight

and the three constants are read out of src/technologies.h, so the numbers printed are
always the ones the game is actually running.

Exit status: 0 on success, 1 if `check` finds a mismatch.
"""
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
README = ROOT / "README.md"
CODES = ROOT / "src" / "codes.h"
TECHNOLOGIES_CPP = ROOT / "src" / "technologies.cpp"
TECHNOLOGIES_H = ROOT / "src" / "technologies.h"

# "| Language | `0x01` | - |" / "| Archery | `0x08` | Language (0.8), Hunting (1.0) |"
ROW = re.compile(r"^\|\s*([A-Za-z][A-Za-z ]*?)\s*\|\s*`(0x[0-9A-Fa-f]+)`\s*\|\s*(.*?)\s*\|\s*$")
DEP = re.compile(r"^(.*?)\s*\(([0-9.]+)\)$")


def macro(name):
    """'Warrior Code' -> 'TECH_WARRIOR_CODE' (the codes.h / DEFAULT_* identifier)."""
    return "TECH_" + name.upper().replace(" ", "_")


def read_readme():
    """[(name, code, [(parent_name, factor), ...]), ...] in table order.

    Only rows inside the "## Science" section are taken -- README.md has other tables
    (governments, for one) that would otherwise match the same row shape.
    """
    techs, in_science = [], False
    for line in README.read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            in_science = line.strip() == "## Science"
            continue
        if not in_science:
            continue
        m = ROW.match(line)
        if not m:
            continue
        name, code, deps = m.group(1), int(m.group(2), 16), m.group(3)
        if name == "Tech":          # the header row
            continue
        parsed = []
        if deps.strip() != "-":
            for part in deps.split(","):
                d = DEP.match(part.strip())
                if not d:
                    sys.exit("README.md: dependency '%s' of '%s' has no (weight)" % (part.strip(), name))
                parsed.append((d.group(1).strip(), float(d.group(2))))
        techs.append((name, code, parsed))

    if not techs:
        sys.exit("README.md: no technology rows found under '## Science'")

    known = {n for n, _, _ in techs}
    for name, _, deps in techs:
        for parent, _ in deps:
            if parent not in known:
                sys.exit("README.md: '%s' depends on unknown technology '%s'" % (name, parent))
    return techs


def depths(techs):
    """name -> longest path in hops from the root, matching TechGraph::computeBiases()."""
    by_name = {n: deps for n, _, deps in techs}
    depth = {n: 0 for n, _, deps in techs if not deps}
    for _ in range(len(techs) + 1):
        changed = False
        for name, deps in by_name.items():
            if not deps:
                continue
            if any(p not in depth for p, _ in deps):
                continue
            d = 1 + max(depth[p] for p, _ in deps)
            if depth.get(name) != d:
                depth[name] = d
                changed = True
        if not changed:
            break
    missing = [n for n, _, _ in techs if n not in depth]
    if missing:
        sys.exit("README.md: unreachable technologies (a dependency cycle?): %s" % ", ".join(missing))
    return depth


def constants():
    """The three tuning constants, read from src/technologies.h."""
    src = TECHNOLOGIES_H.read_text(encoding="utf-8")
    out = {}
    for key in ("TECH_DEFAULT_WEIGHT", "TECH_BIAS_BASE", "TECH_FIRING_THRESHOLD"):
        m = re.search(r"^const float %s\s*=\s*([0-9.]+)f;" % key, src, re.M)
        if not m:
            sys.exit("src/technologies.h: could not find %s" % key)
        out[key] = float(m.group(1))
    return out


# ---- emit ------------------------------------------------------------------------

def emit_codes(techs):
    width = max(len(macro(n)) for n, _, _ in techs)
    return "\n".join("#define %-*s 0x%02x" % (width, macro(n), c) for n, c, _ in techs)


def emit_techs(techs):
    mw = max(len(macro(n)) for n, _, _ in techs)
    nw = max(len(n) for n, _, _ in techs)
    rows = ('    { %-*s "%s"%s },' % (mw + 1, macro(n) + ",", n, " " * (nw - len(n)))
            for n, _, _ in techs)
    return "\n".join(rows).rstrip(",")      # no trailing comma, same as emit_deps


def emit_deps(techs):
    lines = []
    for name, _, deps in techs:
        entries = ["{ %s, %s, %.1ff }" % (macro(p), macro(name), w) for p, w in deps]
        for i in range(0, len(entries), 2):          # two per source line, as in the file
            lines.append("    " + ", ".join(entries[i:i + 2]) + ",")
    return "\n".join(lines).rstrip(",")


# ---- check -----------------------------------------------------------------------

def array_body(src, decl):
    """The text between '<decl> = {' and the matching '};'."""
    i = src.index(decl)
    start = src.index("{", i + len(decl))
    end = src.index("\n};", start)
    return src[start:end]


def check(techs):
    problems = []

    codes_src = CODES.read_text(encoding="utf-8")
    defined = {m.group(1): int(m.group(2), 16)
               for m in re.finditer(r"^#define\s+(TECH_[A-Z_0-9]+)\s+(0x[0-9a-fA-F]+)", codes_src, re.M)}
    # TECH_FIRST/LAST/ROOT/COUNT are derived aliases, not technologies.
    for alias in ("TECH_FIRST", "TECH_LAST", "TECH_ROOT", "TECH_COUNT"):
        defined.pop(alias, None)

    expected_codes = {macro(n): c for n, c, _ in techs}
    for name, code in sorted(expected_codes.items()):
        if name not in defined:
            problems.append("codes.h is missing %s (README says 0x%02x)" % (name, code))
        elif defined[name] != code:
            problems.append("codes.h has %s = 0x%02x, README says 0x%02x" % (name, defined[name], code))
    for name in sorted(set(defined) - set(expected_codes)):
        problems.append("codes.h defines %s, which README.md's table does not list" % name)

    cpp = TECHNOLOGIES_CPP.read_text(encoding="utf-8")

    got_techs = re.findall(r"\{\s*(TECH_[A-Z_0-9]+)\s*,\s*\"([^\"]*)\"\s*\}",
                           array_body(cpp, "DEFAULT_TECHS[]"))
    want_techs = [(macro(n), n) for n, _, _ in techs]
    if got_techs != want_techs:
        problems.append("DEFAULT_TECHS[] does not match README.md (%d rows vs %d); "
                        "regenerate with `emit techs`" % (len(got_techs), len(want_techs)))

    got_deps = [(a, b, float(w)) for a, b, w in
                re.findall(r"\{\s*(TECH_[A-Z_0-9]+)\s*,\s*(TECH_[A-Z_0-9]+)\s*,\s*([0-9.]+)f\s*\}",
                           array_body(cpp, "DEFAULT_DEPS[]"))]
    want_deps = [(macro(p), macro(n), w) for n, _, deps in techs for p, w in deps]
    if got_deps != want_deps:
        problems.append("DEFAULT_DEPS[] does not match README.md (%d edges vs %d); "
                        "regenerate with `emit deps`" % (len(got_deps), len(want_deps)))
        for i, (g, e) in enumerate(zip(got_deps, want_deps)):
            if g != e:
                problems.append("  first difference at edge %d: code has %s, README says %s" % (i, g, e))
                break

    if problems:
        for p in problems:
            print("MISMATCH: " + p)
        return 1

    print("OK: README.md, codes.h and technologies.cpp agree "
          "(%d technologies, %d dependencies)." % (len(techs), len(want_deps)))
    return 0


# ---- costs -----------------------------------------------------------------------

def costs(techs):
    k = constants()
    w0, base, thr = k["TECH_DEFAULT_WEIGHT"], k["TECH_BIAS_BASE"], k["TECH_FIRING_THRESHOLD"]
    logit = math.log(thr / (1.0 - thr))
    depth = depths(techs)

    print("TECH_DEFAULT_WEIGHT %.4f   TECH_BIAS_BASE %.2f   TECH_FIRING_THRESHOLD %.2f"
          % (w0, base, thr))
    print("cost = ceil((logit(threshold) + TECH_BIAS_BASE^depth) / weight), "
          "taking the cheapest parent\n")
    print("%5s %10s %12s  %s" % ("depth", "bias", "cost", "technology"))

    total = 0
    for name, _, deps in techs:
        d = depth[name]
        bias = base ** d
        if not deps:
            print("%5d %10.0f %12s  %s (root)" % (d, bias, "-", name))
            continue
        cost = min(math.ceil((logit + bias) / (f * w0)) for _, f in deps)
        total += cost
        print("%5d %10.0f %12d  %s" % (d, bias, cost, name))

    print("\nTotal SCIENCE to research the whole tree (cheapest parent each): %d" % total)
    print("Deepest technology: %d hops." % max(depth.values()))
    return 0


def main(argv):
    techs = read_readme()

    if len(argv) < 2:
        print(__doc__)
        return 1

    cmd = argv[1]
    if cmd == "check":
        return check(techs)
    if cmd == "costs":
        return costs(techs)
    if cmd == "emit":
        what = argv[2] if len(argv) > 2 else ""
        if what == "codes":
            print(emit_codes(techs))
            return 0
        if what == "techs":
            print(emit_techs(techs))
            return 0
        if what == "deps":
            print(emit_deps(techs))
            return 0
        print("emit what? one of: codes, techs, deps")
        return 1

    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
