#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path


TILE_RE = re.compile(
    r"\{\s*id:\s*(?P<id>-?\d+),\s*x:\s*(?P<x>-?\d+(?:\.\d+)?),\s*y:\s*(?P<y>-?\d+(?:\.\d+)?),\s*size:\s*(?P<size>-?\d+(?:\.\d+)?),\s*constituent_tile_size:\s*(?P<constituent_tile_size>-?\d+(?:\.\d+)?)\s*\}"
)
REGION_RE = re.compile(
    r"\{\s*id:\s*(?P<id>-?\d+),\s*x:\s*(?P<x>-?\d+(?:\.\d+)?),\s*y:\s*(?P<y>-?\d+(?:\.\d+)?),\s*size_x:\s*(?P<size_x>-?\d+(?:\.\d+)?),\s*size_y:\s*(?P<size_y>-?\d+(?:\.\d+)?),\s*constituent_tile_size:\s*(?P<constituent_tile_size>-?\d+(?:\.\d+)?)\s*\}"
)


def parse_layout(path: Path):
    tiles = []
    regions = []
    full_x = None
    full_y = None
    pending_full_size = False

    for raw_line in path.read_text().splitlines():
        line = raw_line.strip()
        if line == "full_size:":
            pending_full_size = True
            continue
        if pending_full_size and line.startswith("x:"):
            full_x = float(line.split(":", 1)[1].strip())
            continue
        if pending_full_size and line.startswith("y:"):
            full_y = float(line.split(":", 1)[1].strip())
            pending_full_size = False
            continue

        match = TILE_RE.search(line)
        if match:
            tiles.append(
                {
                    "id": int(match.group("id")),
                    "x": float(match.group("x")),
                    "y": float(match.group("y")),
                    "size": float(match.group("size")),
                    "constituent_tile_size": float(
                        match.group("constituent_tile_size")
                    ),
                }
            )
            continue

        match = REGION_RE.search(line)
        if match:
            regions.append(
                {
                    "id": int(match.group("id")),
                    "x": float(match.group("x")),
                    "y": float(match.group("y")),
                    "size_x": float(match.group("size_x")),
                    "size_y": float(match.group("size_y")),
                    "constituent_tile_size": float(
                        match.group("constituent_tile_size")
                    ),
                }
            )

    if not tiles and not regions:
        raise ValueError(f"No drawable geometry entries found in {path}")

    return tiles, regions, full_x, full_y


def main():
    parser = argparse.ArgumentParser(description="Plot a UBT tile layout.")
    parser.add_argument(
        "--input",
        default="geometry/ubt_geometry_simulation.yaml",
        help="Path to the tile layout file.",
    )
    parser.add_argument(
        "--output",
        default="geometry/ubt_geometry_simulation.png",
        help="Path to the output PNG file.",
    )
    args = parser.parse_args()

    try:
        import matplotlib.pyplot as plt
        from matplotlib.patches import Rectangle
    except ImportError:
        print("matplotlib is required to draw the geometry.", file=sys.stderr)
        return 1

    layout_path = Path(args.input)
    tiles, regions, full_x, full_y = parse_layout(layout_path)

    fig, ax = plt.subplots(figsize=(10, 10))

    def build_size_color_maps():
        tile_constituent_sizes = sorted(
            {tile["constituent_tile_size"] for tile in tiles}, reverse=True
        )
        region_constituent_sizes = sorted(
            {region["constituent_tile_size"] for region in regions}, reverse=True
        )

        palette = ["#6f8fdc", "#4ecb48", "#e0b84f", "#d97b66", "#8f77c7"]
        tile_constituent_map = {
            size: palette[index % len(palette)]
            for index, size in enumerate(tile_constituent_sizes)
        }
        region_constituent_map = {
            size: palette[index % len(palette)]
            for index, size in enumerate(region_constituent_sizes)
        }
        return tile_constituent_map, region_constituent_map

    (
        constituent_tile_color_map,
        constituent_region_color_map,
    ) = build_size_color_maps()

    def tile_facecolor(tile):
        return constituent_tile_color_map.get(
            tile["constituent_tile_size"], "#b0b0b0"
        )

    def region_facecolor(region):
        return constituent_region_color_map.get(
            region["constituent_tile_size"], "#b0b0b0"
        )

    for region in regions:
        rect = Rectangle(
            (region["x"] - region["size_x"] / 2.0,
             region["y"] - region["size_y"] / 2.0),
            region["size_x"],
            region["size_y"],
            facecolor=region_facecolor(region),
            edgecolor="white",
            linewidth=0.8,
        )
        ax.add_patch(rect)

    for tile in tiles:
        half = tile["size"] / 2.0
        rect = Rectangle(
            (tile["x"] - half, tile["y"] - half),
            tile["size"],
            tile["size"],
            facecolor=tile_facecolor(tile),
            edgecolor="white",
            linewidth=0.15,
        )
        ax.add_patch(rect)

    if full_x is None or full_y is None:
        x_extent = 0.0
        y_extent = 0.0
        if tiles:
            xs = [tile["x"] for tile in tiles]
            ys = [tile["y"] for tile in tiles]
            ss = [tile["size"] for tile in tiles]
            x_extent = max(x_extent, max(x + s / 2.0 for x, s in zip(xs, ss)))
            y_extent = max(y_extent, max(y + s / 2.0 for y, s in zip(ys, ss)))
        if regions:
            x_extent = max(
                x_extent,
                max(region["x"] + region["size_x"] / 2.0 for region in regions),
            )
            y_extent = max(
                y_extent,
                max(region["y"] + region["size_y"] / 2.0 for region in regions),
            )
    else:
        x_extent = full_x / 2.0
        y_extent = full_y / 2.0

    margin = 10.0
    ax.set_xlim(-x_extent - margin, x_extent + margin)
    ax.set_ylim(-y_extent - margin, y_extent + margin)
    ax.set_aspect("equal")
    ax.set_xlabel("x [cm]")
    ax.set_ylabel("y [cm]")
    if tiles:
        ax.set_title(f"UBT tile geometry ({len(tiles)} tiles)")
    else:
        ax.set_title(f"UBT simplified geometry ({len(regions)} regions)")
    ax.grid(False)
    ax.set_facecolor("white")

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(output_path, dpi=200)
    print(f"Saved geometry plot to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
