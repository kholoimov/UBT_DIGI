#!/usr/bin/env python3

from pathlib import Path


FULL_SIZE_CM = 600
HALF_SIZE_CM = FULL_SIZE_CM // 2

GREEN_TILE_CM = 2
BLUE_TILE_CM = 4
SIM_BLOCK_CM = 50


def blue_regions():
    return (
        (-200, 200, 48, 300),
        (-200, 200, -300, -48),
        (-100, 100, -48, 48),
    )


def simulation_regions():
    regions = []
    next_id = 0
    start = -HALF_SIZE_CM + SIM_BLOCK_CM / 2
    stop = HALF_SIZE_CM

    for y in range(int(start), stop, SIM_BLOCK_CM):
        for x in range(int(start), stop, SIM_BLOCK_CM):
            is_blue_top = (-200 <= x <= 200) and (48 <= y <= 300)
            is_blue_bottom = (-200 <= x <= 200) and (-300 <= y <= -48)
            is_blue_neck = (-100 <= x <= 100) and (-48 <= y <= 48)
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


def iter_blue_tiles():
    for x_min, x_max, y_min, y_max in blue_regions():
        for y in range(y_min + BLUE_TILE_CM // 2, y_max, BLUE_TILE_CM):
            for x in range(x_min + BLUE_TILE_CM // 2, x_max, BLUE_TILE_CM):
                yield x, y


def covered_green_cells_by_blue():
    covered = set()
    for x, y in iter_blue_tiles():
        for dx in (-1, 1):
            for dy in (-1, 1):
                covered.add((x + dx, y + dy))
    return covered


def iter_green_tiles(covered_cells):
    for y in range(-HALF_SIZE_CM + GREEN_TILE_CM // 2,
                   HALF_SIZE_CM,
                   GREEN_TILE_CM):
        for x in range(-HALF_SIZE_CM + GREEN_TILE_CM // 2,
                       HALF_SIZE_CM,
                       GREEN_TILE_CM):
            if (x, y) not in covered_cells:
                yield x, y


def write_digitization_layout(output_path: Path):
    lines = [
        'description: "UBT full digitization geometry on a 600 x 600 cm^2 detector plane"',
        'kind: digitization',
        'units: cm',
        'origin: "detector center at (0, 0)"',
        'note: "Green tiles form two side columns (x = -300 to -200 cm and x = 200 to 300 cm, over the full y span) plus two neck blocks near x = [-200, -100] cm and [100, 200] cm. To avoid a leftover 2 cm seam when using 4 x 4 cm blue tiles, the blue/neck boundary is snapped from +/-50 cm to +/-48 cm."',
        'tile_sizes:',
        f'  blue: {BLUE_TILE_CM}',
        f'  green: {GREEN_TILE_CM}',
        'full_size:',
        f'  x: {FULL_SIZE_CM}',
        f'  y: {FULL_SIZE_CM}',
        'tiles:',
    ]

    next_id = 0
    covered_cells = covered_green_cells_by_blue()

    for x, y in iter_blue_tiles():
        lines.append(
            f'  - {{id: {next_id}, x: {x}, y: {y}, size: {BLUE_TILE_CM}, constituent_tile_size: {BLUE_TILE_CM}}}'
        )
        next_id += 1

    for x, y in iter_green_tiles(covered_cells):
        lines.append(
            f'  - {{id: {next_id}, x: {x}, y: {y}, size: {GREEN_TILE_CM}, constituent_tile_size: {GREEN_TILE_CM}}}'
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
        'note: "This compact representation uses a 50 x 50 cm block grid over the full 6 m x 6 m detector plane, while preserving the same hourglass structure as the detailed layout: top and bottom blue bands plus a central blue neck, with the remaining area green."',
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
