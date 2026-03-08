#!/usr/bin/env python3
"""
Validate Broken Shadows area YAML files for syntax and value correctness.

Usage:
    python3 tools/validate_area.py area/thalos.yaml [area/midgaard.yaml ...]
    python3 tools/validate_area.py --all area/
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

VALID_RACES = {
    "unique", "Dwarf", "Elf", "Giant", "Hobbit", "Human", "Troll", "Drow",
    "Gnome", "Half-Elf", "Wyvern", "Vampire", "Wolf", "bat", "bear", "cat",
    "centipede", "dog", "doll", "fido", "fox", "goblin", "hobgoblin", "kobold",
    "lizard", "modron", "orc", "pig", "rabbit", "sailor", "school monster",
    "shiriff", "snake", "song bird", "thain", "water fowl",
}

# tiny/small/medium/large/huge/giant
VALID_SIZES = {"T", "S", "M", "L", "H", "G"}

VALID_SEXES = {0, 1, 2, 3}    # SEX_NEUTRAL, SEX_MALE, SEX_FEMALE, 3=random

VALID_POSITIONS = {0, 1, 2, 3, 4, 5, 6, 7, 8}  # POS_DEAD .. POS_STANDING

VALID_SECTORS = set(range(12))  # SECT_INSIDE(0)..SECT_UNDERGROUND(11)

VALID_DIRS = {0, 1, 2, 3, 4, 5}  # N/E/S/W/U/D

MAX_LEVEL = 100
MAX_VNUM = 65535
MAX_ALIGNMENT = 1000

# Valid APPLY_* locations (0-26)
VALID_APPLY_LOCS = set(range(27))

# Valid item types from merc.h
VALID_ITEM_TYPES = {
    1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 15, 17, 18, 19, 20,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
}

# Valid affect 'where' codes from olc_save / merc.h
# (A=object, F=immune/res bits)
VALID_AFFECT_WHERE = {"A", "F", "I", "O", "R", "V"}

# Flag string: only these characters are valid (uppercase A-Z = bits 0-25,
# lowercase a-f = bits 26-31, '0' = no flags)
FLAG_RE = re.compile(r'^(?:0|[A-Za-f]+)$')


# ---------------------------------------------------------------------------
# Helpers
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
                f"{name} must be an integer, got {type(value).__name__}"
            )
            return False
        if lo is not None and value < lo:
            self.err(ctx, f"{name}={value} below minimum {lo}")
        if hi is not None and value > hi:
            self.err(ctx, f"{name}={value} above maximum {hi}")
        return True

    def check_flag(self, ctx: str, value: Any, name: str) -> None:
        """Flag string: '0' or letters A-Z/a-f within valid range."""
        if not isinstance(value, str):
            self.err(
                ctx,
                f"{name} should be a flag string, "
                f"got {type(value).__name__} ({value!r})"
            )
            return
        if not FLAG_RE.match(value):
            self.err(ctx, f"{name}={value!r} contains invalid flag characters")

    def check_dice(self, ctx: str, dice: Any, name: str) -> None:
        if not isinstance(dice, dict):
            self.err(ctx, f"{name} must be a map with number/type/bonus keys")
            return
        for k in ("number", "type", "bonus"):
            if k not in dice:
                self.err(ctx, f"{name} missing key '{k}'")
            else:
                self.check_int(ctx, dice[k], f"{name}.{k}", lo=0)

    def check_ac(self, ctx: str, ac: Any) -> None:
        if not isinstance(ac, dict):
            self.err(
                ctx,
                "ac must be a map with pierce/bash/slash/exotic keys"
            )
            return
        for k in ("pierce", "bash", "slash", "exotic"):
            if k not in ac:
                self.err(ctx, f"ac missing key '{k}'")
            elif not isinstance(ac[k], int):
                self.err(ctx, f"ac.{k} must be an integer")

    # --- section validators ---

    def validate_area_header(self, area: Any) -> None:
        ctx = "area header"
        if not isinstance(area, dict):
            self.err(ctx, "area: must be a mapping")
            return
        area = cast(dict[str, Any], area)
        for f in ("name", "builders"):
            if f not in area:
                self.err(ctx, f"missing field '{f}'")
        if "security" in area:
            self.check_int(ctx, area["security"], "security", 0, 9)
        if "vnums" in area:
            vnums_raw: Any = area["vnums"]
            if not isinstance(vnums_raw, list) or len(cast(list[Any],
                                                           vnums_raw)) != 2:
                self.err(ctx, "vnums must be a 2-element list [lo, hi]")
            else:
                vnums = cast(list[Any], vnums_raw)
                lo, hi = vnums[0], vnums[1]
                self.check_int(ctx, lo, "vnum_lo", 0, MAX_VNUM)
                self.check_int(ctx, hi, "vnum_hi", 0, MAX_VNUM)
                if isinstance(lo, int) and isinstance(hi, int) and lo > hi:
                    self.err(ctx, f"vnum lo ({lo}) > hi ({hi})")

    def validate_mob(
        self, mob: dict[str, Any],
        vnum_lo: int = 0, vnum_hi: int = MAX_VNUM,
    ) -> None:
        vnum = mob.get("vnum", "?")
        ctx = f"mob {vnum}"
        logger.debug("  mob vnum=%s", vnum)

        self.check_int(ctx, vnum, "vnum", 0, MAX_VNUM)

        for f in ("keywords", "short_desc", "long_desc"):
            if f not in mob:
                self.err(ctx, f"missing required field '{f}'")

        if "race" in mob and mob["race"] not in VALID_RACES:
            self.warn(ctx, f"unknown race '{mob['race']}'")

        for flag_field in (
            "act", "affected_by", "affected2_by", "off_flags",
            "imm_flags", "res_flags", "vuln_flags", "form", "parts"
        ):
            if flag_field in mob:
                self.check_flag(ctx, str(mob[flag_field]), flag_field)

        if "alignment" in mob:
            self.check_int(
                ctx, mob["alignment"], "alignment",
                -MAX_ALIGNMENT, MAX_ALIGNMENT
            )
        if "level" in mob:
            self.check_int(ctx, mob["level"], "level", 0, MAX_LEVEL + 20)
        if "sex" in mob:
            if mob["sex"] not in VALID_SEXES:
                self.err(ctx, f"sex={mob['sex']} not in {VALID_SEXES}")
        if "size" in mob and mob["size"] not in VALID_SIZES:
            self.err(ctx, f"size='{mob['size']}' not in {VALID_SIZES}")

        for dice_field in ("hit", "mana", "damage"):
            if dice_field in mob:
                self.check_dice(ctx, mob[dice_field], dice_field)

        if "ac" in mob:
            self.check_ac(ctx, mob["ac"])

        for pos_field in ("start_pos", "default_pos"):
            if pos_field in mob:
                if mob[pos_field] not in VALID_POSITIONS:
                    self.err(
                        ctx,
                        f"{pos_field}={mob[pos_field]}"
                        " not a valid position (0-8)"
                    )

        if "gold" in mob:
            self.check_int(ctx, mob["gold"], "gold", 0)
        if "hitroll" in mob:
            self.check_int(ctx, mob["hitroll"], "hitroll", -1000, 1000)
        if "dam_type" in mob:
            self.check_int(ctx, mob["dam_type"], "dam_type", 0)

    def validate_object(
        self, obj: dict[str, Any],
        vnum_lo: int = 0, vnum_hi: int = MAX_VNUM,
    ) -> None:
        vnum = obj.get("vnum", "?")
        ctx = f"object {vnum}"
        logger.debug("  object vnum=%s", vnum)

        self.check_int(ctx, vnum, "vnum", 0, MAX_VNUM)

        for f in ("name", "short_desc"):
            if f not in obj:
                self.err(ctx, f"missing required field '{f}'")

        if "item_type" in obj:
            if obj["item_type"] not in VALID_ITEM_TYPES:
                self.warn(
                    ctx,
                    f"item_type={obj['item_type']} not a known item type"
                )

        for flag_field in ("extra_flags", "wear_flags"):
            if flag_field in obj:
                self.check_flag(ctx, str(obj[flag_field]), flag_field)

        if "level" in obj:
            self.check_int(ctx, obj["level"], "level", 0, 1000)
        if "weight" in obj:
            self.check_int(ctx, obj["weight"], "weight", 0)
        if "cost" in obj:
            self.check_int(ctx, obj["cost"], "cost")

        if "values" in obj:
            vals_raw: Any = obj["values"]
            if not isinstance(vals_raw, list):
                self.err(ctx, "values must be a list")
            else:
                for i, v in enumerate(cast(list[Any], vals_raw)):
                    if not isinstance(v, int):
                        self.err(
                            ctx,
                            f"values[{i}] must be an integer, got {v!r}"
                        )

        if "affects" in obj:
            for i, aff in enumerate(obj["affects"]):
                actx = f"{ctx} affect[{i}]"
                if "where" in aff:
                    if aff["where"] not in VALID_AFFECT_WHERE:
                        self.err(
                            actx,
                            f"where='{aff['where']}' not valid (A/F/I/O/R/V)"
                        )
                if "location" in aff:
                    self.check_int(
                        actx, aff["location"], "location",
                        min(VALID_APPLY_LOCS), max(VALID_APPLY_LOCS)
                    )
                if "modifier" in aff:
                    if not isinstance(aff["modifier"], int):
                        self.err(actx, "modifier must be an integer")
                if "bitvector" in aff:
                    self.check_flag(actx, str(aff["bitvector"]), "bitvector")

    def validate_room(
        self, room: dict[str, Any],
        vnum_lo: int = 0, vnum_hi: int = MAX_VNUM,
    ) -> None:
        vnum = room.get("vnum", "?")
        ctx = f"room {vnum}"
        logger.debug("  room vnum=%s", vnum)

        self.check_int(ctx, vnum, "vnum", 0, MAX_VNUM)

        if "name" not in room:
            self.err(ctx, "missing required field 'name'")

        if "room_flags" in room:
            self.check_flag(ctx, str(room["room_flags"]), "room_flags")

        if "sector" in room:
            if room["sector"] not in VALID_SECTORS:
                self.err(ctx, f"sector={room['sector']} not valid (0-11)")

        if "exits" in room:
            for exit_ in room["exits"]:
                ectx = f"{ctx} exit"
                if "dir" in exit_:
                    ectx = f"{ctx} exit[dir={exit_['dir']}]"
                    if exit_["dir"] not in VALID_DIRS:
                        self.err(ectx, f"dir={exit_['dir']} not valid (0-5)")
                if "exit_flags" in exit_:
                    self.check_flag(
                        ectx, str(exit_["exit_flags"]), "exit_flags"
                    )
                if "to_vnum" in exit_:
                    self.check_int(
                        ectx, exit_["to_vnum"], "to_vnum", 0, MAX_VNUM
                    )
                if "key" in exit_:
                    self.check_int(ectx, exit_["key"], "key", -1, MAX_VNUM)

    def validate(self) -> bool:
        logger.debug("validating %s", self.path)
        # --- YAML parse ---
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

        # --- area header ---
        area_hdr: Any = None
        if "area" not in data:
            self.err("top-level", "missing 'area:' section")
        else:
            area_hdr = data["area"]
            if area_hdr is not None:
                self.validate_area_header(area_hdr)

        # determine vnum bounds for cross-checks
        vnum_lo: int = 0
        vnum_hi: int = MAX_VNUM
        if isinstance(area_hdr, dict) and "vnums" in area_hdr:
            _hdr = cast(dict[str, Any], area_hdr)
            vn_raw: Any = _hdr["vnums"]
            if isinstance(vn_raw, list) and len(cast(list[Any], vn_raw)) == 2:
                vn = cast(list[Any], vn_raw)
                vnum_lo, vnum_hi = int(vn[0]), int(vn[1])

        # --- mobiles ---
        for mob in cast(list[Any], data.get("mobiles", [])):
            self.validate_mob(mob, vnum_lo, vnum_hi)

        # --- objects ---
        for obj in cast(list[Any], data.get("objects", [])):
            self.validate_object(obj, vnum_lo, vnum_hi)

        # --- rooms ---
        for room in cast(list[Any], data.get("rooms", [])):
            self.validate_room(room, vnum_lo, vnum_hi)

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
        description="Validate Broken Shadows area YAML files.")
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
