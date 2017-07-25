from math import floor
import tkinter as tk
from tkinter import ttk

INVALID_TILE = -2
CLEAR_TILE = -1

TILE_BRUSHES = [0, 22]
BRUSH_COLORS = ["grey", "#905C34"]

TILE_TOOL_DRAW = 0
TILE_TOOL_FILL = 1
TILE_TOOLS = (
    ("Draw", TILE_TOOL_DRAW),
    ("Fill", TILE_TOOL_FILL)
)

MODE_TILE = 0
MODE_ENTITY = 1
EDIT_MODES = (
    ("Tiles", MODE_TILE),
    ("Entities", MODE_ENTITY)
)

class Map(tk.Canvas):
    def __init__(self, master, width_in_tiles, height_in_tiles):
        super().__init__(master, borderwidth=1, relief=tk.SUNKEN)

        self.pack(fill=tk.BOTH, expand=True)
        self.configure(xscrollincrement=32, yscrollincrement=32, background="black")

        self.width_in_tiles = width_in_tiles
        self.height_in_tiles = height_in_tiles

        # add grid bounds
        self.bounds_id = self.create_rectangle((0, 0, width_in_tiles * 32, height_in_tiles * 32), outline="white") 

        self.brush_id = self.create_rectangle((0, 0, 32, 32), fill=BRUSH_COLORS[0])

        self.tiles = [[-1 for _ in range(width_in_tiles)] for _ in range(height_in_tiles)]
        self.tile_ids = [[None for _ in range(width_in_tiles)] for _ in range(height_in_tiles)]

    def get_tile_at(self, tx, ty):
        if tx < 0 or tx >= self.width_in_tiles: return INVALID_TILE 
        if ty < 0 or ty >= self.height_in_tiles: return INVALID_TILE
        
        return self.tiles[ty][tx]

    def set_tile_at(self, tx, ty, new_tile, fill_color=None):
        if ty >= 0 and ty < self.height_in_tiles and tx >= 0 and tx < self.width_in_tiles:
            if new_tile != self.tiles[ty][tx] and self.tiles[ty][tx] != CLEAR_TILE:
                self.delete(self.tile_ids[ty][tx])

            if new_tile != CLEAR_TILE and new_tile != self.tiles[ty][tx]:
                self.tile_ids[ty][tx] = self.create_rectangle((tx * 32, ty * 32, (tx + 1) * 32, (ty + 1) * 32), fill=fill_color)
            self.tiles[ty][tx] = new_tile

        self.tag_raise(self.bounds_id)
        self.tag_raise(self.brush_id)

    def set_tile_at_canvas(self, canvas_x, canvas_y, new_tile, fill_color=None):
        tx = floor(canvas_x / 32)
        ty = floor(canvas_y / 32)
        
        self.set_tile_at(tx, ty, new_tile, fill_color)

    def fill_at_canvas(self, canvas_x, canvas_y, new_tile, fill_color):
        tx = floor(canvas_x / 32)
        ty = floor(canvas_y / 32)

        start_tile = self.get_tile_at(tx, ty)
        if start_tile == INVALID_TILE: return

        pos_list = [(tx, ty)]

        while pos_list:
            pos = pos_list.pop(0)
            
            if self.get_tile_at(pos[0], pos[1]) != start_tile: continue
            self.set_tile_at(pos[0], pos[1], new_tile, fill_color)

            pos_list.append((pos[0] - 1, pos[1]))
            pos_list.append((pos[0], pos[1] - 1))
            pos_list.append((pos[0] + 1, pos[1]))
            pos_list.append((pos[0], pos[1] + 1))

class ModePanel(tk.Frame):
    def __init__(self, master, mode_var):
        super().__init__(master)

        self.pack(side=tk.LEFT)
        
        for text, mode in EDIT_MODES:
            button = tk.Radiobutton(self, text=text, variable=mode_var, value=mode)
            button.pack(side=tk.LEFT, anchor=tk.W, padx=5, pady=5)

class TileToolPanel(tk.Frame):
    def __init__(self, master, tool_var):
        super().__init__(master, relief=tk.GROOVE, borderwidth=2)

        self.pack(side=tk.LEFT, fill=tk.Y, padx=5)

        for text, tool in TILE_TOOLS:
            button = tk.Radiobutton(self, text=text, variable=tool_var, value=tool)
            button.pack(side=tk.TOP, anchor=tk.W, padx=5, pady=5)

class Editor(tk.Frame):
    def __init__(self, master, width_in_tiles, height_in_tiles):
        super().__init__(master)

        self.pack(fill=tk.BOTH, expand=True)

        self.brush_idx = 0
        
        self.mode_var = tk.IntVar()
        self.mode_var.set(MODE_TILE)

        self.tool_var = tk.IntVar()
        self.tool_var.set(TILE_TOOL_DRAW)

        row_frame = tk.Frame(self)
        row_frame.pack(fill=tk.X) 

        mode_panel = ModePanel(row_frame, self.mode_var)

        below_frame = tk.Frame(self)
        below_frame.pack(fill=tk.BOTH, expand=True)

        self.tool_panel = TileToolPanel(below_frame, self.tool_var)
        self.map = Map(below_frame, width_in_tiles, height_in_tiles)

        self.bind_all("<Key>", self.key_pressed)
        self.map.bind("<Motion>", self.mouse_moved)
        self.map.bind("<Button-1>", self.mouse_pressed)
        self.map.bind("<Button-3>", self.mouse_pressed)
        self.map.bind("<B1-Motion>", self.mouse_pressed_moved)
        self.map.bind("<B3-Motion>", self.mouse_second_pressed_moved)

    def mouse_pressed(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_FILL:
                self.map.fill_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), TILE_BRUSHES[self.brush_idx], BRUSH_COLORS[self.brush_idx])

    def mouse_second_pressed(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_FILL:
                self.map.fill_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), TILE_BRUSHES[self.brush_idx], BRUSH_COLORS[self.brush_idx])

    def mouse_pressed_moved(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_DRAW:            
                self.map.set_tile_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), TILE_BRUSHES[self.brush_idx], BRUSH_COLORS[self.brush_idx])
                # TODO: Not sure if there's a better way to propogate the event; it doesn't seem to happen by default
                self.mouse_moved(e)

    def mouse_second_pressed_moved(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_DRAW:
                self.map.set_tile_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), CLEAR_TILE)
                self.mouse_moved(e)
    
    def mouse_moved(self, e):
        new_x = self.map.canvasx(e.x) - 16
        new_y = self.map.canvasy(e.y) - 16
        
        self.map.coords(self.map.brush_id, (new_x, new_y, new_x + 32, new_y + 32))

    def switch_brush(self, idx):
        if idx < 0: idx = 0
        elif idx >= len(TILE_BRUSHES): idx = len(TILE_BRUSHES) - 1

        self.brush_idx = idx
        self.map.itemconfig(self.map.brush_id, fill=BRUSH_COLORS[self.brush_idx])

    def key_pressed(self, e):
        if e.char == 'w':
            self.map.yview_scroll(-1, "units")
        elif e.char == 's':
            self.map.yview_scroll(1, "units")
        elif e.char == 'a':
            self.map.xview_scroll(-1, "units")
        elif e.char == 'd':
            self.map.xview_scroll(1, "units")
        elif e.char == 'q':
            self.switch_brush(self.brush_idx - 1)
        elif e.char == 'e':
            self.switch_brush(self.brush_idx + 1)
        elif e.char == 'p':
            if self.mode_var.get() == MODE_TILE:
                self.tool_var.set(TILE_TOOL_DRAW)
        elif e.char == 'f':
            if self.mode_var.get() == MODE_TILE:
                self.tool_var.set(TILE_TOOL_FILL)

class NewMapDialog(tk.Toplevel):
    def __init__(self, main_frame):
        super().__init__()
        
        self.title("New Map")
        
        frame1 = tk.Frame(self)
        frame1.pack(fill=tk.X)

        tk.Label(frame1, text="Width:", width=6).pack(side=tk.LEFT, padx=5, pady=3)
        self.width_entry = tk.Entry(frame1)
        self.width_entry.pack(fill=tk.X, padx=5, expand=True)

        frame2 = tk.Frame(self)
        frame2.pack(fill=tk.X)

        tk.Label(frame2, text="Height:", width=6).pack(side=tk.LEFT, padx=5, pady=3)
        self.height_entry = tk.Entry(frame2);
        self.height_entry.pack(fill=tk.X, padx=5, expand=True)

        center_window(self, 250, 100)

        frame3 = tk.Frame(self)
        frame3.pack(side=tk.BOTTOM, fill=tk.X)

        def confirm(e=None):
            if main_frame.editor_frame:
                main_frame.editor_frame.destroy()
            main_frame.editor_frame = Editor(main_frame, int(self.width_entry.get()), int(self.height_entry.get()))
            self.destroy()

        tk.Button(frame3, text="Create", command=confirm).pack(side=tk.RIGHT, padx=5, pady=5)
        tk.Button(frame3, text="Cancel", command=lambda e: self.destroy()).pack(side=tk.LEFT, padx=5, pady=5)

        self.bind("<Escape>", lambda e: self.destroy())
        self.bind("<Return>", confirm)
        self.bind("<Key>", lambda e: "break")

        self.grab_set()
        self.focus_force()
        self.transient(main_frame)
        self.wait_window(self)
 
class Main(tk.Frame):
    def __init__(self):
        super().__init__()

        self.master.title("Wolf3D Level Editor")

        self.pack(fill=tk.BOTH, expand=True)

        menu_bar = tk.Menu(self.master)
        self.master.config(menu=menu_bar)

        file_menu = tk.Menu(menu_bar)

        file_menu.add_command(label="New Map", underline=0, command=lambda: NewMapDialog(self))
        file_menu.add_separator()
        file_menu.add_command(label="Exit", underline=0, command=lambda: self.quit()) 

        menu_bar.add_cascade(label="File", underline=0, menu=file_menu)

        self.editor_frame = Editor(self, 20, 20)

def center_window(master, width, height):
    w = width
    h = height

    sw = master.winfo_screenwidth()
    sh = master.winfo_screenheight()

    x = int((sw - w) / 2)
    y = int((sh - h) / 2)
    
    master.geometry("{}x{}+{}+{}".format(w, h, x, y))

def main():
    root = tk.Tk()
    root.bind("<Escape>", lambda e: root.quit())

    center_window(root, 640, 480)

    app = Main()
    root.mainloop()

if __name__ == "__main__":
    main()
