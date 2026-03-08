///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
////  YAML player-file support added 2026
///////////////////////////////////////////////////////////////////////

#ifndef YAML_SAVE_H
#define YAML_SAVE_H

/*
 * YAML-based player file loading and saving.
 *
 * Player files may be stored in either the legacy text format (../player/Name)
 * or the new YAML format (../player/Name.yaml).  If both exist for a given
 * character, the .yaml file takes precedence.  Saving always writes a .yaml
 * file; the original text file is left untouched.
 *
 * YAML player file schema: see doc/yaml_player_schema.md
 */

/*
 * Check whether a YAML player file exists for the given character name.
 *
 * If found, the full path is written into yaml_path (caller must supply a
 * buffer of at least MAX_INPUT_LENGTH bytes) and TRUE is returned.
 * Otherwise returns FALSE.
 */
bool yaml_player_file_exists( const char *name, char *yaml_path );

/*
 * Save a character and their inventory/pet to a YAML player file.
 * Equivalent to save_char_obj() but writes YAML.
 */
void save_char_obj_yaml( CHAR_DATA *ch );

/*
 * Load a character and their inventory/pet from a YAML player file.
 * Populates *d->character and returns TRUE if the file existed
 * (returning FALSE means a brand-new character should be created).
 */
bool load_char_obj_yaml( DESCRIPTOR_DATA *d, const char *name );

#endif /* YAML_SAVE_H */
