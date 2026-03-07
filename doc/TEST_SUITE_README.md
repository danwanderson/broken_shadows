# Broken Shadows MUD - Automated Test Suite

## Overview

The automated test suite provides comprehensive functional testing of the Broken Shadows MUD server. It automates the entire process from building the Docker image through testing core game functionality and cleanup.

## What It Tests

The test suite validates the following functionality:

- **Docker Build** - Compiles and builds the Docker image
- **Container Startup** - Starts the Docker container successfully
- **Server Responsiveness** - Server responds to Telnet connections
- **Character Creation** - Creates a test character with password
- **Movement** - Movement commands work between rooms
- **Inventory** - Inventory system functions correctly
- **Logout** - Graceful disconnection from the game
- **Cleanup** - Properly stops container and removes test data

## Requirements

- **Docker & Docker Compose** - For containerization
- **Python 3.6+** - For the test script
- **Standard Libraries** (included with Python):
  - telnetlib
  - subprocess
  - pathlib
  - argparse

## Installation

The test script is already in the repository root:

```bash
cd /path/to/broken_shadows
```

The script is executable by default. If permissions were lost, restore them:

```bash
chmod +x bin/test_mud_automated.py
```

## Usage

### Basic Usage (Build + Full Test)

```bash
./bin/test_mud_automated.py
```

This will:

1. Build the Docker image
2. Start the container
3. Run all game tests
4. Clean up automatically

### Skip Docker Build

If you've already built the image:

```bash
./bin/test_mud_automated.py --skip-build
```

### Custom Port

If your MUD runs on a non-standard port:

```bash
./bin/test_mud_automated.py --port 5000
```

### Combined Options

```bash
./bin/test_mud_automated.py --skip-build --port 5000
```

## Output

The test suite provides detailed, timestamped logging:

```text
[2026-03-07 13:15:22] [START] BROKEN SHADOWS MUD AUTOMATED TEST SUITE
[2026-03-07 13:15:23] [INFO] Building Docker image...
[2026-03-07 13:15:45] [PASS] ✓ PASSED: Docker build
[2026-03-07 13:15:46] [INFO] Starting Docker container...
[2026-03-07 13:15:50] [PASS] ✓ PASSED: Container startup
[2026-03-07 13:15:52] [INFO] Waiting for server to be ready (timeout: 30s)...
[2026-03-07 13:15:53] [PASS] ✓ PASSED: Server responsiveness
[2026-03-07 13:15:54] [INFO] Connecting to MUD at localhost:4000...
[2026-03-07 13:15:55] [PASS] ✓ PASSED: Telnet connection
...
[2026-03-07 13:16:30] [RESULT] TESTS PASSED: 9
[2026-03-07 13:16:30] [RESULT] TESTS FAILED: 0
```

## Test Details

### Character Creation Test

- Connects via Telnet
- Enters test character name: `TestChar`
- Enters password: `test123`
- Confirms character creation
- Verifies welcome message received

### Movement Test

- Executes `look` command to verify current room
- Attempts to move using `north` command
- Verifies navigation capability
- (Note: Actual success depends on available exits in starting area)

### Inventory Test

- Issues `inventory` command
- Verifies command response

### Logout Test

- Sends `quit` command
- Confirms quit operation
- Closes Telnet connection

## Cleanup

The test suite automatically cleans up after itself:

1. **Docker Container** - Stopped and removed
2. **Player File** - Test character file deleted from `player/` directory
3. **Connection** - Telnet connection properly closed

If tests are interrupted (Ctrl+C), cleanup still occurs.

## Return Codes

- `0` - All tests passed
- `1` - One or more tests failed
- `130` - Tests interrupted by user (Ctrl+C)

## Troubleshooting

### Docker Not Found

```text
Error: docker-compose.yml not found. Run from repository root.
```

**Solution:** Run the script from the repository root directory.

### Port Already in Use

```texttext
[ERROR] Failed to start container: ...port is already allocated
```

**Solution:** Either:

- Stop the existing container: `docker-compose down`
- Use alternative port: `./bin/test_mud_automated.py --port 5000`

### Server Timeout

```text
[ERROR] Server did not respond within timeout
```

**Solution:** Check Docker container logs:

```bash
docker-compose logs broken_shadows
```

### Character Creation Fails

```text
[FAIL] ✗ FAILED: Character creation
```

**Solution:**

- Check that the MUD server started correctly
- Verify the test character doesn't already exist in `player/TestChar`
- Review detailed error messages in test output

### Connection Timeout

**Solution:** Increase timeout or check network:

```bash
docker-compose logs broken_shadows
telnet localhost 4000
```

## Advanced Usage

### Running in CI/CD

For automated testing in GitHub Actions or similar:

```bash
#!/bin/bash
set -e

cd broken_shadows
./bin/test_mud_automated.py --skip-build

# Or with timeout protection
timeout 300 ./bin/test_mud_automated.py --skip-build
```

### Running Specific Test Phases

To debug individual phases, modify `bin/test_mud_automated.py` or create a wrapper script.

### Custom Test Scenarios

The `MUDTestSuite` class can be extended for additional tests:

```python
from test_mud_automated import MUDTestSuite
import telnetlib

class ExtendedTestSuite(MUDTestSuite):
    def test_spellcasting(self, tn: telnetlib.Telnet):
        """Test spell system."""
        # Your test code here
        pass

# Use in main():
suite = ExtendedTestSuite(port=4000)
suite.run_all_tests()
```

## Files Generated/Modified

### During Test

- **Docker image** - Built as `danwanderson/broken_shadows:latest`
- **Docker container** - Running as `broken_shadows`
- **Player file** - Created as `player/TestChar`

### After Test

All temporary files are cleaned up. Only Docker image remains (for faster subsequent builds).

## Performance

Typical test execution times:

| Phase | Duration |
| ------ | --------- |
| Docker Build | 1-2 minutes (first time) |
| Container Start | ~3 seconds |
| Character Creation | ~5 seconds |
| Game Tests | ~5 seconds |
| Cleanup | ~2 seconds |
| **Total** | **~15 seconds** (skip-build) |
| **Total** | **60-90 seconds** (with build) |

## Logs and Debugging

Test output includes:

- Timestamps for all operations
- Command execution logs
- Pass/Fail status for each test
- Error messages with context

To capture output to file:

```bash
./bin/test_mud_automated.py > test_results.log 2>&1
```

## Future Enhancements

Potential improvements to the test suite:

- [ ] Additional command testing (combat, magic, skills)
- [ ] Player interaction tests (grouping, PK)
- [ ] OLC (Online Level Creator) tests
- [ ] Performance benchmarks
- [ ] Stress testing (multiple concurrent characters)
- [ ] Area validation
- [ ] Custom test profiles
- [ ] HTML/JSON report generation

## Contributing

To add new tests:

1. Add test method to `MUDTestSuite` class
2. Follow naming convention: `test_<feature_name>`
3. Use `self.test_passed()` and `self.test_failed()` for tracking
4. Include proper error handling and logging

Example:

```python
def test_socials(self, tn: telnetlib.Telnet) -> bool:
    """Test social commands."""
    try:
        self.send_command(tn, "emote tests the system", wait_for=">")
        self.log("Social test successful")
        self.test_passed("Social commands")
        return True
    except Exception as e:
        self.log(f"Social test failed: {e}", "ERROR")
        self.test_failed("Social commands")
        return False
```

## Support

If you encounter issues:

1. Check the detailed test output
2. Review Docker logs: `docker-compose logs broken_shadows`
3. Test manual Telnet connection: `telnet localhost 4000`
4. Verify Docker/Python installation

---

**Test Suite Version:** 1.0
**Last Updated:** March 7, 2026
**Compatible With:** Broken Shadows MUD (Docker containerized)
