#!/usr/bin/env python3
# pyright: basic
"""
convert_to_yaml.py  --  Broken Shadows legacy-to-YAML converter
================================================================

Converts legacy .are area files and legacy player save-files to the
YAML format consumed by yaml_area.c and yaml_save.c.

Usage
-----
  python3 convert_to_yaml.py --area  ../area/thalos.are
  python3 convert_to_yaml.py --player ../player/Rahl
  python3 convert_to_yaml.py --all-areas  ../area/
  python3 convert_to_yaml.py --all-players ../player/

Output files are written alongside the input with the extension
replaced by ".yaml".  Existing .yaml files are NOT overwritten unless
--force is given.

YAML schema
-----------
  Area:     { area, mobiles, objects, rooms, resets, shops, specials, helps }
  Player:   { player, objects, pets }

Flag encoding (from db.c flag_convert):
  A=2^0  B=2^1 … Z=2^25   a=2^26  b=2^27 … z=2^51
  Flags may be a sequence of letters or a plain decimal/hex integer.
  The '|' character OR's two flag values.
"""

import argparse
import logging
import os
import re
import sys
from typing import Any
import yaml

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# YAML dump helper – keep multi-line strings readable, numbers as ints
# ---------------------------------------------------------------------------


class _LiteralStr(str):
    pass


def _literal_representer(dumper: Any, data: Any) -> Any:
    if '\n' in data:
        return dumper.represent_scalar(
            'tag:yaml.org,2002:str', data, style='|')
    return dumper.represent_scalar('tag:yaml.org,2002:str', data)


yaml.add_representer(_LiteralStr, _literal_representer)


def _dump(data: Any) -> str:
    return yaml.dump(
        data,
        allow_unicode=True,
        default_flow_style=False,
        sort_keys=False,
    )


def _str(s: Any) -> str:
    """Wrap a string so multi-line values get literal-block style."""
    if s is None:
        return ''
    s = str(s)
    return _LiteralStr(s) if '\n' in s else s


# ---------------------------------------------------------------------------
# Flag conversion helper (mirrors db.c flag_convert / fread_flag)
# ---------------------------------------------------------------------------

def flag_to_int(s: Any) -> int:
    """
    Convert a flag string such as 'ABV', '0', '256', 'A|B' to an integer.
    Letters:  A=2^0, B=2^1, …, Z=2^25, a=2^26, b=2^27, …, z=2^51
    Supports: plain integer strings, sequences of letters, and '|' OR.
    """
    s = str(s).strip()
    if not s or s == '0':
        return 0
    total = 0
    parts = s.split('|')
    for part in parts:
        part = part.strip()
        if not part:
            continue
        # Pure decimal integer?
        if part.lstrip('-').isdigit():
            total += int(part)
            continue
        # Flag letters
        for ch in part:
            if 'A' <= ch <= 'Z':
                total += 1 << (ord(ch) - ord('A'))
            elif 'a' <= ch <= 'z':
                total += 1 << (26 + ord(ch) - ord('a'))
    return total


def int_to_flag(n: int) -> str:
    """
    Convert an integer to a flag letter string
    (mirrors fwrite_flag in olc_save.c).
    Bits 0-25  → 'A'-'Z'   (A=2^0, B=2^1, …, Z=2^25)
    Bits 26+   → 'a'-'f'   (a=2^26, b=2^27, …)
    Zero       → '0'
    """
    if n == 0:
        return '0'
    result = []
    for offset in range(32):
        if n & (1 << offset):
            if offset <= 25:
                result.append(chr(ord('A') + offset))
            else:
                result.append(chr(ord('a') + offset - 26))
    return ''.join(result)


# ---------------------------------------------------------------------------
# Low-level area-file tokeniser
# mirrors fread_letter / fread_number / fread_flag / fread_string / fread_word
# ---------------------------------------------------------------------------

class _Buf:
    """Thin wrapper around a bytes/string buffer with a mutable position."""

    def __init__(self, text: str) -> None:
        self._t = text
        self.pos = 0

    def eof(self) -> bool:
        return self.pos >= len(self._t)

    def peek(self) -> str:
        if self.eof():
            return ''
        return self._t[self.pos]

    def advance(self) -> str:
        c = self._t[self.pos]
        self.pos += 1
        return c

    # ------------------------------------------------------------------
    def skip_ws(self) -> None:
        while not self.eof() and self._t[self.pos] in ' \t\r\n':
            self.pos += 1

    def fread_letter(self) -> str:
        """Skip whitespace, return next single character."""
        self.skip_ws()
        if self.eof():
            return ''
        return self.advance()

    def fread_number(self) -> int:
        """Skip whitespace, read a (possibly signed) decimal integer."""
        self.skip_ws()
        sign = 1
        n = 0
        if self.peek() == '+':
            self.advance()
        elif self.peek() == '-':
            sign = -1
            self.advance()
        if not self.peek().isdigit():
            ctx = self._t[self.pos:self.pos + 10]
            raise ValueError(
                f"fread_number: expected digit at pos {self.pos}, "
                f"got {ctx!r}"
            )
        while not self.eof() and self.peek().isdigit():
            n = n * 10 + int(self.advance())
        if not self.eof() and self.peek() == '|':
            self.advance()
            n += self.fread_number()
        return sign * n

    def fread_flag(self) -> int:
        """
        Skip whitespace, read flag letters or a decimal integer.
        Supports the '|' OR operator (same as db.c).
        """
        self.skip_ws()
        total = 0
        c = self.peek()
        if c in ('+', '-') or c.isdigit():
            return self.fread_number()
        # letters
        while not self.eof() and (self.peek().isalpha()):
            ch = self.advance()
            total += flag_to_int(ch)
        # pipe?
        if not self.eof() and self.peek() == '|':
            self.advance()
            total += self.fread_flag()
        return total

    def fread_string(self) -> str:
        """
        Skip leading whitespace, read until '~'.
        Newlines inside are preserved (as \\n).
        An empty string is just '~'.
        """
        self.skip_ws()
        result: list[str] = []
        while not self.eof():
            c = self.advance()
            if c == '~':
                break
            if c == '\r':
                continue   # strip carriage returns
            result.append(c)
        return ''.join(result)

    def fread_word(self) -> str:
        """Skip whitespace, read until next whitespace."""
        self.skip_ws()
        word: list[str] = []
        while not self.eof() and self.peek() not in ' \t\r\n':
            word.append(self.advance())
        return ''.join(word)

    def fread_to_eol(self) -> None:
        """Consume and discard to end of line."""
        while not self.eof() and self.peek() not in '\r\n':
            self.advance()

    def fread_dice(self) -> dict[str, int]:
        """Read a dice expression NdS+B and return {number, type, bonus}."""
        n = self.fread_number()
        self.fread_letter()   # 'd'
        s = self.fread_number()
        self.fread_letter()   # '+' or '-'
        b = self.fread_number()
        return {'number': n, 'type': s, 'bonus': b}


# ---------------------------------------------------------------------------
#  Area file parser
# ---------------------------------------------------------------------------

def parse_area_file(path: str) -> dict[str, Any]:
    """
    Parse a legacy .are file and return a dict ready for YAML serialisation.
    Keys match what yaml_area.c expects:
      area, mobiles, objects, rooms, resets, shops, specials, helps
    """
    with open(path, 'r', errors='replace') as f:
        text = f.read()

    buf = _Buf(text)
    result: dict[str, Any] = {
        'area': None,
        'mobiles': [],
        'objects': [],
        'rooms': [],
        'resets': [],
        'shops': [],
        'specials': [],
        'helps': [],
    }

    while not buf.eof():
        buf.skip_ws()
        if buf.eof():
            break
        c = buf.fread_letter()
        if c != '#':
            buf.fread_to_eol()
            continue
        word = buf.fread_word()
        if word == '$':
            break
        elif word == 'AREADATA':
            result['area'] = _parse_areadata(buf)
        elif word == 'MOBILES':
            result['mobiles'] = _parse_mobiles(buf)
        elif word == 'OBJECTS':
            result['objects'] = _parse_objects(buf)
        elif word == 'ROOMS':
            result['rooms'] = _parse_rooms(buf)
        elif word == 'RESETS':
            result['resets'] = _parse_resets(buf)
        elif word == 'SHOPS':
            result['shops'] = _parse_shops(buf)
        elif word == 'SPECIALS':
            result['specials'] = _parse_specials(buf)
        elif word == 'HELPS':
            result['helps'] = _parse_helps(buf)
        elif word == 'SOCIALS':
            _skip_section(buf)
        elif word == 'CLANS':
            _skip_section(buf)
        else:
            _skip_section(buf)

    return result


def _skip_section(buf: '_Buf') -> None:
    """Consume until the next top-level '#' (leave the '#' in the buffer)."""
    while not buf.eof():
        buf.skip_ws()
        if buf.peek() == '#':
            return
        buf.fread_to_eol()


def _parse_areadata(buf: '_Buf') -> dict[str, Any]:
    area: dict[str, Any] = {
        'name': '',
        'builders': '',
        'security': 9,
        'area_flags': 0,
        'vnums': [0, 0],
    }
    while not buf.eof():
        buf.skip_ws()
        if buf.eof():
            break
        word = buf.fread_word()
        if word == 'End':
            break
        elif word == 'Name':
            area['name'] = _str(buf.fread_string())
        elif word == 'Builders':
            area['builders'] = _str(buf.fread_string())
        elif word == 'Security':
            area['security'] = buf.fread_number()
        elif word == 'VNUMs':
            area['vnums'] = [buf.fread_number(), buf.fread_number()]
        else:
            buf.fread_to_eol()
    return area


def _parse_mobiles(buf: '_Buf') -> list[Any]:
    mobs: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        c = buf.fread_letter()
        if c != '#':
            continue
        vnum = buf.fread_number()
        if vnum == 0:
            break

        mob: dict[str, Any] = {'vnum': vnum}

        mob['keywords'] = _str(buf.fread_string())
        mob['short_desc'] = _str(buf.fread_string())
        mob['long_desc'] = _str(buf.fread_string())
        mob['description'] = _str(buf.fread_string())
        mob['race'] = buf.fread_string().strip()

        # Line: act aff aff2 alignment clan_letter
        mob['act'] = int_to_flag(buf.fread_flag())
        mob['affected_by'] = int_to_flag(buf.fread_flag())
        mob['affected2_by'] = int_to_flag(buf.fread_flag())
        mob['alignment'] = buf.fread_number()
        clan_letter = buf.fread_letter()
        if clan_letter == 'S':
            mob['clan'] = 0
        else:
            mob['clan'] = max(0, ord(clan_letter) - 64)

        # Line: level hitroll NdT+B NdT+B NdT+B dam_type
        mob['level'] = buf.fread_number()
        mob['hitroll'] = buf.fread_number()
        mob['hit'] = buf.fread_dice()
        mob['mana'] = buf.fread_dice()
        mob['damage'] = buf.fread_dice()
        mob['dam_type'] = buf.fread_number()

        # Line: ac[0..3]  (stored raw; yaml_area.c uses these as /10 values)
        _ac = [buf.fread_number(), buf.fread_number(),
               buf.fread_number(), buf.fread_number()]
        mob['ac'] = {
            'pierce': _ac[0], 'bash': _ac[1],
            'slash': _ac[2], 'exotic': _ac[3]
        }

        # Line: off_flags imm_flags res_flags vuln_flags
        mob['off_flags'] = int_to_flag(buf.fread_flag())
        mob['imm_flags'] = int_to_flag(buf.fread_flag())
        mob['res_flags'] = int_to_flag(buf.fread_flag())
        mob['vuln_flags'] = int_to_flag(buf.fread_flag())

        # Line: start_pos default_pos sex gold
        mob['start_pos'] = buf.fread_number()
        mob['default_pos'] = buf.fread_number()
        mob['sex'] = buf.fread_number()
        mob['gold'] = buf.fread_number()

        # Line: form parts
        mob['form'] = int_to_flag(buf.fread_flag())
        mob['parts'] = int_to_flag(buf.fread_flag())

        # size_letter material
        size_letter = buf.fread_letter()
        size_map = {'T': 'T', 'S': 'S', 'M': 'M', 'L': 'L', 'H': 'H', 'G': 'G'}
        mob['size'] = size_map.get(size_letter.upper(), 'M')
        mob['material'] = buf.fread_word().strip()

        # spec_fun is optional; set blank – populated from #SPECIALS if present
        mob['spec_fun'] = ''

        mobs.append(mob)

    return mobs


def _parse_objects(buf: '_Buf') -> list[Any]:
    objs: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        c = buf.fread_letter()
        if c != '#':
            continue
        vnum = buf.fread_number()
        if vnum == 0:
            break

        obj: dict[str, Any] = {'vnum': vnum}

        obj['name'] = _str(buf.fread_string())
        obj['short_desc'] = _str(buf.fread_string())
        obj['description'] = _str(buf.fread_string())
        obj['material'] = buf.fread_string().strip()

        obj['item_type'] = buf.fread_number()
        obj['extra_flags'] = int_to_flag(buf.fread_flag())
        obj['wear_flags'] = int_to_flag(buf.fread_flag())
        obj['values'] = [buf.fread_flag() for _ in range(5)]
        obj['level'] = buf.fread_number()
        obj['weight'] = buf.fread_number()
        obj['cost'] = buf.fread_number()

        obj['affects'] = []
        obj['extra_descs'] = []

        while not buf.eof():
            buf.skip_ws()
            if buf.peek() == '#':
                break
            letter = buf.fread_letter()
            if letter == 'A':
                # Simple 'A' affect: location modifier
                loc = buf.fread_number()
                mod = buf.fread_number()
                obj['affects'].append({
                    'where': 'O',
                    'location': loc,
                    'modifier': mod,
                    'bitvector': '0',
                })
            elif letter == 'F':
                # Extended 'F' affect: where_letter location modifier bitvector
                where_letter = buf.fread_letter()
                buf.fread_number()  # af_type (not stored)
                loc = buf.fread_number()
                mod = buf.fread_number()
                bv = buf.fread_flag()
                obj['affects'].append({
                    'where': where_letter,
                    'location': loc,
                    'modifier': mod,
                    'bitvector': int_to_flag(bv),
                })
            elif letter == 'E':
                keywords = buf.fread_string()
                desc = _str(buf.fread_string())
                obj['extra_descs'].append({
                    'keywords': keywords,
                    'description': desc,
                })
            else:
                # unknown letter — put it back and break
                buf.pos -= 1
                break

        objs.append(obj)

    return objs


def _parse_rooms(buf: '_Buf') -> list[Any]:
    rooms: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        c = buf.fread_letter()
        if c != '#':
            continue
        vnum = buf.fread_number()
        if vnum == 0:
            break

        room: dict[str, Any] = {'vnum': vnum}
        room['name'] = _str(buf.fread_string())
        room['description'] = _str(buf.fread_string())
        buf.fread_number()  # area number (ignored)
        room['room_flags'] = int_to_flag(buf.fread_flag())
        room['sector'] = buf.fread_number()
        room['owner'] = ''
        room['exits'] = []
        room['extra_descs'] = []

        while not buf.eof():
            buf.skip_ws()
            if buf.peek() == '#':
                break
            letter = buf.fread_letter()
            if letter == 'S':
                break
            elif letter == 'D':
                door = buf.fread_number()
                desc = _str(buf.fread_string())
                keywords = buf.fread_string()
                locks = buf.fread_number()
                key = buf.fread_number()
                to_vnum = buf.fread_number()
                # Translate legacy lock value to rs_flags (mirrors load_rooms)
                lock_map = {
                    0: 0,
                    1: 0x01,                        # EX_ISDOOR
                    2: 0x01 | 0x04,                 # EX_ISDOOR | EX_PICKPROOF
                    3: 0x01 | 0x10 | 0x20,   # EX_ISDOOR|EX_PASSPROOF|EX_HIDDEN
                    4: 0x01 | 0x04 | 0x10 | 0x20,
                    5: 0x01 | 0x10,
                    6: 0x01 | 0x04 | 0x10,
                    7: 0x01 | 0x20,
                    8: 0x01 | 0x04 | 0x20,
                }
                rs_flags = lock_map.get(locks, locks)
                room['exits'].append({
                    'dir': door,
                    'desc': desc,
                    'keywords': keywords,
                    'exit_flags': int_to_flag(rs_flags),
                    'key': key,
                    'to_vnum': to_vnum,
                })
            elif letter == 'E':
                kw = buf.fread_string()
                desc = _str(buf.fread_string())
                room['extra_descs'].append({
                    'keywords': kw,
                    'description': desc,
                })
            elif letter == 'O':
                room['owner'] = buf.fread_string()
            else:
                buf.fread_to_eol()

        rooms.append(room)

    return rooms


def _parse_resets(buf: '_Buf') -> list[Any]:
    resets: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        if buf.peek() == '#':
            break
        letter = buf.fread_letter()
        if letter == 'S':
            break
        if letter == '*':
            buf.fread_to_eol()
            continue
        buf.fread_number()  # if_flag discarded (always 0)
        arg1 = buf.fread_number()
        arg2 = buf.fread_number()
        if letter in ('G', 'R'):
            arg3 = 0
        else:
            arg3 = buf.fread_number()
        buf.fread_to_eol()
        resets.append({
            'command': letter,
            'arg1': arg1,
            'arg2': arg2,
            'arg3': arg3,
        })
    return resets


def _parse_shops(buf: '_Buf') -> list[Any]:
    shops: list[Any] = []
    MAX_TRADE = 5
    while not buf.eof():
        buf.skip_ws()
        if buf.peek() == '#':
            break
        keeper = buf.fread_number()
        if keeper == 0:
            break
        buy_types = [buf.fread_number() for _ in range(MAX_TRADE)]
        profit_buy = buf.fread_number()
        profit_sell = buf.fread_number()
        open_hour = buf.fread_number()
        close_hour = buf.fread_number()
        buf.fread_to_eol()
        shops.append({
            'keeper': keeper,
            'buy_types': buy_types,
            'profit_buy': profit_buy,
            'profit_sell': profit_sell,
            'open_hour': open_hour,
            'close_hour': close_hour,
        })
    return shops


def _parse_specials(buf: '_Buf') -> list[Any]:
    specials: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        if buf.peek() == '#':
            break
        letter = buf.fread_letter()
        if letter == 'S':
            break
        if letter == '*':
            buf.fread_to_eol()
            continue
        if letter == 'M':
            vnum = buf.fread_number()
            spec_fun = buf.fread_word()
            buf.fread_to_eol()
            specials.append({'vnum': vnum, 'spec_fun': spec_fun})
        else:
            buf.fread_to_eol()
    return specials


def _parse_helps(buf: '_Buf') -> list[Any]:
    helps: list[Any] = []
    while not buf.eof():
        buf.skip_ws()
        if buf.peek() == '#':
            break
        try:
            level = buf.fread_number()
        except (ValueError, IndexError):
            break
        keyword = buf.fread_string().strip()
        if keyword.startswith('$'):
            break
        text = _str(buf.fread_string())
        helps.append({
            'level': level,
            'keyword': keyword,
            'text': text,
        })
    return helps


# ---------------------------------------------------------------------------
#  Post-process: wire spec_fun from the specials list into mobs
# ---------------------------------------------------------------------------

def _apply_specials_to_mobs(data: dict[str, Any]) -> None:
    """Set mob['spec_fun'] based on data['specials'] vnum lookup."""
    mob_by_vnum = {m['vnum']: m for m in data.get('mobiles', [])}
    for sp in data.get('specials', []):
        mob = mob_by_vnum.get(sp['vnum'])
        if mob:
            mob['spec_fun'] = sp['spec_fun']


# ---------------------------------------------------------------------------
#  Player file parser
# ---------------------------------------------------------------------------

def parse_player_file(path: str) -> dict[str, Any]:
    """
    Parse a legacy player save-file and return a dict for YAML output.
    Keys match what yaml_save.c expects under 'player:', 'objects:', 'pets:'.
    """
    with open(path, 'r', errors='replace') as f:
        lines = f.readlines()

    result: dict[str, Any] = {'player': {}, 'objects': [], 'pets': []}
    i = 0

    while i < len(lines):
        line = lines[i].rstrip('\r\n')
        i += 1
        stripped = line.strip()

        if stripped == '#PLAYER':
            pl, i = _parse_player_section(lines, i)
            result['player'] = pl
        elif stripped == '#O':
            obj, i = _parse_obj_section(lines, i)
            result['objects'].append(obj)
        elif stripped == '#PET':
            pet, i = _parse_pet_section(lines, i)
            result['pets'].append(pet)
        elif stripped == '#END':
            break

    return result


def _read_tilde_word(s: str) -> str:
    """Return the part of s before the first '~', stripped."""
    idx = s.find('~')
    if idx >= 0:
        return s[:idx].strip()
    return s.strip()


def _pop_key(line: str) -> tuple[str, str]:
    """Split 'Key rest' into (key, rest)."""
    parts = line.strip().split(None, 1)
    if len(parts) == 0:
        return '', ''
    if len(parts) == 1:
        return parts[0], ''
    return parts[0], parts[1]


def _parse_player_section(
        lines: list[str], i: int) -> tuple[dict[str, Any], int]:
    """Parse from current position until 'End'. Returns (dict, new_i)."""
    pl: dict[str, Any] = {}
    while i < len(lines):
        raw = lines[i].rstrip('\r\n')
        i += 1
        key, rest = _pop_key(raw)
        if not key:
            continue
        if key == 'End':
            break

        # ------------------------------------------------------------------
        # Map legacy keys → YAML keys used by yaml_save.c
        # ------------------------------------------------------------------
        if key == 'Name':
            pl['name'] = _read_tilde_word(rest)
        elif key == 'Vers':
            pl['version'] = int(rest.strip())
        elif key == 'Race':
            pl['race'] = _read_tilde_word(rest)
        elif key == 'Sex':
            pl['sex'] = int(rest.strip())
        elif key == 'Cla':
            pl['class'] = int(rest.strip())
        elif key == 'Levl':
            pl['level'] = int(rest.strip())
        elif key == 'Trust':
            pl['trust'] = int(rest.strip())
        elif key == 'Sec':
            pl['security'] = int(rest.strip())
        elif key == 'Log':
            pl['logon'] = int(rest.strip())
        elif key == 'Plyd':
            pl['played'] = int(rest.strip())
        elif key == 'Note':
            pl['last_note'] = int(rest.strip())
        elif key == 'Scro':
            pl['lines'] = int(rest.strip())
        elif key == 'Room':
            pl['room'] = int(rest.strip())
        elif key == 'HMV':
            n = list(map(int, rest.strip().split()))
            pl['hmv'] = {
                'hit': n[0], 'max_hit': n[1], 'mana': n[2],
                'max_mana': n[3], 'move': n[4], 'max_move': n[5]
            }
        elif key == 'Gold':
            pl['gold'] = int(rest.strip())
        elif key == 'Bank':
            pl['bank'] = int(rest.strip())
        elif key == 'Bounty':
            pl['bounty'] = int(rest.strip())
        elif key == 'Pkills':
            pl['pkills'] = int(rest.strip())
        elif key == 'Pkilled':
            pl['pkilled'] = int(rest.strip())
        elif key == 'Killed':
            pl['killed'] = int(rest.strip())
        elif key == 'Exp':
            pl['exp'] = int(rest.strip())
        elif key == 'ActF':
            pl['act'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'AfByF':
            pl['affected_by'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'AfBy2':
            pl['affected2_by'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'CommF':
            pl['comm'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'WiznF':
            pl['wiznet'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'Invi':
            pl['invis_level'] = int(rest.strip())
        elif key == 'Incog':
            pl['incog_level'] = int(rest.strip())
        elif key == 'Pos':
            pl['position'] = int(rest.strip())
        elif key == 'Prac':
            pl['practice'] = int(rest.strip())
        elif key == 'Trai':
            pl['train'] = int(rest.strip())
        elif key == 'Save':
            pl['saving_throw'] = int(rest.strip())
        elif key == 'Alig':
            pl['alignment'] = int(rest.strip())
        elif key == 'Hit':
            pl['hitroll'] = int(rest.strip())
        elif key == 'Dam':
            pl['damroll'] = int(rest.strip())
        elif key == 'Wimpy':
            pl['wimpy'] = int(rest.strip())
        elif key == 'ACs':
            n = list(map(int, rest.strip().split()))
            pl['armor'] = {
                'pierce': n[0], 'bash': n[1],
                'slash': n[2], 'exotic': n[3]
            }
        elif key == 'Attr':
            n = list(map(int, rest.strip().split()))
            pl['perm_stat'] = {
                'str': n[0], 'int': n[1], 'wis': n[2],
                'dex': n[3], 'con': n[4]
            }
        elif key == 'AMod':
            n = list(map(int, rest.strip().split()))
            pl['mod_stat'] = {
                'str': n[0], 'int': n[1], 'wis': n[2],
                'dex': n[3], 'con': n[4]
            }
        elif key == 'Pass':
            pl['password'] = _read_tilde_word(rest)
        elif key == 'Titl':
            pl['title'] = _read_tilde_word(rest)
        elif key == 'Pnts':
            pl['points'] = int(rest.strip())
        elif key == 'TSex':
            pl['true_sex'] = int(rest.strip())
        elif key == 'LLev':
            pl['last_level'] = int(rest.strip())
        elif key == 'HMVP':
            n = list(map(int, rest.strip().split()))
            pl['perm_hmv'] = {'hit': n[0], 'mana': n[1], 'move': n[2]}
        elif key == 'Cond':
            n = list(map(int, rest.strip().split()))
            pl['condition'] = {'drunk': n[0], 'full': n[1], 'thirst': n[2]}
        elif key == 'Prom':
            pl['prompt'] = _read_tilde_word(rest)
        elif key == 'Clan':
            pl['clan'] = _read_tilde_word(rest)
        elif key == 'ClanLeader':
            pl['clan_leader'] = int(rest.strip())
        elif key == 'QuestPnts':
            pl['quest_points'] = int(rest.strip())
        elif key == 'QuestNext':
            pl['quest_next'] = int(rest.strip())
        elif key == 'BonusPts':
            pl['bonus_points'] = int(rest.strip())
        elif key == 'Recl':
            pl['recall_room'] = int(rest.strip())
        elif key == 'Email':
            pl['email'] = _read_tilde_word(rest)
        elif key == 'Comnt':
            pl['comment'] = _read_tilde_word(rest)
        elif key == 'Incr':
            pl['incarnations'] = int(rest.strip())
        elif key == 'Bamfi':
            pl['bamfin'] = _read_tilde_word(rest)
        elif key == 'Bamfo':
            pl['bamfout'] = _read_tilde_word(rest)
        elif key == 'Spouse':
            pl['spouse'] = _read_tilde_word(rest)
        elif key == 'Boards':
            pl['boards'] = _parse_boards_line(rest)
        elif key == 'Sk':
            if 'skills' not in pl:
                pl['skills'] = []
            # Format: level 'skill name'
            m = re.match(r'(\d+)\s+\'([^\']+)\'', rest)
            if m:
                pl['skills'].append({
                    'name':  m.group(2).strip(),
                    'level': int(m.group(1)),
                })
        elif key == 'Gr':
            if 'groups' not in pl:
                pl['groups'] = []
            # Format: 'group name'    or   group name~
            m = re.match(r"'([^']+)'", rest.strip())
            if m:
                pl['groups'].append(m.group(1).strip())
            else:
                pl['groups'].append(_read_tilde_word(rest))
        elif key == 'AffD':
            if 'affects' not in pl:
                pl['affects'] = []
            aff = _parse_affd_line(rest)
            if aff:
                pl['affects'].append(aff)
        elif key == 'Alias':
            if 'aliases' not in pl:
                pl['aliases'] = []
            # Format: alias~ expansion~
            parts = rest.split('~')
            if len(parts) >= 2:
                pl['aliases'].append({
                    'alias':     parts[0].strip(),
                    'expansion': parts[1].strip(),
                })
        # intentionally ignoring: 'Titl2', old fields, etc.

    return pl, i


def _parse_boards_line(rest: str) -> list[Any]:
    """
    Parse: N name1 time1 name2 time2 ...
    Returns list of {name, last_note} dicts.
    """
    tokens = rest.strip().split()
    boards: list[Any] = []
    try:
        count = int(tokens[0])
        idx = 1
        for _ in range(count):
            name = tokens[idx]
            note = int(tokens[idx + 1])
            boards.append({'name': name, 'last_note': note})
            idx += 2
    except (IndexError, ValueError):
        pass
    return boards


def _parse_affd_line(rest: str) -> 'dict[str, Any] | None':
    """
    Parse: 'skill name'  where level duration modifier location bitvector
    Returns a dict matching write_affect() keys.
    """
    m = re.match(
        r"'([^']+)'\s+([-\d]+)\s+([-\d]+)\s+([-\d]+)"
        r"\s+([-\d]+)\s+([-\d]+)\s+([-\d]+)",
        rest.strip()
    )
    if m:
        return {
            'name': m.group(1).strip(),
            'where': int(m.group(2)),
            'level': int(m.group(3)),
            'duration': int(m.group(4)),
            'modifier': int(m.group(5)),
            'location': int(m.group(6)),
            'bitvector': int_to_flag(int(m.group(7))),
        }
    return None


def _parse_obj_section(
        lines: list[str], i: int) -> tuple[dict[str, Any], int]:
    """Parse #O section lines. Returns (dict, new_i)."""
    obj: dict[str, Any] = {}
    while i < len(lines):
        raw = lines[i].rstrip('\r\n')
        i += 1
        key, rest = _pop_key(raw)
        if not key:
            continue
        if key in ('#O', '#PET', '#END', 'End'):
            i -= 1   # put back
            break

        if key == 'Nest':
            obj['nest'] = int(rest.strip())
        elif key == 'Vnum':
            obj['vnum'] = int(rest.strip())
        elif key == 'Worn':
            obj['wear_loc'] = int(rest.strip())
        elif key == 'Enchanted':
            obj['enchanted'] = int(rest.strip())
        elif key == 'Name':
            obj['name'] = _read_tilde_word(rest)
        elif key == 'ShD':
            obj['short_desc'] = _read_tilde_word(rest)
        elif key == 'Desc':
            obj['description'] = _read_tilde_word(rest)
        elif key == 'ExtraF':
            obj['extra_flags'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'WearF':
            obj['wear_flags'] = int_to_flag(flag_to_int(rest.strip()))
        elif key == 'ItemType':
            obj['item_type'] = int(rest.strip())
        elif key == 'Weight':
            obj['weight'] = int(rest.strip())
        elif key == 'Level':
            obj['level'] = int(rest.strip())
        elif key == 'Timer':
            obj['timer'] = int(rest.strip())
        elif key == 'Cost':
            obj['cost'] = int(rest.strip())
        elif key == 'Val':
            obj['values'] = list(map(int, rest.strip().split()))
        elif key == 'Spell':
            # Spell_1/Spell_2/Spell_3 extra fields
            m = re.match(r'(\d+)\s+(.*)', rest.strip())
            if m:
                idx_s = int(m.group(1))
                name_s = m.group(2).strip()
                spell_key = f'spell_{idx_s}'
                obj[spell_key] = name_s
        elif key == 'AffD':
            if 'affects' not in obj:
                obj['affects'] = []
            aff = _parse_affd_line(rest)
            if aff:
                obj['affects'].append(aff)
        elif key == 'ExDesc':
            if 'extra_descs' not in obj:
                obj['extra_descs'] = []
            # Format: keyword~ description~
            parts = rest.split('~')
            obj['extra_descs'].append({
                'keywords':    parts[0].strip() if len(parts) > 0 else '',
                'description': parts[1].strip() if len(parts) > 1 else '',
            })
        elif key == 'End':
            break

    return obj, i


def _parse_pet_section(
        lines: list[str], i: int) -> tuple[dict[str, Any], int]:
    """Parse #PET section lines. Returns (dict, new_i)."""
    pet: dict[str, Any] = {}
    while i < len(lines):
        raw = lines[i].rstrip('\r\n')
        i += 1
        key, rest = _pop_key(raw)
        if not key:
            continue

        if key == 'Vnum':
            pet['vnum'] = int(rest.strip())
        elif key == 'Name':
            pet['name'] = _read_tilde_word(rest)
        elif key == 'ShD':
            pet['short_desc'] = _read_tilde_word(rest)
        elif key == 'LnD':
            pet['long_desc'] = _read_tilde_word(rest)
        elif key == 'Levl':
            pet['level'] = int(rest.strip())
        elif key == 'Sex':
            pet['sex'] = int(rest.strip())
        elif key == 'AffD':
            if 'affects' not in pet:
                pet['affects'] = []
            aff = _parse_affd_line(rest)
            if aff:
                pet['affects'].append(aff)
        elif key == 'End':
            break

    return pet, i


# ---------------------------------------------------------------------------
#  Output helpers
# ---------------------------------------------------------------------------

def write_yaml(path: str, data: Any) -> None:
    """Write data as YAML to path."""
    with open(path, 'w') as f:
        f.write(_dump(data))
    logger.info("wrote %s", path)


def convert_area(src: str, out_path: str, force: bool = False) -> None:
    if os.path.exists(out_path) and not force:
        logger.info("skip (exists): %s", out_path)
        return
    logger.info("parsing %s", src)
    data = parse_area_file(src)
    _apply_specials_to_mobs(data)
    # Remove empty top-level sections to keep YAML tidy
    empty_keys = (
        'mobiles', 'objects', 'rooms', 'resets', 'shops', 'specials', 'helps'
    )
    for key in empty_keys:
        if data[key] == []:
            del data[key]
    write_yaml(out_path, data)


def convert_player(src: str, out_path: str, force: bool = False) -> None:
    if os.path.exists(out_path) and not force:
        logger.info("skip (exists): %s", out_path)
        return
    logger.info("parsing %s", src)
    data = parse_player_file(src)
    write_yaml(out_path, data)


# ---------------------------------------------------------------------------
#  CLI
# ---------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(
        description=(
            'Convert Broken Shadows legacy .are / player files to YAML.'
        ))
    ap.add_argument('--area', metavar='FILE',
                    help='Convert one .are file')
    ap.add_argument('--player', metavar='FILE',
                    help='Convert one player file')
    ap.add_argument('--all-areas', metavar='DIR',
                    help='Convert all .are files in DIR')
    ap.add_argument('--all-players', metavar='DIR',
                    help='Convert all player files in DIR')
    ap.add_argument('--force', action='store_true',
                    help='Overwrite existing .yaml files')
    ap.add_argument('--outdir', metavar='DIR',
                    help='Write output to DIR instead of alongside source')
    ap.add_argument('--debug', action='store_true',
                    help='Enable debug logging')
    args = ap.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format='%(levelname)s: %(message)s',
    )

    if not any([args.area, args.player, args.all_areas, args.all_players]):
        ap.print_help()
        sys.exit(1)

    def _out(src: str, new_ext: str) -> str:
        base = os.path.splitext(src)[0]
        name = os.path.basename(base) + new_ext
        if args.outdir:
            return os.path.join(args.outdir, name)
        return base + new_ext

    if args.area:
        convert_area(args.area, _out(args.area, '.yaml'), args.force)

    if args.player:
        src = args.player
        if args.outdir:
            out = os.path.join(
                args.outdir, os.path.basename(src) + '.yaml'
            )
        else:
            out = src + '.yaml'
        convert_player(src, out, args.force)

    if args.all_areas:
        d = args.all_areas
        for fn in sorted(os.listdir(d)):
            if fn.endswith('.are'):
                src = os.path.join(d, fn)
                convert_area(src, _out(src, '.yaml'), args.force)

    if args.all_players:
        d = args.all_players
        for fn in sorted(os.listdir(d)):
            full = os.path.join(d, fn)
            skip = fn.endswith('.yaml') or fn.endswith('.bak')
            if os.path.isfile(full) and not skip:
                if args.outdir:
                    out = os.path.join(args.outdir, fn + '.yaml')
                else:
                    out = full + '.yaml'
                convert_player(full, out, args.force)


if __name__ == '__main__':
    main()
