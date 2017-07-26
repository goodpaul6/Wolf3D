import os
from math import floor
from copy import deepcopy
import tkinter as tk
import tkinter.filedialog
from tkinter import ttk
import convert_map

INVALID_TILE = -2
CLEAR_TILE = -1

MAX_TILEMAP_UNDO = 32

TILE_BRUSHES = [0, 22]
TILE_COLORS = {
    0: "grey", 
    22: "#905C34"
}

TILE_BRUSH_NAMES = ["Floor", "Wood"]

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

        self.brush_id = self.create_rectangle((0, 0, 32, 32), fill=TILE_COLORS[TILE_BRUSHES[0]])

        self.tiles = [[-1 for _ in range(width_in_tiles)] for _ in range(height_in_tiles)]
        self.tile_ids = [[None for _ in range(width_in_tiles)] for _ in range(height_in_tiles)]

    def set_tiles(self, tiles):
        for y, row in enumerate(self.tile_ids):
            for x, item in enumerate(row):
                if item:
                    self.delete(item)

        self.tiles = tiles
        self.tile_ids = [[None for _ in row] for row in tiles]

        self.width_in_tiles = len(tiles[0])
        self.height_in_tiles = len(tiles)

        for y, row in enumerate(self.tiles):
            for x, tile in enumerate(row):
                if tile != CLEAR_TILE:
                    self.tile_ids[y][x] = self.create_rectangle((x * 32, y * 32, (x + 1) * 32, (y + 1) * 32), fill=TILE_COLORS[tile])
        
        self.tag_raise(self.bounds_id)
        self.tag_raise(self.brush_id)

    def get_tile_at(self, tx, ty):
        if tx < 0 or tx >= self.width_in_tiles: return INVALID_TILE 
        if ty < 0 or ty >= self.height_in_tiles: return INVALID_TILE
        
        return self.tiles[ty][tx]

    def set_tile_at(self, tx, ty, new_tile):
        if ty >= 0 and ty < self.height_in_tiles and tx >= 0 and tx < self.width_in_tiles:
            if new_tile != self.tiles[ty][tx] and self.tiles[ty][tx] != CLEAR_TILE:
                self.delete(self.tile_ids[ty][tx])

            if new_tile != CLEAR_TILE and new_tile != self.tiles[ty][tx]:
                self.tile_ids[ty][tx] = self.create_rectangle((tx * 32, ty * 32, (tx + 1) * 32, (ty + 1) * 32), fill=TILE_COLORS[new_tile])
            self.tiles[ty][tx] = new_tile

        self.tag_raise(self.bounds_id)
        self.tag_raise(self.brush_id)

    def set_tile_at_canvas(self, canvas_x, canvas_y, new_tile):
        tx = floor(canvas_x / 32)
        ty = floor(canvas_y / 32)
        
        self.set_tile_at(tx, ty, new_tile)

    def fill_at_canvas(self, canvas_x, canvas_y, new_tile):
        tx = floor(canvas_x / 32)
        ty = floor(canvas_y / 32)

        start_tile = self.get_tile_at(tx, ty)
        if start_tile == INVALID_TILE: return

        pos_list = [(tx, ty)]

        while pos_list:
            pos = pos_list.pop(0)
            
            if self.get_tile_at(pos[0], pos[1]) != start_tile: continue
            self.set_tile_at(pos[0], pos[1], new_tile)

            pos_list.append((pos[0] - 1, pos[1]))
            pos_list.append((pos[0], pos[1] - 1))
            pos_list.append((pos[0] + 1, pos[1]))
            pos_list.append((pos[0], pos[1] + 1))

class MapUndo:
    def __init__(self, _map, max_size):
        self.map = _map
        self.max_size = max_size
        self.buffer = []
        self.redo_buffer = []
    
    def _record(self):
        if len(self.buffer) >= self.max_size:
            self.buffer.pop(0)
        self.buffer.append(deepcopy(self.map.tiles))

    def record(self):
        self._record()
        self.redo_buffer = []

    def apply(self):
        if self.buffer:
            # record state before undo for redo
            self.redo_buffer.append(deepcopy(self.map.tiles))
            self.map.set_tiles(self.buffer.pop())
    
    def unapply(self):
        if self.redo_buffer:
            # store state before redo for undoing redo
            self._record()
            self.map.set_tiles(self.redo_buffer.pop())

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

class TileBrushPanel(tk.Frame):
    def __init__(self, master, brush_var):
        super().__init__(master, relief=tk.GROOVE, borderwidth=2)

        self.pack(side=tk.BOTTOM, fill=tk.X, padx=5)

        for index in range(len(TILE_BRUSHES)):
            button = tk.Radiobutton(self, text=TILE_BRUSH_NAMES[index], variable=brush_var, value=index)
            button.pack(side=tk.LEFT, anchor=tk.W, padx=5, pady=5)

class Editor(tk.Frame):
    def __init__(self, master, width_in_tiles, height_in_tiles):
        super().__init__(master)

        self.pack(fill=tk.BOTH, expand=True)

        self.mode_var = tk.IntVar()
        self.mode_var.set(MODE_TILE)

        self.tool_var = tk.IntVar()
        self.tool_var.set(TILE_TOOL_DRAW)

        self.brush_var = tk.IntVar()
        self.brush_var.set(0)

        def update_brush(*args):
            self.map.itemconfig(self.map.brush_id, fill=TILE_COLORS[TILE_BRUSHES[self.brush_var.get()]])

        self.brush_var.trace("w", update_brush)

        row_frame = tk.Frame(self)
        row_frame.pack(fill=tk.X) 

        mode_panel = ModePanel(row_frame, self.mode_var)

        below_frame = tk.Frame(self)
        below_frame.pack(fill=tk.BOTH, expand=True)

        self.tool_panel = TileToolPanel(below_frame, self.tool_var)
        self.brush_panel = TileBrushPanel(below_frame, self.brush_var)
        self.map = Map(below_frame, width_in_tiles, height_in_tiles)

        self.undo = MapUndo(self.map, MAX_TILEMAP_UNDO)

        self.bind_all("<Key>", self.key_pressed) 

        self.map.bind("<Motion>", self.mouse_moved)

        self.map.bind("<Button-1>", self.mouse_pressed)
        self.map.bind("<Button-3>", self.mouse_pressed)

        self.map.bind("<B1-Motion>", self.mouse_pressed_moved)
        self.map.bind("<B3-Motion>", self.mouse_second_pressed_moved)
        self.map.bind("<Shift-B1-Motion>", self.mouse_second_pressed_moved)

    def mouse_pressed(self, e):
        self.undo.record()
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_FILL:
                self.map.fill_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), TILE_BRUSHES[self.brush_var.get()])
                self.tool_var.set(TILE_TOOL_DRAW)

    def mouse_second_pressed(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_FILL:
                self.map.fill_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), CLEAR_TILE)
                self.tool_var.set(TILE_TOOL_DRAW)

    def mouse_pressed_moved(self, e):
        if self.mode_var.get() == MODE_TILE:
            if self.tool_var.get() == TILE_TOOL_DRAW:            
                self.map.set_tile_at_canvas(self.map.canvasx(e.x), self.map.canvasy(e.y), TILE_BRUSHES[self.brush_var.get()])
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

    def key_pressed(self, e):
        if e.char == 'u':
            self.undo.apply()
        elif e.char == 'r':
            self.undo.unapply()
        elif e.char == 'w':
            self.map.yview_scroll(-1, "units")
        elif e.char == 's':
            self.map.yview_scroll(1, "units")
        elif e.char == 'a':
            self.map.xview_scroll(-1, "units")
        elif e.char == 'd':
            self.map.xview_scroll(1, "units")
        elif e.char == 'q':
            old_value = self.brush_var.get()
            self.brush_var.set(old_value - 1 if old_value > 0 else old_value)
        elif e.char == 'e':
            old_value = self.brush_var.get()
            self.brush_var.set(old_value + 1 if old_value < len(TILE_BRUSHES) - 1 else old_value)
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
            if main_frame.editor:
                main_frame.editor.destroy()
                main_frame.editor = None
            main_frame.editor = Editor(main_frame, int(self.width_entry.get()), int(self.height_entry.get()))
            self.destroy() 

        def tab_pressed(e):
            e.widget.tk_focusNext().focus()
            return "break"

        def shift_tab_pressed(e):
            e.widget.tk_focusPrev().focus()
            return "break"

        def key_pressed(e):
            return "break"

        tk.Button(frame3, text="Create", command=confirm).pack(side=tk.RIGHT, padx=5, pady=5)
        tk.Button(frame3, text="Cancel", command=lambda e: self.destroy()).pack(side=tk.LEFT, padx=5, pady=5)

        self.bind("<Escape>", lambda e: self.destroy())
        self.bind("<Return>", confirm)
        self.bind("<Tab>", tab_pressed)
        self.bind("<Shift-Tab>", shift_tab_pressed)
        self.bind("<Key>", key_pressed)
        
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

        self.save_name = None

        def save_as_handler():
            if not self.editor: return
            name = tk.filedialog.asksaveasfilename(filetypes=(("Tile", ".tile"), ("All", ".*")), \
                                                     initialdir=os.getcwd(), \
                                                     parent=self, \
                                                     title="Save As", \
                                                     defaultextension=".tile")
            if name == "": return

            self.save(name)
        
        def open_handler():
            if not self.editor: return
            name = tk.filedialog.askopenfilename(filetypes=(("Tile", ".tile"), ("All", ".*")), \
                    initialdir=os.getcwd(), \
                    parent=self, \
                    title="Open")

            if name == "": return

            tiles, entities = convert_map.read(name)
        
            # TODO: Make an editor.load_map method or something
            self.editor.undo.record()
            self.editor.map.set_tiles(tiles)

        file_menu.add_command(label="Open", underline=0, command=open_handler)
        file_menu.add_command(label="Save As", underline=0, command=save_as_handler) 
        file_menu.add_command(label="New Map", underline=0, command=lambda: NewMapDialog(self))
        file_menu.add_separator()
        file_menu.add_command(label="Exit", underline=0, command=lambda: self.quit()) 

        menu_bar.add_cascade(label="File", underline=0, menu=file_menu)

        self.editor = Editor(self, 20, 20) 

    def save(self, filename):
        self.save_name = filename
            
        tiles = self.editor.map.tiles
        entities = convert_map.create_box_collider_ents(tiles)

        convert_map.save(tiles, entities, filename)
    
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
