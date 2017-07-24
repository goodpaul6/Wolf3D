import argparse
from PIL import Image

def create_image_without_padding(image, rows, columns, tile_width, tile_height, padding):
    new_image = Image.new(image.mode, (columns * tile_width, rows * tile_height), (0, 0, 0, 0)) 
    for row in range(rows):
        for column in range(columns):
            new_image.paste(image.crop((column * (tile_width + padding), row * (tile_height + padding), (column + 1) * (tile_width + padding), (row + 1) * (tile_height + padding))), (column * tile_width, row * tile_height))

    return new_image

def main():
    parser = argparse.ArgumentParser(description="Tool for extracting sprites from images and manipulating them.")
    parser.add_argument("-i", "--image", type=str, required=True, help="Image to manipulate.")
    parser.add_argument("-o", "--output-image", type=str, default="sprite.png", help="Path to output image.")
    parser.add_argument("-r", "--rows", type=int, required=True, help="Number of rows in the input image.")
    parser.add_argument("-c", "--columns", type=int, required=True, help="Number of columns in the input image.")
    parser.add_argument("-t", "--tile-size", nargs=2, type=int, required=True, help="Size of each tile (width, height)")
    parser.add_argument("-p", "--padding", type=int, required=True, help="Amount of padding in the input spritesheet (in pixels)")

    args = parser.parse_args()
    
    image = Image.open(args.image)
    new_image = create_image_without_padding(image, args.rows, args.columns, args.tile_size[0], args.tile_size[1], args.padding)

    new_image.save(args.output_image)

if __name__ == "__main__":
    main()
