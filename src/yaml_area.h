///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
////  YAML area file support added 2026
///////////////////////////////////////////////////////////////////////

#ifndef YAML_AREA_H
#define YAML_AREA_H

/*
 * YAML-based area file loading and saving.
 *
 * Area files may be stored in either the legacy .are format or the new
 * .yaml format.  If both exist for a given area, the .yaml file takes
 * precedence.  Saving always writes a .yaml file; the original .are file
 * is left untouched.
 *
 * YAML area file schema: see doc/yaml_area_schema.md
 */

/*
 * Given an .are filename (e.g. "midgaard.are"), check whether a sibling
 * .yaml file exists (e.g. "midgaard.yaml").
 *
 * If found, the full yaml path is written into yaml_filename (caller must
 * supply a buffer of at least MAX_INPUT_LENGTH bytes) and TRUE is returned.
 * Otherwise returns FALSE and yaml_filename is left unchanged.
 */
bool yaml_area_file_exists( const char *are_filename, char *yaml_filename );

/*
 * Load a YAML area file and populate the standard game data structures
 * (area_first/area_last, room_index_hash, mob_index_hash, obj_index_hash,
 * shop_first/shop_last, …) exactly as the legacy .are loader would.
 *
 * Returns TRUE on success, FALSE on parse/IO error (in which case a bug()
 * message is logged and the caller should abort boot).
 */
bool load_yaml_area_file( const char *filename );

/*
 * Pass 2 loader: load resets and shops from a YAML area file.
 * Must be called after all areas have completed their pass 1 load so that
 * rooms referenced by reset commands are already in the hash tables.
 *
 * Returns TRUE on success.
 */
bool load_yaml_area_resets_shops( const char *filename );

/*
 * Serialise all data belonging to pArea into a YAML file whose name is
 * derived from pArea->filename by replacing the ".are" extension with
 * ".yaml".
 *
 * Returns TRUE on success.
 */
bool save_yaml_area( AREA_DATA *pArea );

/*
 * Load clans from a YAML file (replaces load_clans for clans.yaml).
 * Returns TRUE on success.
 */
bool load_clans_yaml( const char *filename );

/*
 * Save all clans to clans.yaml in the current area directory.
 * Called alongside save_clans() to keep the YAML file in sync.
 * Returns TRUE on success.
 */
bool save_clans_yaml( void );

#endif /* YAML_AREA_H */
