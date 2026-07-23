#!/usr/bin/env python3

from pathlib import Path


FULL_SIZE_CM = 600
HALF_SIZE_CM = FULL_SIZE_CM // 2

GREEN_TILE_CM = 2
BLUE_TILE_CM = 4
SIM_BLOCK_CM = 40


def simulation_regions():
    regions = []
    next_id = 0
    start = -HALF_SIZE_CM + SIM_BLOCK_CM / 2
    stop = HALF_SIZE_CM

    for y in range(int(start), stop, SIM_BLOCK_CM):
        for x in range(int(start), stop, SIM_BLOCK_CM):
            # The three outer columns on each side are green over the full
            # height. In the central neck, two additional green columns on
            # each side, centered at x = +/-160 cm and x = +/-120 cm, leave a
            # five-column blue core. Extending the neck from +/-160 cm to
            # +/-120 cm adds six green 40 x 40 cm regions in total.
            is_blue_top = (-160 <= x <= 160) and (48 <= y <= 300)
            is_blue_bottom = (-160 <= x <= 160) and (-300 <= y <= -48)
            is_blue_neck = (-80 <= x <= 80) and (-48 <= y <= 48)
            constituent_tile_size = (
                BLUE_TILE_CM
                if (is_blue_top or is_blue_bottom or is_blue_neck)
                else GREEN_TILE_CM
            )
            regions.append(
                {
                    "id": next_id,
                    "x": x,
                    "y": y,
                    "size_x": SIM_BLOCK_CM,
                    "size_y": SIM_BLOCK_CM,
                    "constituent_tile_size": constituent_tile_size,
                }
            )
            next_id += 1

    return regions


def iter_constituent_tiles():
    """Subdivide every simulation region without crossing its boundary."""
    for region in simulation_regions():
        tile_size = region["constituent_tile_size"]
        x_min = region["x"] - region["size_x"] // 2
        y_min = region["y"] - region["size_y"] // 2
        for y in range(y_min + tile_size // 2,
                       y_min + region["size_y"],
                       tile_size):
            for x in range(x_min + tile_size // 2,
                           x_min + region["size_x"],
                           tile_size):
                yield x, y, tile_size


def write_digitization_layout(output_path: Path):
    lines = [
        'description: "UBT full digitization geometry on a 600 x 600 cm^2 detector plane"',
        'kind: digitization',
        'units: cm',
        'origin: "detector center at (0, 0)"',
        'note: "Every 2 x 2 cm green or 4 x 4 cm blue tile is contained in one 40 x 40 cm parent region. Three green parent columns cover x = -300 to -180 cm and x = 180 to 300 cm over the full height. The central neck adds the parent columns centered at x = -160, -120, 120, and 160 cm; the innermost pair adds six green regions across the three central rows."',
        'tile_sizes:',
        f'  blue: {BLUE_TILE_CM}',
        f'  green: {GREEN_TILE_CM}',
        'full_size:',
        f'  x: {FULL_SIZE_CM}',
        f'  y: {FULL_SIZE_CM}',
        'tiles:',
    ]

    next_id = 0
    for x, y, tile_size in iter_constituent_tiles():
        lines.append(
            f'  - {{id: {next_id}, x: {x}, y: {y}, size: {tile_size}, constituent_tile_size: {tile_size}}}'
        )
        next_id += 1

    lines.append('mapping: "AddTile(id, centerX, centerY, sizeX, sizeY)"')
    output_path.write_text("\n".join(lines) + "\n")
    return next_id


def write_simulation_layout(output_path: Path):
    lines = [
        'description: "UBT simplified simulation geometry on a 600 x 600 cm^2 detector plane"',
        'kind: simulation',
        'units: cm',
        'origin: "detector center at (0, 0)"',
        'note: "This compact representation uses a 15 x 15 grid of 40 x 40 cm parent regions over the full 6 m x 6 m detector plane. Three green columns occupy each outer side. In the central neck, additional green columns centered at x = -160, -120, 120, and 160 cm surround the five-column blue core."',
        'full_size:',
        f'  x: {FULL_SIZE_CM}',
        f'  y: {FULL_SIZE_CM}',
        'regions:',
    ]

    for region in simulation_regions():
        lines.append(
            "  - {id: %d, x: %d, y: %d, size_x: %d, size_y: %d, constituent_tile_size: %d}"
            % (
                region["id"],
                region["x"],
                region["y"],
                region["size_x"],
                region["size_y"],
                region["constituent_tile_size"],
            )
        )

    lines.append('mapping: "AddRegion(id, centerX, centerY, sizeX, sizeY)"')
    output_path.write_text("\n".join(lines) + "\n")
    return len(simulation_regions())


def main():
    digitization_path = Path("geometry/ubt_geometry_digitization.yaml")
    simulation_path = Path("geometry/ubt_geometry_simulation.yaml")

    digitization_count = write_digitization_layout(digitization_path)
    simulation_count = write_simulation_layout(simulation_path)

    print(f"Wrote {digitization_path} with {digitization_count} tiles")
    print(f"Wrote {simulation_path} with {simulation_count} regions")


if __name__ == "__main__":
    main()
