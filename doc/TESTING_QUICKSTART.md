# Broken Shadows MUD - Automated Testing Quick Start

## Files Created

Three new files have been added to support automated testing:

1. **`bin/test_mud_automated.py`** (15 KB)
   - Main test suite - runs dockerbuild, server startup, game tests, and cleanup
   - Executable Python script

2. **`TEST_SUITE_README.md`**
   - Comprehensive guide for using the test suite
   - Usage examples, troubleshooting, and advanced features

3. **`bin/validate_testenv.py`**
   - Environment validation tool
   - Checks for required dependencies

## Quick Start

### 1. First Time Setup - Validate Environment

```bash
cd /path/to/broken_shadows
python3 bin/validate_testenv.py
```

This checks:

- Docker and Docker Compose installation
- Python 3.6+ availability
- Repository structure
- All required files

**Note:** telnetlib is a Python standard library and will work even if the validator shows it's missing.

### 2. Run Full Test Suite (with Docker build)

```bash
./bin/test_mud_automated.py
```

This will:

1. Build the Docker image (1-2 minutes first time)
2. Start the container
3. Create test character
4. Test movement, inventory, logout
5. Automatically clean up

### 3. Run Tests with Pre-Built Image

```bash
./bin/test_mud_automated.py --skip-build
```

Much faster (~15 seconds) if you already have the Docker image.

### 4. Use Custom Port

```bash
./bin/test_mud_automated.py --port 5000
```

## Test Coverage

The automated suite tests:

| Feature | Test | Status |
| --------- | ------ | -------- |
| Docker Build | Compiles image | ✓ Automated |
| Container | Starts and runs | ✓ Automated |
| Connection | Telnet to port 4000 | ✓ Automated |
| Character | Create new character | ✓ Automated |
| Movement | Navigate between rooms | ✓ Automated |
| Inventory | View items | ✓ Automated |
| Logout | Quit game | ✓ Automated |
| Cleanup | Remove test data | ✓ Automated |

## Example Run

```bash
$ ./bin/test_mud_automated.py --skip-build

============================================================
[2026-03-07 13:15:22] [START] BROKEN SHADOWS MUD AUTOMATED TEST SUITE
============================================================
[2026-03-07 13:15:23] [INFO] Starting Docker container...
[2026-03-07 13:15:26] [PASS] ✓ PASSED: Container startup
[2026-03-07 13:15:28] [PASS] ✓ PASSED: Server responsiveness
[2026-03-07 13:15:29] [PASS] ✓ PASSED: Telnet connection
[2026-03-07 13:15:30] [PASS] ✓ PASSED: Character creation
[2026-03-07 13:15:35] [PASS] ✓ PASSED: Movement
[2026-03-07 13:15:36] [PASS] ✓ PASSED: Inventory check
[2026-03-07 13:15:37] [PASS] ✓ PASSED: Logout
[2026-03-07 13:15:38] [INFO] Cleaning up Docker container...
[2026-03-07 13:15:40] [INFO] Cleaning up player file...
============================================================
[2026-03-07 13:15:40] [RESULT] TESTS PASSED: 7
[2026-03-07 13:15:40] [RESULT] TESTS FAILED: 0
============================================================
```

## What Gets Tested

### 1. Docker Build Phase

- Compiles C source code
- Creates Docker image
- Installs runtime dependencies

### 2. Container Startup

- Initializes Docker container
- Loads world data files
- Starts game server on port 4000

### 3. Server Connectivity

- Verifies server responds to Telnet
- Confirms port is open and accepting connections

### 4. Character Creation

- Enters new character name
- Sets password
- Receives welcome message
- Confirms character loaded into game

### 5. Movement Test

- Executes `look` command
- Navigates with `north` command
- Verifies room descriptions

### 6. Inventory

- Runs `inventory` command
- Verifies command processing

### 7. Logout

- Executes `quit` command
- Confirms logout
- Closes connection

### 8. Cleanup

- Stops Docker container
- Removes test player file
- Cleans up temporary data

## Requirements

- **Docker** - Container runtime
- **Docker Compose** - Container orchestration
- **Python 3.6+** - Test script
- **macOS/Linux** - Supported OS (Windows requires WSL)

## Environment Variables (Optional)

You can set custom values if needed:

```bash
# Custom port (also use --port flag)
export MUD_PORT=5000

# Run directly (also use --skip-build)
export SKIP_BUILD=1
```

## Interpreting Results

### All Tests Passed ✓

```text
TESTS PASSED: 8
TESTS FAILED: 0
```

Server is working correctly. Ready to use.

### Some Tests Failed ✗

```text
TESTS PASSED: 5
TESTS FAILED: 3
```

There's an issue. Check the detailed output above for which tests failed.

### Common Issues

#### "Server did not respond"

- Docker image didn't build successfully
- Server failed to start in container
- Port already in use

#### "Character creation failed"

- Previous test character still exists
- Server not responding to input

#### "Connection refused"

- Container didn't start
- Docker daemon not running

See `TEST_SUITE_README.md` for detailed troubleshooting.

## Integration with CI/CD

For automated testing in GitHub Actions or similar:

```yaml
- name: Run MUD Tests
  run: |
    cd broken_shadows
    ./bin/test_mud_automated.py --skip-build
```

With timeout protection:

```yaml
- name: Run MUD Tests
  timeout-minutes: 5
  run: |
    cd broken_shadows
    timeout 300 ./bin/test_mud_automated.py --skip-build
```

## Manual Testing Alternative

If you prefer manual testing:

```bash
# Start server manually
docker-compose up -d

# Connect manually
telnet localhost 4000

# Stop server
docker-compose down
```

## Next Steps

1. ✓ Validate environment: `python3 bin/validate_testenv.py`
2. ✓ Run first test: `./bin/test_mud_automated.py --skip-build`
3. ✓ Review output and logs
4. ✓ Develop your own tests by extending `MUDTestSuite`

## Documentation

- **CODEBASE_DOCUMENTATION.md** - Full codebase reference (created previously)
- **TEST_SUITE_README.md** - Detailed testing guide
- **This file** - Quick start guide

## Support

For issues:

1. Check the error messages in test output
2. Review `TEST_SUITE_README.md` Troubleshooting section
3. Run `docker-compose logs broken_shadows` for server logs
4. Try manual `telnet localhost 4000` connection

---

**Test Suite Version:** 1.0
**Created:** March 7, 2026
**Status:** Ready to use
