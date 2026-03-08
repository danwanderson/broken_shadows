#!/usr/bin/env python3
"""
Validate Broken Shadows player YAML files for syntax and value correctness.

Usage:
    python3 tools/validate_player.py player/Rahl.yaml [player/Kahlan.yaml ...]
    python3 tools/validate_player.py --all player/
"""

import sys
import re
import argparse
import logging
from pathlib import Path
from typing import Any, cast

try:
    import yaml
except ImportError:
    print(
        "ERROR: PyYAML not installed. Run: pip install pyyaml",
        file=sys.stderr
    )
    sys.exit(1)


logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Constants mirrored from merc.h / const.c
# ---------------------------------------------------------------------------

MAX_LEVEL = 100
MAX_TRUST = 100          # implementor = MAX_LEVEL = 100 (security 0-9)
MAX_VNUM = 65535
MAX_ALIGNMENT = 1000
MAX_STAT = 25           # typical upper cap; immortals can exceed via sets
MIN_STAT = 3
MAX_COND = 48
MAX_ALIAS = 8
MAX_GOLD = 2_000_000

VALID_SEXES = {0, 1, 2}
VALID_CLASSES = set(range(5))    # 0=Mage,1=Cleric,2=Thief,3=Warrior,4=Paladin
VALID_POSITIONS = set(range(9))    # 0=POS_DEAD .. 8=POS_STANDING
VALID_SECURITY = set(range(10))   # 0-9

VALID_PC_RACES = {
    "dwarf", "elf", "giant", "hobbit", "human", "troll",
    "drow", "gnome", "half-elf", "wyvern", "Vampire",
}

# Apply locations valid on player affects (0-26, same as APPLY_*)
VALID_APPLY_LOCS = set(range(27))

# Valid affect 'where' field values (stored as integers in player files)
VALID_AFFECT_WHERE_INT = {0, 1, 2, 3, 4, 5}  # TO_AFFECTS, TO_OBJECT, etc.

KNOWN_BOARDS = {"General", "Ideas", "Announce", "Bugs", "Personal",
                "Penalties", "Immortal"}

# Flag string: uppercase A-Z (bits 0-25), lowercase a-f (bits 26-31), or '0'
FLAG_RE = re.compile(r'^(?:0|[A-Za-f]+)$')


# ---------------------------------------------------------------------------
# Validator
# ---------------------------------------------------------------------------

class Validator:
    def __init__(self, path: str):
        self.path = path
        self.errors: list[str] = []
        self.warnings: list[str] = []

    def err(self, ctx: str, msg: str):
        self.errors.append(f"  {ctx}: {msg}")

    def warn(self, ctx: str, msg: str):
        self.warnings.append(f"  {ctx}: {msg}")

    def check_int(
        self, ctx: str, value: Any, name: str,
        lo: int | None = None, hi: int | None = None,
    ) -> bool:
        if not isinstance(value, int):
            self.err(
                ctx,
                f"{name} must be an integer, "
                f"got {type(value).__name__} ({value!r})"
            )
            return False
        if lo is not None and value < lo:
            self.err(ctx, f"{name}={value} below minimum {lo}")
        if hi is not None and value > hi:
            self.err(ctx, f"{name}={value} above maximum {hi}")
        return True

    def check_flag(self, ctx: str, value: Any, name: str) -> None:
        if not isinstance(value, str):
            self.err(
                ctx,
                f"{name} should be a flag string, "
                f"got {type(value).__name__} ({value!r})"
            )
            return
        if not FLAG_RE.match(value):
            self.err(ctx, f"{name}={value!r} contains invalid flag characters")

    def check_stat_map(self, ctx: str, m: Any, name: str) -> None:
        if not isinstance(m, dict):
            self.err(
                ctx,
                f"{name} must be a map with str/int/wis/dex/con keys"
            )
            return
        for k in ("str", "int", "wis", "dex", "con"):
            if k not in m:
                self.err(ctx, f"{name} missing key '{k}'")
            else:
                self.check_int(ctx, m[k], f"{name}.{k}")

    def check_hmv_map(
        self, ctx: str, m: Any, name: str, fields: tuple[str, ...],
    ) -> None:
        if not isinstance(m, dict):
            self.err(ctx, f"{name} must be a mapping with keys: {fields}")
            return
        for k in fields:
            if k not in m:
                self.err(ctx, f"{name} missing key '{k}'")
            else:
                self.check_int(ctx, m[k], f"{name}.{k}", lo=0)

    def check_ac_map(self, ctx: str, m: Any) -> None:
        if not isinstance(m, dict):
            self.err(
                ctx,
                "armor must be a map with pierce/bash/slash/exotic keys"
            )
            return
        for k in ("pierce", "bash", "slash", "exotic"):
            if k not in m:
                self.err(ctx, f"armor missing key '{k}'")
            else:
                if not isinstance(m[k], int):
                    self.err(ctx, f"armor.{k} must be an integer")

    # --- player section ---

    def validate_player(self, pl: dict[str, Any]) -> None:
        ctx = "player"
        name = pl.get("name", "?")
        ctx = f"player '{name}'"
        logger.debug("validating player '%s'", name)

        required = ("name", "race", "sex", "class", "level",
                    "room", "hmv", "perm_stat", "armor", "password")
        for f in required:
            if f not in pl:
                self.err(ctx, f"missing required field '{f}'")

        if "version" in pl:
            v = pl["version"]
            if v not in (3, 4):
                self.warn(ctx, f"version={v} is not 3 or 4")

        if "race" in pl:
            if pl["race"] not in VALID_PC_RACES:
                self.warn(ctx, f"unknown pc race '{pl['race']}'")

        if "sex" in pl:
            if pl["sex"] not in VALID_SEXES:
                self.err(
                    ctx,
                    f"sex={pl['sex']} not valid "
                    "(0=neutral, 1=male, 2=female)"
                )
        if "true_sex" in pl:
            if pl["true_sex"] not in VALID_SEXES:
                self.err(ctx, f"true_sex={pl['true_sex']} not valid")

        if "class" in pl:
            if pl["class"] not in VALID_CLASSES:
                self.err(ctx, f"class={pl['class']} not valid (0-4)")

        if "level" in pl:
            self.check_int(ctx, pl["level"], "level", 0, MAX_LEVEL + 20)

        if "security" in pl:
            if pl["security"] not in VALID_SECURITY:
                self.err(ctx, f"security={pl['security']} not valid (0-9)")

        if "alignment" in pl:
            self.check_int(
                ctx, pl["alignment"], "alignment",
                -MAX_ALIGNMENT, MAX_ALIGNMENT
            )

        if "position" in pl:
            if pl["position"] not in VALID_POSITIONS:
                self.err(ctx, f"position={pl['position']} not valid (0-8)")

        if "invis_level" in pl:
            self.check_int(
                ctx, pl["invis_level"], "invis_level",
                0, MAX_LEVEL + 20
            )
        if "incog_level" in pl:
            self.check_int(
                ctx, pl["incog_level"], "incog_level",
                0, MAX_LEVEL + 20
            )

        if "room" in pl:
            self.check_int(ctx, pl["room"], "room", 1, MAX_VNUM)

        if "hmv" in pl:
            self.check_hmv_map(
                ctx, pl["hmv"], "hmv",
                ("hit", "max_hit", "mana", "max_mana", "move", "max_move")
            )
        if "perm_hmv" in pl:
            self.check_hmv_map(
                ctx, pl["perm_hmv"], "perm_hmv",
                ("hit", "mana", "move")
            )

        if "gold" in pl:
            self.check_int(ctx, pl["gold"], "gold", 0)
        if "bank" in pl:
            self.check_int(ctx, pl["bank"], "bank", 0)
        if "exp" in pl:
            self.check_int(ctx, pl["exp"], "exp", 0)

        for flag_field in (
            "act", "affected_by", "affected2_by", "comm", "wiznet"
        ):
            if flag_field in pl:
                self.check_flag(ctx, str(pl[flag_field]), flag_field)

        if "perm_stat" in pl:
            self.check_stat_map(ctx, pl["perm_stat"], "perm_stat")
        if "mod_stat" in pl:
            self.check_stat_map(ctx, pl["mod_stat"], "mod_stat")

        if "armor" in pl:
            self.check_ac_map(ctx, pl["armor"])

        if "condition" in pl:
            cond = pl["condition"]
            if not isinstance(cond, dict):
                self.err(
                    ctx,
                    "condition must be a map with drunk/full/thirst keys"
                )
            else:
                for k in ("drunk", "full", "thirst"):
                    if k not in cond:
                        self.err(ctx, f"condition missing key '{k}'")
                    else:
                        self.check_int(
                            ctx, cond[k], f"condition.{k}", -1, MAX_COND
                        )

        for int_field in ("points", "last_level", "practice", "train",
                          "saving_throw", "hitroll", "damroll", "wimpy",
                          "quest_points", "quest_next", "bonus_points",
                          "incarnations", "lines", "played"):
            if int_field in pl:
                self.check_int(ctx, pl[int_field], int_field)

        if "recall_room" in pl:
            self.check_int(ctx, pl["recall_room"], "recall_room", 1, MAX_VNUM)

        if "aliases" in pl:
            aliases_raw: Any = pl["aliases"]
            if not isinstance(aliases_raw, list):
                self.err(ctx, "aliases must be a list")
            else:
                aliases = cast(list[Any], aliases_raw)
                if len(aliases) > MAX_ALIAS:
                    self.warn(
                        ctx,
                        f"alias count {len(aliases)} "
                        f"exceeds MAX_ALIAS {MAX_ALIAS}"
                    )
                for i, a in enumerate(aliases):
                    actx = f"{ctx} alias[{i}]"
                    if not isinstance(a, dict):
                        self.err(actx, "each alias must be a mapping")
                    else:
                        a = cast(dict[str, Any], a)
                        for k in ("name", "command"):
                            if k not in a:
                                self.err(actx, f"missing key '{k}'")

        if "boards" in pl:
            for b in cast(list[Any], pl["boards"]):
                if not isinstance(b, dict):
                    self.err(ctx, "each board entry must be a mapping")
                    continue
                b = cast(dict[str, Any], b)
                if "name" not in b:
                    self.err(ctx, "board entry missing 'name'")
                elif b["name"] not in KNOWN_BOARDS:
                    self.warn(ctx, f"unknown board name '{b['name']}'")
                if "last_note" in b:
                    self.check_int(ctx, b["last_note"], "last_note", 0)

        if "skills" in pl:
            for sk in cast(list[Any], pl["skills"]):
                if not isinstance(sk, dict):
                    continue
                sk = cast(dict[str, Any], sk)
                if "level" in sk:
                    self.check_int(f"{ctx} skill '{sk.get('name', '?')}'",
                                   sk["level"], "level", 0, 100)

        if "affects" in pl:
            for i, aff in enumerate(cast(list[Any], pl["affects"])):
                actx = f"{ctx} affect[{i}]"
                if "where" in aff:
                    if aff["where"] not in VALID_AFFECT_WHERE_INT:
                        self.err(actx, f"where={aff['where']} not valid (0-5)")
                if "level" in aff:
                    self.check_int(
                        actx, aff["level"], "level",
                        0, MAX_LEVEL + 20
                    )
                if "duration" in aff:
                    self.check_int(actx, aff["duration"], "duration", -1)
                if "modifier" in aff:
                    if not isinstance(aff["modifier"], int):
                        self.err(actx, "modifier must be an integer")
                if "location" in aff:
                    self.check_int(actx, aff["location"], "location",
                                   0, max(VALID_APPLY_LOCS))
                if "bitvector" in aff:
                    self.check_flag(actx, str(aff["bitvector"]), "bitvector")

    def validate_objects(self, objects: list[Any]) -> None:
        for i, obj in enumerate(objects):
            if not isinstance(obj, dict):
                self.err(f"object[{i}]", "must be a mapping")
                continue
            obj = cast(dict[str, Any], obj)
            ctx = f"object vnum={obj.get('vnum', '?')}"
            if "vnum" in obj:
                self.check_int(ctx, obj["vnum"], "vnum", 1, MAX_VNUM)
            if "nest" in obj:
                self.check_int(ctx, obj["nest"], "nest", 0)
            if "cost" in obj:
                self.check_int(ctx, obj["cost"], "cost", 0)
            if "extra_flags" in obj:
                self.check_flag(ctx, str(obj["extra_flags"]), "extra_flags")
            if "wear_flags" in obj:
                self.check_flag(ctx, str(obj["wear_flags"]), "wear_flags")
            if "values" in obj:
                vals_raw: Any = obj["values"]
                if not isinstance(vals_raw, list):
                    self.err(ctx, "values must be a list")
                else:
                    for j, v in enumerate(cast(list[Any], vals_raw)):
                        if not isinstance(v, int):
                            self.err(
                                ctx,
                                f"values[{j}] must be an integer, got {v!r}"
                            )
            if "affects" in obj:
                for j, aff in enumerate(cast(list[Any], obj["affects"])):
                    actx = f"{ctx} affect[{j}]"
                    if "location" in aff:
                        self.check_int(actx, aff["location"], "location",
                                       0, max(VALID_APPLY_LOCS))
                    if "modifier" in aff:
                        if not isinstance(aff["modifier"], int):
                            self.err(actx, "modifier must be an integer")
                    if "bitvector" in aff:
                        self.check_flag(
                            actx, str(aff["bitvector"]), "bitvector"
                        )

    def validate(self) -> bool:
        logger.debug("validating %s", self.path)
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f)
        except yaml.YAMLError as e:
            self.err("YAML parse", str(e))
            return False
        except OSError as e:
            self.err("file", str(e))
            return False

        if not isinstance(data, dict):
            self.err("top-level", "document must be a mapping")
            return False
        data = cast(dict[str, Any], data)

        pl_raw: Any = data.get("player")
        if pl_raw is None:
            self.err("top-level", "missing 'player:' section")
            return False
        if not isinstance(pl_raw, dict):
            self.err("top-level", "'player:' must be a mapping")
            return False
        pl = cast(dict[str, Any], pl_raw)

        self.validate_player(pl)

        if "objects" in data:
            self.validate_objects(cast(list[Any], data["objects"]))

        return len(self.errors) == 0

    def report(self) -> bool:
        ok = len(self.errors) == 0
        status = "OK" if ok else "FAIL"
        warn_str = (
            f"  ({len(self.warnings)} warning(s))" if self.warnings else ""
        )
        print(f"[{status}] {self.path}{warn_str}")
        for w in self.warnings:
            print(f"  WARN {w}")
        for e in self.errors:
            print(f"  ERR  {e}")
        return ok


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate Broken Shadows player YAML files.")
    parser.add_argument("files", nargs="*", help="YAML files to validate")
    parser.add_argument("--all", metavar="DIR",
                        help="Validate all *.yaml files in DIR")
    parser.add_argument("--debug", action="store_true",
                        help="Enable debug logging")
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="%(levelname)s: %(message)s",
    )

    paths = list(args.files)
    if args.all:
        paths += sorted(str(p) for p in Path(args.all).glob("*.yaml"))

    if not paths:
        parser.print_help()
        sys.exit(0)

    total = 0
    failed = 0
    for p in paths:
        v = Validator(p)
        v.validate()
        if not v.report():
            failed += 1
        total += 1

    print(f"\n{total - failed}/{total} files passed.")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
