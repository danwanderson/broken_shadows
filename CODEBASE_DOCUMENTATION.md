# Broken Shadows MUD - Codebase Documentation

**Last Updated:** March 7, 2026
**Purpose:** Reference documentation for LLM-assisted development and maintenance

---

## 1. Project Overview

### What is Broken Shadows?

Broken Shadows is a **Multi-User Dungeon (MUD)** game server written in C. It is a text-based multiplayer role-playing game where players connect via Telnet to explore a fantasy world, fight monsters, complete quests, and interact with other players.

### Technical Summary

- **Language:** C (compiled with Clang)
- **Platform:** Linux (containerized with Docker)
- **Network Protocol:** Raw TCP sockets (Telnet)
- **Default Port:** 4000
- **Lineage:** Based on ROM 2.3/2.4 → Merc Diku Mud → Diku Mud
- **Author:** Daniel Anderson (Rahl) - <https://github.com/danwanderson>
- **License:** Custom derivative license (see section 2)

### Key Features

- **Hot-boot/Copyover:** Restart server without disconnecting players
- **Online Level Creator (OLC):** In-game world building tools
- **Clan System:** Player organizations with clan halls and channels
- **Quest System:** Automated quest generation
- **Multiple Classes:** Warrior, Mage, Cleric, Thief, Paladin
- **Multiple Races:** Human, Elf, Dwarf, Halfling, Pixie, Half-elf, Half-orc, Half-ogre, Half-troll, Gith, Drow, Troll, Hobbit
- **Original Features:** Socials, marriage system, music system, flowers, engraving, dice games, etc.

---

## 2. Licensing and Credits

### License Hierarchy

The codebase follows a **chain of derivative licenses**:

1. **Diku Mud License** (license.doc)
2. **Merc License** (license.txt)
3. **ROM License** (rom.license)
4. **Broken Shadows License** (shadows.license)

### Key License Requirements

All users/developers must:

- Follow all upstream licenses (Diku, Merc, ROM)
- Include notice that the MUD is based on Broken Shadows code in login messages
- Email author (<https://github.com/danwanderson>) before opening a Broken Shadows-based MUD
- Not call their MUD "Broken Shadows" (only derivatives allowed)
- Cannot release official Broken Shadows source (only Rahl can do that)

### Credits

**Upstream Projects:**

- Original Diku Mud (Sebastian Hammer, Michael Seifert, et al.)
- Merc Diku Mud (Michael Chastain, Michael Quan, Mitchell Tse)
- ROM 2.3/2.4 (Russ Taylor and ROM crew)
- EmberMUD 0.17 (Zak, Zane, Thexder)

**Major Contributors:**

- Vassago (quest system)
- Erwin (copyover, rename, board, buffer)
- Rindar (shield block, boot_db fixes)
- Many others (see doc/shadows.credits)

---

## 3. Technical Architecture

### Architecture Pattern

Broken Shadows uses a **single-threaded, event-driven architecture** common to MUD servers:

1. **Main Loop** (comm.c): Handles all I/O via select() system call
2. **Game Loop** (update.c): Updates game state (combat, regeneration, weather, etc.)
3. **Command Interpreter** (interp.c): Parses and executes player commands
4. **Database** (db.c): Loads/saves world data and player files

### Memory Model

- **Static String Pool:** Fixed-size string storage (1,048,576 bytes default)
- **Object Recycling:** Reuses freed memory structures (recycle.h)
- **No Dynamic Libraries:** All code compiled into single binary

### Network Model

- **Protocol:** Raw TCP sockets, no encryption
- **Connection Handling:** Non-blocking I/O with select()
- **Port:** Default 4000 (configurable via command line)
- **Hot-boot Support:** Can transfer active connections during restart

### Data Persistence

**World Data** (area/ directory):

- `.are` files: Area definitions (rooms, mobs, objects, shops, resets)
- Text-based format, human-readable
- Loaded at boot, modified via OLC

**Player Data** (player/ directory):

- Individual files per character
- Plain text format
- Saved on quit, periodically, and on crash

**God/Immortal Data** (gods/ directory):

- Similar to player files
- Separate storage for staff characters

**Notes** (notes/ directory):

- In-game message boards (General, Bugs, Ideas, etc.)

---

## 4. Directory Structure

```text
broken_shadows/
├── area/              # World data files (.are format)
│   ├── *.are         # Individual area files
│   ├── area.lst      # Master list of areas to load
│   ├── MOBProgs/     # Mobile (NPC) programs
│   └── old/          # Deprecated or backup areas
├── backup/           # Automated backups (if configured)
├── bin/              # Compiled binaries and utility scripts
│   ├── shadows       # Main game server executable (compiled from src/)
│   ├── start_shadows.sh  # Server startup/restart wrapper script
│   ├── backup.sh     # Backup utility
│   ├── clean_pfiles.sh   # Player file cleanup
│   └── stats.sh      # Statistics generator
├── core/             # Core dumps for debugging
├── doc/              # Documentation files
│   ├── shadows.license   # Broken Shadows license
│   ├── shadows.credits   # Full credits list
│   ├── changelog.txt     # Historical changelog (1996-1997)
│   ├── area.txt          # Area file format documentation
│   ├── command.txt       # Command system documentation
│   ├── skill.txt         # Skill/spell system documentation
│   └── *.txt             # Various other docs
├── gods/             # Immortal/staff player files
├── log/              # Server log files
├── notes/            # In-game bulletin boards
│   ├── General       # General board
│   ├── Bugs          # Bug reports
│   ├── Ideas         # Player suggestions
│   ├── Immortal      # Staff-only board
│   └── Penalties     # Disciplinary actions
├── player/           # Player character files
│   ├── [A-Z]/        # Organized alphabetically (may not exist yet)
│   └── *             # Individual player files
├── src/              # C source code
│   ├── *.c           # Implementation files
│   ├── *.h           # Header files
│   ├── Makefile      # Build configuration
│   ├── save/         # Player save subsystem (if separate)
│   └── unused/       # Deprecated code
├── Dockerfile        # Multi-stage Docker build definition
├── docker-compose.yml    # Docker orchestration
├── build_image.sh    # Docker image build script
├── update_image.sh   # Docker image update script
├── run_local.sh      # Local Docker run script
└── connect_local.sh  # Local Telnet connection script
```

### Critical Files

| File | Purpose |
| ------ | --------- |
| `src/merc.h` | Main header with all type definitions and constants |
| `src/comm.c` | Network I/O and main event loop |
| `src/db.c` | Database loading/saving |
| `src/interp.c` | Command interpreter |
| `src/act_*.c` | Player action implementations (communication, movement, info, etc.) |
| `src/fight.c` | Combat system |
| `src/magic.c` | Spell system |
| `src/skills.c` | Skill system |
| `src/update.c` | Game tick updates |
| `src/handler.c` | Object/character manipulation utilities |
| `area/area.lst` | Master list of areas to load |
| `bin/start_shadows.sh` | Main startup script with crash recovery |

---

## 5. Build System

### Makefile Configuration

**Location:** `src/Makefile`

**Compiler:** Clang (can be changed to GCC)

**Compilation Flags:**

```makefile
CC      = clang
PROF    = -DLINUX
C_FLAGS = -Wall -O2 $(PROF) $(NOCRYPT)
L_FLAGS = -O2 $(PROF) -lcrypt
```

**Key Targets:**

```bash
make          # Build the 'shadows' executable
make clean    # Remove object files
make src      # Create git tags and push (commented in current version)
make srcpull  # Pull from git repository
```

### Build Process

**Step-by-step:**

```bash
cd src/
make
```

**Output:**

- Object files (*.o) in src/
- Executable: `src/shadows`

**Dependencies:**

- Standard C library
- libcrypt (for password hashing)
- libm (math library)

### Docker Build

**Multi-stage build** (see Dockerfile):

**Stage 1:** Build the binary

- Base: Debian latest
- Installs: clang, make, gdb, telnet
- Compiles shadows executable

**Stage 2:** Runtime image

- Copies compiled binary from Stage 1
- Sets up directory structure
- Runs as non-root user 'shadows'
- Entry point: `/srv/shadows/bin/start_shadows.sh`

**Build commands:**

```bash
./build_image.sh      # Build Docker image
./update_image.sh     # Update Docker image
./run_local.sh        # Run locally with Docker
```

---

## 6. Deployment

### Docker Deployment (Recommended)

**Using docker-compose:**

```bash
docker-compose up -d
```

**Configuration:** `docker-compose.yml`

- Image: `danwanderson/broken_shadows:latest`
- Port: 4000:4000/tcp
- Volumes: Mounts local data directories (area, gods, log, notes, player, core)
- Restart policy: unless-stopped

**Advantages:**

- Consistent environment
- Easy updates
- Isolated from host system
- Automatic restarts

### Native Deployment

**Requirements:**

- Linux system
- Clang or GCC
- Standard development tools
- libcrypt development headers

**Steps:**

1. Compile: `cd src && make`
2. Copy binary: `cp src/shadows bin/`
3. Start: `cd area && ../bin/start_shadows.sh [port]`

### Server Startup Script

**Script:** `bin/start_shadows.sh`

**Features:**

- Infinite restart loop (keeps server running after crashes)
- Core dump capture with GDB backtrace
- Automatic cleanup of shutdown.txt
- 10-second delay between restarts
- Configurable port (default 4000)

**Usage:**

```bash
./bin/start_shadows.sh          # Start on port 4000
./bin/start_shadows.sh 5000     # Start on port 5000
```

**Stopping:**

- Create `area/shutdown.txt` (in-game: `shutdown` command)
- Or send SIGTERM to process

---

## 7. Key Components

### 7.1 Core Systems

#### Communication System (comm.c)

**Responsibilities:**

- Socket initialization and management
- Main game loop (select-based I/O multiplexing)
- Player input/output buffering
- Connection accept/close
- Hot-boot (copyover) functionality

**Key Functions:**

- `main()` - Program entry point
- `init_socket(int port)` - Create and bind listening socket
- `game_loop_select()` - Main event loop
- `read_from_descriptor()` - Read player input
- `write_to_descriptor()` - Send output to player

#### Database System (db.c, db.h)

**Responsibilities:**

- Load area files at boot
- Create in-memory world representation
- Save/load player files
- Reset areas based on reset data

**Key Functions:**

- `boot_db()` - Initialize entire world database
- `load_area(FILE *fp)` - Parse and load one area file
- `save_char_obj(CHAR_DATA *ch)` - Save player to disk
- `load_char_obj(DESCRIPTOR_DATA *d, char *name)` - Load player

**Data Structures:**

- Linked lists of rooms, objects, mobiles
- Hash tables for fast lookups
- String allocation pool

#### Command Interpreter (interp.c, interp.h)

**Responsibilities:**

- Parse player input
- Match commands (with partial matching)
- Verify permissions (level/position requirements)
- Execute command functions
- Handle social emotes

**Key Data:**

- `cmd_table[]` - Array of all commands
- Command structure includes: name, function pointer, position, level, log level

**Key Functions:**

- `interpret(CHAR_DATA *ch, char *argument)` - Main command dispatcher
- Command functions: `do_<command>(CHAR_DATA *ch, char *argument)`

### 7.2 Game Systems

#### Combat System (fight.c)

**Features:**

- Turn-based combat with automatic attacks
- Multi-attack for high-level characters
- Weapon types and damage types
- Armor class system
- Special attacks (bash, trip, kick, etc.)

**Key Functions:**

- `violence_update()` - Process one round of combat for all fighting characters
- `damage()` - Core damage calculation and application
- `death_cry()` - Handle character death

#### Magic System (magic.c, magic.h)

**Features:**

- Spell casting with mana costs
- Spell memorization system
- Target types (self, character, object, room)
- Saving throws
- Spell effects (affects)

**Spell Types:**

- Offensive (damage spells)
- Defensive (protection, healing)
- Detection spells
- Transportation (recall, portal)
- Creation spells

**Key Functions:**

- `cast_spell()` - Execute spell casting
- Individual `spell_*()` functions for each spell

#### Skill System (skills.c)

**Features:**

- Practice-based improvement
- Skill checks with % success
- Class-specific skill lists
- Skill delays and prerequisites

**Key Skills:**

- Combat: backstab, circle, bash, trip
- Utility: hide, sneak, pick lock, steal
- Crafting: butcher, engrave

#### Quest System (quest.c)

**Features (1997 additions):**

- Automatic quest generation
- Quest points as currency
- Quest timer tracking
- Quest rewards (items, gold, quest points)

### 7.3 Special Features

#### Online Level Creator (olc.c, olc_act.c, olc_save.c, olc.h)

**Purpose:**
In-game world building tools for immortals (level 92+)

**Capabilities:**

- Create/edit rooms, mobiles, objects
- Define resets (mob/object spawning)
- Create shops
- Set area parameters
- Real-time world modification

**Commands:**
redit, medit, oedit, aedit, etc.

#### Clan System (clans.c)

**Features:**

- Player-created clans
- Clan halls (private areas)
- Clan channels
- Clan ranks and leadership
- Clan recall command

#### Copyover/Hot-boot

**Purpose:**
Restart server without disconnecting players

**How it works:**

1. Save all player states
2. Write connection file descriptors to file
3. exec() new server process
4. New process inherits file descriptors
5. Reconnect all players seamlessly

**File:** `COPYOVER_DATA` (temporary, deleted after successful copyover)

#### Marriage System (marry.c)

Custom feature allowing player characters to marry in-game.

#### Music System (music.c, music.h)

System for playing in-game music or sounds (text-based descriptions).

#### Flowers System (flowers.c)

Create and give flowers to other players.

#### Engraving System (engrave.c)

Engrave custom messages on items.

#### Dice Games (dice_games.c)

In-game gambling mechanics.

### 7.4 Support Systems

#### Buffer System (buffer.c, buffer.h)

**Purpose:**
Efficient string building and memory management

**Prevents:**
Buffer overflow vulnerabilities

#### Alias System (alias.c)

Player-created command aliases and shortcut macros.

#### Ban System (ban.c)

Site-based banning for problematic players/sites.

#### Board System (board.c, board.h)

In-game bulletin board system (notes/).

---

## 8. Source Code Organization

### File Naming Conventions

| Pattern | Purpose |
| --------- | --------- |
| `act_*.c` | Player action commands (act_comm, act_move, act_info, act_wiz, etc.) |
| `*.h` | Header files (type definitions, function prototypes, macros) |
| `olc*.c` | Online Level Creator subsystem |
| `*_update.c` | Periodic update systems |
| `mob_*.c` | Mobile (NPC) behaviors |

### Key Header Files

**merc.h** - Main header

- All struct definitions (CHAR_DATA, OBJ_DATA, ROOM_INDEX_DATA, etc.)
- Global constants
- Function prototypes
- Macros

**interp.h** - Command interpreter

- Command table declarations
- DO_FUN macro (command function signature)

**magic.h** - Magic system

- Spell definitions
- Spell function prototypes

**olc.h** - Online Level Creator

- OLC-specific structures
- Editor modes

**recycle.h** - Memory recycling

- Object allocation/deallocation macros

**board.h** - Bulletin board system

- Board data structures

### Code Style

**Conventions observed:**

- K&R-style brace placement (mostly)
- 4-space indentation (mixed with tabs)
- Snake_case for functions
- ALL_CAPS for constants and macros
- Hungarian-style prefixes for some variables (ch = character, obj = object)

**Common Patterns:**

```c
// Command function signature
void do_command_name(CHAR_DATA *ch, char *argument)

// Iteration over character list
for (CHAR_DATA *vch = char_list; vch != NULL; vch = vch->next)

// Sending output to player
send_to_char("Message\n\r", ch);

// Checking if character is NPC
if (IS_NPC(ch))
```

---

## 9. Data File Formats

### Area Files (.are)

**Format:** Text-based with section markers

**Sections:**

```text
#AREA        - Area header (name, author, level range)
#HELPS       - Help file entries
#MOBILES     - Mobile (NPC) definitions
#OBJECTS     - Object definitions
#ROOMS       - Room descriptions and exits
#RESETS      - Spawn/reset instructions
#SHOPS       - Shop keeper configurations
#SPECIALS    - Special mob behaviors
#$           - End marker
```

**Documentation:** See `doc/area.txt` for complete format specification

### Player Files

**Format:** Plain text, key-value pairs

**Location:** `player/` (may be organized alphabetically)

**Content:**

- Character attributes (stats, level, class, race)
- Inventory and equipment
- Skills and spell knowledge
- Quest progress
- Clan membership
- Aliases

**Example snippet:**

```text
Name        Character~
Sex         1
Class       2
Race        1
Level       50
```

---

## 10. Running and Testing

### Connecting to the Server

#### Method 1: Telnet

```bash
telnet localhost 4000
```

#### Method 2: MUD client

- TinTin++
- MUSHclient
- Mudlet
- zMUD/CMUD

#### Method 3: Local connection script

```bash
./connect_local.sh
```

### Creating a God/Immortal Character

**Steps:**

1. Connect and create new character
2. Disconnect
3. Edit player file directly
4. Change level to 98 or 99
5. Reconnect

**Note:** There should be proper "advance" or "promote" commands for this, but manual editing works.

### Common Immortal Commands

| Command | Level | Purpose |
| --------- | ------- | --------- |
| `goto` | 92+ | Teleport to room |
| `transfer` | 92+ | Summon player |
| `slay` | 97+ | Kill player |
| `shutdown` | 98+ | Stop server |
| `copyover` | 98+ | Hot reboot |
| `redit` | 92+ | Edit rooms |
| `medit` | 92+ | Edit mobiles |
| `oedit` | 92+ | Edit objects |
| `wiznet` | 92+ | Immortal chat channel |

### Development Workflow

**Typical cycle:**

1. Make code changes in `src/`
2. Compile: `cd src && make`
3. Test on local instance
4. Use `copyover` for hot reload (or restart server)
5. Check logs in `log/` for errors
6. If crash, examine core dump in `core/`

### Debugging

**Core Dumps:**

- Automatically captured by `start_shadows.sh`
- Stored in `core/` with timestamp
- GDB backtrace saved as `.txt` file

**Manual GDB:**

```bash
gdb bin/shadows core/core.TIMESTAMP
(gdb) bt full
```

**Logging:**

- Check `log/` directory
- Common logs: game activity, bugs, security issues

---

## 11. Common Development Tasks

### Adding a New Command

**Steps:**

1. Add command function in appropriate `act_*.c` file:

   ```c
   void do_newcommand(CHAR_DATA *ch, char *argument)
   {
       send_to_char("You execute the new command!\n\r", ch);
   }
   ```

2. Add prototype to `merc.h`:

   ```c
   DECLARE_DO_FUN(do_newcommand);
   ```

3. Add to command table in `interp.c`:

   ```c
   { "newcommand", do_newcommand, POS_STANDING, 0, LOG_NORMAL, 1 }
   ```

4. Add help entry in `area/help.are`:

   ```text
   0 NEWCOMMAND~
   Syntax: newcommand


5. Recompile and test

### Adding a New Spell

**Steps:**

1. Define spell in `magic.h` (add to spell enum)
2. Implement `spell_newspell()` function in `magic.c`
3. Add to spell table in `const.c`
4. Add to class skill tables in `const.c`
5. Create help entry
6. Recompile

### Adding a New Area

**Steps:**

1. Create new `.are` file in `area/` (use `area/proto.are` as template)
2. Define rooms, mobiles, objects, resets
3. Add filename to `area/area.lst`
4. Restart server (or use OLC to load dynamically)
5. Test area thoroughly

### Modifying Existing Features

**Best practices:**

- **Read before editing:** Understand existing code flow
- **Backup first:** Copy to `src/unused/` or comment out
- **Test incrementally:** Small changes, frequent testing
- **Check for side effects:** Search for function usage across codebase
- **Update help files:** Keep documentation in sync

---

## 12. Important Constants and Limits

### Level System

| Level Range | Status |
| ------------- | -------- |
| 1-51 | Mortal players |
| 52-60 | Hero levels |
| 92-99 | Immortal/God levels |
| 97+ | High-level admin (slay, smite) |
| 98+ | Full admin (shutdown, copyover) |

### Position States

```c
POS_DEAD
POS_MORTAL
POS_INCAP
POS_STUNNED
POS_SLEEPING
POS_RESTING
POS_SITTING
POS_FIGHTING
POS_STANDING
```

### Wear Locations

```c
WEAR_LIGHT
WEAR_FINGER_L / WEAR_FINGER_R
WEAR_NECK_1 / WEAR_NECK_2
WEAR_BODY
WEAR_HEAD
WEAR_LEGS
WEAR_FEET
WEAR_HANDS
WEAR_ARMS
WEAR_SHIELD
WEAR_ABOUT
WEAR_WAIST
WEAR_WRIST_L / WEAR_WRIST_R
WEAR_WIELD
WEAR_HOLD
WEAR_FLOAT
```

### Classes

```c
CLASS_MAGE
CLASS_CLERIC
CLASS_THIEF
CLASS_WARRIOR
CLASS_PALADIN
```

### Item Types

```c
ITEM_LIGHT
ITEM_SCROLL
ITEM_WAND
ITEM_STAFF
ITEM_WEAPON
ITEM_TREASURE
ITEM_ARMOR
ITEM_POTION
ITEM_FURNITURE
ITEM_CONTAINER
ITEM_DRINK_CON
ITEM_FOOD
ITEM_MONEY
(and many more...)
```

---

## 13. Known Issues and Technical Debt

### From Changelog (1996-1997)

**Fixed in past:**

- Polearm bug (were set to whips in OLC)
- Order/mobprog bug
- mshow bug (crashed on non-number input)
- Count command issues
- CHAOS point scoring
- XP formula adjustments
- Various compile warnings
- Buffer overflows in rlist

**Historical Context:**
Most active development was 1996-1997. Code may contain legacy patterns and dated practices.

### Potential Modern Concerns

**Security:**

- No input sanitization in many places (classic C buffer issues)
- Plain text passwords (libcrypt, but old)
- No TLS/SSL support
- Direct telnet with no encryption

**Code Quality:**

- Mixed coding styles
- Global variables
- Manual memory management (prone to leaks)
- Limited error handling in places
- Some hard-coded limits

**Architecture:**

- Single-threaded (can't utilize multiple cores)
- No unit tests
- Tight coupling in some areas
- Fixed-size string pool

**Recommendations:**

- Add comprehensive input validation
- Consider modernizing security (TLS support)
- Gradually add error handling
- Document memory ownership
- Add automated testing

---

## 14. Dependencies and Requirements

### System Requirements

**Operating System:**

- Linux (primary target)
- Possibly BSD/Unix (not tested recently)
- Windows via WSL or Cygwin (not officially supported)

**Build Tools:**

- C compiler (Clang or GCC)
- Make
- Standard POSIX headers

**Runtime Libraries:**

- libc (standard C library)
- libcrypt (password hashing)
- libm (math functions)

**Optional:**

- Docker (for containerized deployment)
- GDB (for debugging)
- Git (for version control)
- Telnet client (for testing)

### Port Requirements

**Default Ports:**

- 4000/TCP: Main game port
- No other ports required

**Firewall Configuration:**
Allow incoming TCP connections on game port (4000 by default).

---

## 15. Future Development Considerations

### Modernization Opportunities

**Code Quality:**

- Refactor to reduce global state
- Add const correctness
- Improve error handling
- Add logging framework
- Consider C++ migration (gradual)

**Features:**

- Web-based client support (WebSocket)
- TLS encryption for security
- Database backend (PostgreSQL/MySQL) instead of flat files
- RESTful API for statistics/monitoring
- Multi-threading for specific tasks

**Tooling:**

- Add unit tests (Check framework, etc.)
- Continuous integration (GitHub Actions)
- Static analysis (Clang-tidy, Coverity)
- Automated area validation
- Player file migration tools

**Documentation:**

- Generate API docs with Doxygen
- Create architecture diagrams
- Document all commands
- Create builder's guide for areas
- Video tutorials for OLC

### Backward Compatibility

**Considerations when updating:**

- Player file format changes require migration scripts
- Area file format should remain compatible
- Consider version numbers in save files
- Maintain hot-boot compatibility

---

## 16. Additional Resources

### Internal Documentation

| File | Content |
| ------ | --------- |
| `doc/area.txt` | Area file format specification |
| `doc/command.txt` | Command system documentation |
| `doc/skill.txt` | Skill/spell system |
| `doc/changelog.txt` | Historical changes (1996-1997) |
| `doc/shadows.credits` | Full credits list |
| `doc/rom.credits` | ROM credits |
| `doc/license.*` | All relevant licenses |
| `area/olc.hlp` | OLC help file |

### External References

**ROM Resources:**

- ROM 2.4 documentation (compatible base)
- ROM mailing list archives
- MUD development forums

**Diku/Merc History:**

- Classic MUD lineage documentation
- Historical MUD development resources

**C Programming:**

- POSIX system call documentation
- Socket programming tutorials
- Classic C idioms and patterns

---

## 17. Quick Reference

### Starting the Server

```bash
# Docker method (recommended)
docker-compose up -d

# Native method
cd area
../bin/start_shadows.sh

# With custom port
../bin/start_shadows.sh 5000
```

### Building from Source

```bash
cd src
make clean
make
cp shadows ../bin/
```

### Connecting

```bash
telnet localhost 4000
# or
./connect_local.sh
```

### Stopping the Server

```bash
# Graceful (in-game command as immortal)
shutdown

# Or create shutdown file
touch area/shutdown.txt

# Docker
docker-compose down
```

### Getting Help In-Game

```text
help
help commands
help <topic>
areas
score
stat
who
```

### Essential Immortal Commands

```text
goto <vnum>          # Teleport to room
transfer <player>    # Summon player
holylight            # See hidden/invis
wizinvis <level>     # Set wiz invisibility
copyover             # Hot reboot
redit                # Enter room editor
```

---

## 18. Glossary

| Term | Definition |
| ------ | ------------ |
| **MUD** | Multi-User Dungeon - text-based multiplayer game |
| **Area** | A zone/region in the game world (collection of rooms) |
| **Mobile/Mob** | Non-player character (NPC) |
| **OLC** | Online Level Creator - in-game world building tools |
| **Vnum** | Virtual number - unique ID for rooms, mobs, objects |
| **Reset** | Spawning instruction for mobs/objects |
| **Copyover** | Hot-boot/hot-reboot - restart without disconnecting players |
| **Immortal** | Staff member with elevated permissions |
| **God** | High-level immortal (level 98-99) |
| **PC** | Player Character |
| **NPC** | Non-Player Character (same as mobile) |
| **Affect** | Temporary status effect (spell effect, poison, etc.) |
| **Prog** | Program - scripted behavior (MOBProg = mobile program) |
| **PK** | Player Killing - PvP combat |

---

## 19. Contact and Support

**Original Author:** Daniel Anderson (Rahl)
**GitHub:** <https://github.com/danwanderson>

**License Questions:** Contact author before using code

**This Documentation:**

- Created: March 7, 2026
- Purpose: LLM-assisted development reference
- Maintainer: Repository owner

---

## 20. Notes for LLM Development Sessions

### Key Points for Future Sessions

1. **This is legacy C code** from mid-1990s - expect older patterns
2. **Single-threaded architecture** - all game logic in one event loop
3. **Manual memory management** - watch for leaks and buffer overflows
4. **No modern frameworks** - pure C with POSIX sockets
5. **Flat file persistence** - no SQL database
6. **Hot-boot support** - code must preserve connection state for copyover

### When Making Changes

- **Always read surrounding code** - understand context before editing
- **Test incrementally** - MUDs are complex, small changes first
- **Check multiple files** - functionality often spans act_*.c files
- **Update help files** - keep documentation in sync in area/help.are
- **Consider OLC** - immortals can modify world at runtime
- **Respect licenses** - must maintain all copyright notices

### Common Pitfalls

- **Buffer overflows** - validate all input lengths
- **Null pointer dereferences** - check before dereferencing
- **Memory leaks** - use recycle functions properly
- **String handling** - use safe string functions
- **Global state** - be aware of shared variables
- **Save file format** - changes require migration

### Useful Search Patterns

```bash
# Find command implementations
grep -r "void do_" src/

# Find spell implementations
grep -r "spell_" src/magic.c

# Find global variables
grep -r "^[A-Z_]*\s*[a-z_]*;" src/*.c

# Find struct definitions
grep -r "^struct.*{" src/*.h
```

---

## End of Documentation

This document should be updated as the codebase evolves. Future LLM sessions should read this file first to understand the project context before making changes.
