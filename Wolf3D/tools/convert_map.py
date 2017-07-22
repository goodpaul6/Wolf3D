import sys

ENT_TO_TYPE = {
    "player": 0,
    "door": 1,
    "enemy": 2,
    "painting": 3,
    "bc": 4
}

CEILING_TILE = 108

def read_tiles_and_ents(filename):
    tiles = []
    entities = []

    with open(filename, "r") as f:
        w, h = tuple(map(int, f.readline().split()))
        for y in range(h):
            row = tuple(map(int, f.readline().split()))
            if len(row) != w:
                raise RuntimeError("row size does not match width")
            tiles.append(row)
        
        line = f.readline()
        while line:
            tokens = line.split()
            if not tokens: 
                line = f.readline()
                continue
            entities.append(tuple([ENT_TO_TYPE[tokens[0]]] + tokens[1:]))
            line = f.readline()

    return (tiles, entities)

def create_box_collider_ents(tiles):
    entities = []
    solid = [[False for tile in row] for row in tiles]

    x, y = (0, 0)

    def add_box(x, y, w, h):
        print("Adding box", x, y, w, h)
        entities.append((ENT_TO_TYPE["bc"], x * 2 + (w / 2) * 2, 0, y * 2 + (h / 2) * 2, (-w / 2) * 2, -1, (-h / 2) * 2, (w / 2) * 2, 1, (h / 2) * 2))
        for ty in range(y, y + h):
            for tx in range(x, x + w):
                solid[ty][tx] = True

    while y < len(tiles):
        # if this is a solid tile and there is no bounding box here
        # scan for the longest solid consecutive bounds possible
        if not solid[y][x] and tiles[y][x] > 0:
            xx, yy = (x + 1, y + 1)

            while yy < len(tiles) and tiles[yy][x] > 0: yy += 1
            while xx < len(tiles[y]) and tiles[y][xx] > 0: xx += 1

            row_length, col_length = (xx - x, yy - y)

            xx, yy = (x + 1, y + 1)

            # TODO: Check for optimal box (width + height) and compare
            # its area to the row/column colliders

            # longer as a row
            if row_length > col_length:
                add_box(x, y, row_length, 1)
            else:
                add_box(x, y, 1, col_length)

        x += 1
        if x >= len(tiles[y]):
            x = 0
            y += 1

    print("Total number of tiles:", len(tiles) * len(tiles[0]))
    print("Total number of boxes:", len(entities))

    return entities

def create_planes(tiles, scale = 2):
    planes = []

    # naively creates planes for each tile
    # and connects them
    y = 0
    for row in tiles: 
        x = 0
        for tile in row:
            xx = x * scale
            yy = y * scale
            if tile == 0:
                planes.append((xx, 0, yy + scale, xx, 0, yy, xx + scale, 0, yy + scale, tile))
            else:
                # make walls wherever they are visible
                
                # low-x wall
                if x - 1 >= 0 and row[x - 1] == 0:
                    planes.append((xx, 0, yy, xx, scale, yy, xx, 0, yy + scale, tile))
                # low-z wall
                if y - 1 >= 0 and tiles[y - 1][x] == 0:
                    planes.append((xx, 0, yy, xx, scale, yy, xx + scale, 0, yy, tile))
                # high-x wall
                if x + 1 < len(row) and row[x + 1] == 0:
                    planes.append((xx + scale, 0, yy, xx + scale, scale, yy, xx + scale, 0, yy + scale, tile)) 
                # high-z wall
                if y + 1 < len(tiles) and tiles[y + 1][x] == 0:
                    planes.append((xx + scale, 0, yy + scale, xx + scale, scale, yy + scale, xx, 0, yy + scale, tile))
            x += 1
        y += 1 
    
    print("Total number of planes:", len(planes))
    return planes

def save(planes, entities, filename):
    with open(filename, "w") as f:
        f.write("{}\n".format(len(planes)))
        for plane in planes:
            f.write(" ".join(map(str, plane)) + "\n")

        counts = {t: 0 for t in ENT_TO_TYPE.values()}
        
        for ent in entities:
            counts[ent[0]] += 1

        # write entity counts in the expected order (player door enemy ...)
        sorted_types = sorted(ENT_TO_TYPE.values())
        for ent_type in sorted_types:
            f.write(str(counts[ent_type]) + " ")
        f.write("\n") 

        # write sorted entities
        sorted_entities = sorted(entities)
        for ent in sorted_entities:
            # no need to write entity type since that's implied by the order
            f.write(" ".join(map(str, ent[1:])) + "\n")

def main():
    if len(sys.argv) != 3:
        print("Usage: python tiles_to_edges.py path/to/tile/file path/to/map/file")
        sys.exit(0)

    tiles, ents = read_tiles_and_ents(sys.argv[1])
    planes = create_planes(tiles)
    ents += create_box_collider_ents(tiles)
    save(planes, ents, sys.argv[2])

if __name__ == "__main__":
    main()
