import json
import re
import sys
import os


# ============================================================
# Utility: Load JSON that may contain // comments (Tiled-style)
# ============================================================
def load_json_with_comments(path: str) -> dict:
    """
    Loads a JSON file that may contain JavaScript-style comments (//).

    This is common in Tiled map-related workflow files, but invalid in strict JSON.
    We strip comments before parsing.
    """
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # Remove // comments (simple but effective for config-like files)
    content = re.sub(r"//.*", "", content)

    return json.loads(content)


# ============================================================
# Configuration
# ============================================================

LOOKUP_FILE = "lookup.json"

# IMPORTANT RULE:
# Your mapping requires all input tile IDs to be shifted by +1
# before lookup is applied.
ADD_PLUS_ONE_TO_INPUT = True


# ============================================================
# Load lookup table
# ============================================================
def load_lookup_table(path: str) -> dict:
    """
    Loads and normalizes the lookup table.

    JSON keys are strings, so we convert them to integers.
    """
    raw = load_json_with_comments(path)
    return {int(k): int(v) for k, v in raw.items()}


lookup_table = load_lookup_table(LOOKUP_FILE)


# ============================================================
# Tile remapping logic
# ============================================================
# Keep track of missing IDs so we don't spam duplicates
missing_ids = set()
def remap_tile(tile_id: int) -> int:
    """
    Remaps a tile ID using lookup table.
    Warns if the ID is not found.
    """

    adjusted_id = tile_id + 1 if ADD_PLUS_ONE_TO_INPUT else tile_id

    if adjusted_id in lookup_table:
        return lookup_table[adjusted_id]

    # Warn once per missing ID
    if adjusted_id not in missing_ids:
        print(f"⚠ Warning: No mapping found for tile ID {adjusted_id}")
        missing_ids.add(adjusted_id)

    return tile_id

# ============================================================
# Process a full Tiled map file
# ============================================================
def process_map(input_path: str) -> None:
    """
    Loads a Tiled JSON map, remaps all tile layers, and saves output.

    Output file is automatically generated as:
        <original_name>_updated.json
    """

    map_data = load_json_with_comments(input_path)

    # Iterate through all layers in the map
    for layer in map_data.get("layers", []):
        # Only process tile layers
        if layer.get("type") != "tilelayer":
            continue

        # Remap all tile IDs in the layer
        original_data = layer.get("data", [])
        layer["data"] = [remap_tile(tile) for tile in original_data]

    # Generate output filename
    base, ext = os.path.splitext(input_path)
    output_path = f"{base}_updated{ext}"

    # Write updated map
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(map_data, f, indent=2)

    print(f"✔ Map successfully processed: {output_path}")


# ============================================================
# CLI entry point
# ============================================================
def main():
    """
    Command-line interface entry point.

    Usage:
        python remap.py <map_file.json>
    """

    if len(sys.argv) < 2:
        print("Usage: python remap.py <map_file.json>")
        sys.exit(1)

    input_file = sys.argv[1]

    if not os.path.exists(input_file):
        print(f"Error: File not found -> {input_file}")
        sys.exit(1)

    process_map(input_file)


# Run script
if __name__ == "__main__":
    main()
