import json
import re
import sys
import os

LOOKUP_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lookup.json")


def load_json_with_comments(path):
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    content = re.sub(r"//[^\n]*", "", content)
    return json.loads(content)


def build_remap_table(path):
    """
    Builds the remap table from lookup.json.

    Rule: lookup entry "K": V means file tile (K+1) -> output (V+1).
    Example: "1432": 996  =>  tile 1433 in file becomes 997 in output.
    Example: "520": -1    =>  tile 521 in file becomes 0 in output.
    """
    raw = load_json_with_comments(path)
    return {int(k) + 1: int(v) + 1 for k, v in raw.items()}


def process_file(input_path):
    with open(input_path, "r", encoding="utf-8", newline="") as f:
        content = f.read()

    table = build_remap_table(LOOKUP_FILE)
    warned = set()

    def remap(tile_id):
        if tile_id == 0:
            return 0
        if tile_id in table:
            return table[tile_id]
        if tile_id not in warned:
            print(f"⚠ Warning: No mapping for tile ID {tile_id}")
            warned.add(tile_id)
        return tile_id

    def replace_in_data_array(m):
        def replace_num(nm):
            return str(remap(int(nm.group())))
        # Only replace digits within the array content, not the key name
        prefix = m.group(1)
        array_body = m.group(2)
        return prefix + re.sub(r"\d+", replace_num, array_body)

    # Remap tile data arrays: "data":[...] (may span multiple lines)
    content = re.sub(
        r'("data"\s*:\s*)(\[[^\]]*\])',
        replace_in_data_array,
        content,
        flags=re.DOTALL,
    )

    # Remap gid fields in object layers: "gid": NUMBER
    def replace_gid(m):
        return m.group(1) + str(remap(int(m.group(2))))

    content = re.sub(r'("gid"\s*:\s*)(\d+)', replace_gid, content)

    with open(input_path, "w", encoding="utf-8", newline="") as f:
        f.write(content)

    print(f"✔ Processed in-place: {input_path}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python remap.py <map_file.tmj> [<map_file2.tmj> ...]")
        sys.exit(1)

    for path in sys.argv[1:]:
        if not os.path.exists(path):
            print(f"Error: File not found -> {path}")
            sys.exit(1)
        process_file(path)


if __name__ == "__main__":
    main()
