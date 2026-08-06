#!/usr/bin/env python3
"""Configure, build, and run the host test suite."""

import argparse
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent
TEST_BUILD = ROOT / "firmware" / "build" / "host-tests"


def run(command: list[str]) -> None:
    print(f"$ (cd {ROOT} && {' '.join(command)})", flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build and run host tests with coverage and sanitizers."
    )
    parser.add_argument("layer", nargs="?", choices=("all", "unit"), default="all")
    args = parser.parse_args()

    configure = [
        "cmake",
        "-S",
        "tests",
        "-B",
        str(TEST_BUILD),
        f"-DPER_TEST_LAYER={args.layer}",
        "-DPER_TEST_SANITIZERS=ON",
        "-DPER_TEST_COVERAGE=ON",
    ]
    run(configure)
    run(["cmake", "--build", str(TEST_BUILD)])

    run(["cmake", "--build", str(TEST_BUILD), "--target", "coverage"])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
