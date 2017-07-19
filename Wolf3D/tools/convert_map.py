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
    # naively creates a collider for each solid tile
    y = 0
    for row in tiles:
        x = 0
        for tile in row:
            if tile > 0:
                # TODO: Don't hardcode level scaling factor (2) here
                entities.append((ENT_TO_TYPE["bc"], x * 2 + 1, 0, y * 2 + 1, -1, -1, -1, 1, 1, 1))
            x += 1
        y += 1
    return entities

def create_edges_and_connections(tiles):
    edges = []
    connections = []

    # naively creates edges for each tile
    # and connects them
    y = 0
    for row in tiles: 
        x = 0
        for tile in row:
            if tile == 0:
                edges.append((x, y, x, y + 1, 0))
                edges.append((x + 1, y, x + 1, y + 1, 0)) 
                connections.append((len(edges) - 2, len(edges) - 1, tile))
            else:
                edges.append((x, y, x, y + 1, 0))
                edges.append((x, y, x, y + 1, 1))
                connections.append((len(edges) - 2, len(edges) - 1, tile))

                edges.append((x, y + 1, x + 1, y + 1, 0))
                edges.append((x, y + 1, x + 1, y + 1, 1))
                connections.append((len(edges) - 2, len(edges) - 1, tile))
                
                edges.append((x + 1, y + 1, x + 1, y, 0))
                edges.append((x + 1, y + 1, x + 1, y, 1))
                connections.append((len(edges) - 2, len(edges) - 1, tile))

                edges.append((x + 1, y, x, y, 0))
                edges.append((x + 1, y, x, y, 1))
                connections.append((len(edges) - 2, len(edges) - 1, tile))
            # ceiling
            edges.append((x, y, x, y + 1, 1))
            edges.append((x + 1, y, x + 1, y + 1, 1))
            connections.append((len(edges) - 2, len(edges) - 1, CEILING_TILE))
            x += 1
        y += 1 
    return (edges, connections)

def save(edges, connections, entities, filename):
    with open(filename, "w") as f:
        f.write("{} {}\n".format(len(edges), len(connections)))
        for edge in edges:
            f.write("{} {} {} {} {}\n".format(edge[0], edge[1], edge[2], edge[3], edge[4]))
        for con in connections:
            f.write("{} {} {}\n".format(con[0], con[1], con[2])) 

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
    edges, connections = create_edges_and_connections(tiles)
    ents += create_box_collider_ents(tiles)
    save(edges, connections, ents, sys.argv[2])

if __name__ == "__main__":
    main()
