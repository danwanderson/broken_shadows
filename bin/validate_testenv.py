#!/usr/bin/env python3
"""
Quick test to validate dependencies and basic connectivity.

This script checks if all required tools are installed and optionally
performs a quick connectivity test.

Usage:
    python3 validate_testenv.py [--quick-test]
"""

import sys
import subprocess
import shutil
import importlib
from pathlib import Path
from typing import Optional


def check_executable(name: str, hint: Optional[str] = None) -> bool:
    """Check if an executable is available in PATH."""
    if shutil.which(name):
        print(f"✓ {name:20} found")
        return True
    else:
        msg = f"✗ {name:20} NOT FOUND"
        if hint:
            msg += f"\n  Hint: {hint}"
        print(msg)
        return False


def check_python_module(name: str, import_name: Optional[str] = None) -> bool:
    """Check if a Python module is available."""
    mod_name = import_name or name
    try:
        importlib.import_module(mod_name)
        print(f"✓ Python {name:15} available")
        return True
    except ImportError:
        print(f"✗ Python {name:15} NOT FOUND - pip install {name}")
        return False


def check_file(path: str, description: str = "") -> bool:
    """Check if a file exists."""
    if Path(path).exists():
        desc = f" ({description})" if description else ""
        print(f"✓ {path}{desc}")
        return True
    else:
        print(f"✗ {path} - NOT FOUND")
        return False


def check_docker() -> bool:
    """Check Docker installation and basic functionality."""
    print("\n=== Docker ===")
    if not check_executable("docker"):
        return False

    # Try to run docker ps
    try:
        result = subprocess.run(
            ["docker", "ps"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if result.returncode == 0:
            print("✓ Docker daemon is running")
            return True
        else:
            print("✗ Docker daemon not accessible")
            print("  Hint: Try 'docker ps' to troubleshoot")
            return False
    except Exception as e:
        print(f"✗ Docker check failed: {e}")
        return False


def check_python() -> bool:
    """Check Python version and modules."""
    print("\n=== Python ===")
    # Check Python version
    version = sys.version_info
    if version.major >= 3 and version.minor >= 6:
        print(f"✓ Python {version.major}.{version.minor}.{version.micro}")
    else:
        print(f"✗ Python {version.major}.{version.minor} - Need 3.6+")
        return False

    # Check required modules
    check_python_module("telnetlib3", "telnetlib3")
    check_python_module("asyncio", "asyncio")
    check_python_module("subprocess", "subprocess")
    check_python_module("pathlib", "pathlib")
    check_python_module("argparse", "argparse")

    return True


def check_repository() -> bool:
    """Check repository structure."""
    print("\n=== Repository ===")

    files = {
        "docker-compose.yml": "Docker Compose config",
        "Dockerfile": "Docker image definition",
        "build_image.sh": "Build script",
        "bin/test_mud_automated.py": "Test suite",
    }

    all_good = True
    for fname, desc in files.items():
        if not check_file(fname, desc):
            all_good = False

    dirs = {
        "area": "World data",
        "src": "Source code",
        "player": "Player files",
        "doc": "Documentation",
    }

    for dname, desc in dirs.items():
        path = Path(dname)
        if path.is_dir():
            print(f"✓ {dname:20} ({desc})")
        else:
            print(f"✗ {dname:20} ({desc}) - NOT FOUND")
            all_good = False

    return all_good


def quick_connectivity_test() -> bool:
    """Quick test: Start container and verify connectivity."""
    print("\n=== Quick Connectivity Test ===")
    import time
    import asyncio
    import telnetlib3

    async def test_connection():
        """Test Telnet connection asynchronously."""
        try:
            _, writer = await asyncio.wait_for(
                telnetlib3.open_connection(
                    "localhost", 4000
                ),
                timeout=5,
            )
            writer.close()
            await writer.wait_closed()
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    print("Starting container (background)...")
    try:
        # Start container
        result = subprocess.run(
            ["docker-compose", "up", "-d"],
            capture_output=True,
            text=True,
            timeout=30,
        )

        if result.returncode != 0:
            print(f"✗ Failed to start container: {result.stderr}")
            return False

        print("✓ Container started")
        time.sleep(3)  # Give server time to start

        # Test connectivity
        print("Testing Telnet connection to localhost:4000...")
        try:
            success = asyncio.run(test_connection())
            if success:
                print("✓ Server is responding")

                # Stop container
                print("Stopping container...")
                subprocess.run(
                    ["docker-compose", "down"],
                    capture_output=True,
                    timeout=15,
                )
                print("✓ Container stopped")

                return True
            else:
                print("✗ Connection test failed")
                # Try to stop anyway
                subprocess.run(
                    ["docker-compose", "down"],
                    capture_output=True,
                    timeout=15,
                )
                return False

        except asyncio.TimeoutError:
            print("✗ Connection timeout")
            subprocess.run(
                ["docker-compose", "down"],
                capture_output=True,
                timeout=15,
            )
            return False

    except subprocess.TimeoutExpired:
        print("✗ Operation timed out")
        return False
    except Exception as e:
        print(f"✗ Test failed: {e}")
        return False


def main():
    """Run all validation checks."""
    import argparse

    parser = argparse.ArgumentParser(description="Validate test environment")
    parser.add_argument(
        "--quick-test",
        action="store_true",
        help="Also run quick Docker connectivity test",
    )
    args = parser.parse_args()

    print("=" * 60)
    print("BROKEN SHADOWS TEST ENVIRONMENT VALIDATION")
    print("=" * 60)

    results = {
        "Docker": check_docker(),
        "Python": check_python(),
        "Repository": check_repository(),
    }

    if args.quick_test:
        results["Connectivity"] = quick_connectivity_test()

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)

    all_passed = True
    for name, result in results.items():
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status}: {name}")
        if not result:
            all_passed = False

    print("=" * 60)

    if all_passed:
        print("\n✓ All checks passed!")
        print("\nYou can now run the test suite:")
        print("  ./test_mud_automated.py")
        return 0
    else:
        print("\n✗ Some checks failed. See details above.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
