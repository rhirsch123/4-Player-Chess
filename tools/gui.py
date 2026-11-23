import tkinter as tk
from tkinter import messagebox
from tkinter import ttk
from typing import Dict, Optional, Tuple

try:
    from .engine_client import EngineClient, EngineClientError
except ImportError:
    from engine_client import EngineClient, EngineClientError


BOARD_SIZE = 14
CELL_SIZE = 48
HIDDEN_COLOR = "#dfe3e8"

START_FEN = (
    "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-"
    "3,yR,yN,yB,yK,yQ,yB,yN,yR,3/"
    "3,yP,yP,yP,yP,yP,yP,yP,yP,3/"
    "14/"
    "bR,bP,10,gP,gR/"
    "bN,bP,10,gP,gN/"
    "bB,bP,10,gP,gB/"
    "bK,bP,10,gP,gQ/"
    "bQ,bP,10,gP,gK/"
    "bB,bP,10,gP,gB/"
    "bN,bP,10,gP,gN/"
    "bR,bP,10,gP,gR/"
    "14/"
    "3,rP,rP,rP,rP,rP,rP,rP,rP,3/"
    "3,rR,rN,rB,rQ,rK,rB,rN,rR,3"
)

PIECE_TEXT = {
    "P": "\u265F",
    "N": "\u265E",
    "B": "\u265D",
    "R": "\u265C",
    "Q": "\u265B",
    "K": "\u265A",
}

PIECE_COLOR = {
    "r": "#e74c3c",
    "b": "#4d78ff",
    "y": "#f1c40f",
    "g": "#27ae60",
}


Square = Tuple[int, int]
Board = Dict[Square, str]


def valid_square(row: int, col: int) -> bool:
    return 0 <= row < BOARD_SIZE and 0 <= col < BOARD_SIZE and (
        (row >= 3 and row <= 10) or (col >= 3 and col <= 10)
    )


def parse_fen4(fen: str) -> Board:
    fields = fen.split("-")
    if len(fields) != 7:
        raise ValueError("Expected a seven-field FEN4 position")

    board: Board = {}
    ranks = fields[6].split("/")
    if len(ranks) != BOARD_SIZE:
        raise ValueError("FEN4 board must contain 14 ranks")

    for row, rank in enumerate(ranks):
        col = 0
        for token in rank.split(","):
            token = token.strip()
            if not token:
                continue
            if token.isdigit():
                col += int(token)
            elif token == "x":
                col += 1
            elif len(token) == 2 and token[0] in PIECE_COLOR and token[1] in PIECE_TEXT:
                if col >= BOARD_SIZE:
                    raise ValueError("Piece lies outside the board")
                board[(row, col)] = token
                col += 1
            else:
                raise ValueError("Unrecognized FEN4 board token: " + token)

            if col > BOARD_SIZE:
                raise ValueError("Rank extends beyond the board")

    return board


def square_to_uci(row: int, col: int) -> str:
    return chr(ord("a") + col) + str(BOARD_SIZE - row)


def uci_to_square(move_part: str) -> Optional[Square]:
    if len(move_part) < 2:
        return None

    col = ord(move_part[0].lower()) - ord("a")
    rank_text = move_part[1:]
    if not rank_text.isdigit():
        return None

    rank = int(rank_text)
    if not 1 <= rank <= BOARD_SIZE:
        return None

    row = BOARD_SIZE - rank
    if valid_square(row, col):
        return (row, col)
    return None


class ChessGui:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("4-Player Chess - Board Viewer")
        self.root.resizable(False, False)

        self.board: Board = {}
        self.selected: Optional[Square] = None
        self.move_count = 0
        self.engine = EngineClient.find_default()
        self.move_history: list[str] = []
        self.legal_moves: set[str] = set()
        self.base_fen = START_FEN
        self.current_turn = "R"
        self.highlighted_destinations: set[Square] = set()

        self._build_layout()
        self.load_fen(START_FEN)
        self.root.protocol("WM_DELETE_WINDOW", self.close)

    def _build_layout(self) -> None:
        outer = ttk.Frame(self.root, padding=10)
        outer.grid(row=0, column=0, sticky="nsew")

        board_width = BOARD_SIZE * CELL_SIZE
        board_height = BOARD_SIZE * CELL_SIZE
        self.canvas = tk.Canvas(
            outer,
            width=board_width,
            height=board_height,
            background=HIDDEN_COLOR,
            highlightthickness=1,
            highlightbackground="#7f8c8d",
        )
        self.canvas.grid(row=0, column=0, rowspan=6)
        self.canvas.bind("<Button-1>", self.on_board_click)

        controls = ttk.Frame(outer, width=280, height=board_height)
        controls.grid(row=0, column=1, sticky="nsew", padx=(12, 0))
        controls.grid_propagate(False)

        self.engine_status_var = tk.StringVar(value="Engine: not connected")
        ttk.Label(
            controls,
            textvariable=self.engine_status_var,
            font=("TkDefaultFont", 9, "bold"),
        ).grid(row=14, column=0, columnspan=2, sticky="ew", pady=(8, 0))

        ttk.Label(controls, text="Turn").grid(row=0, column=0, sticky="w")
        self.turn_label = ttk.Label(controls, text="Red", font=("TkDefaultFont", 11, "bold"))
        self.turn_label.grid(row=0, column=1, sticky="w", padx=(8, 0))

        ttk.Label(controls, text="FEN4 position").grid(row=1, column=0, columnspan=2, sticky="w", pady=(14, 4))
        self.fen_entry = tk.Text(controls, width=32, height=7, wrap="char")
        self.fen_entry.grid(row=2, column=0, columnspan=2, sticky="ew")

        ttk.Button(controls, text="Load position", command=self.load_entry_fen).grid(
            row=3, column=0, columnspan=2, sticky="ew", pady=(6, 0)
        )
        ttk.Button(controls, text="Reset to start", command=lambda: self.load_fen(START_FEN)).grid(
            row=4, column=0, columnspan=2, sticky="ew", pady=(6, 0)
        )

        ttk.Label(controls, text="Move (for example d13d12)").grid(
            row=5, column=0, columnspan=2, sticky="w", pady=(14, 4)
        )
        self.move_entry = ttk.Entry(controls)
        self.move_entry.grid(row=6, column=0, columnspan=2, sticky="ew")
        ttk.Button(controls, text="Apply locally", command=self.apply_move_entry).grid(
            row=7, column=0, columnspan=2, sticky="ew", pady=(6, 0)
        )

        ttk.Separator(controls).grid(row=8, column=0, columnspan=2, sticky="ew", pady=(12, 10))

        ttk.Button(controls, text="Connect engine", command=self.connect_engine).grid(
            row=9, column=0, columnspan=2, sticky="ew"
        )

        ttk.Label(controls, text="Perft depth").grid(row=10, column=0, sticky="w", pady=(10, 4))
        self.perft_entry = ttk.Entry(controls, width=8)
        self.perft_entry.insert(0, "1")
        self.perft_entry.grid(row=10, column=1, sticky="e", pady=(10, 4))
        ttk.Button(controls, text="Run perft", command=self.run_perft).grid(
            row=11, column=0, columnspan=2, sticky="ew"
        )
        ttk.Label(controls, text="Perft result").grid(row=12, column=0, sticky="w", pady=(6, 0))
        self.perft_result_var = tk.StringVar(value="-")
        ttk.Label(
            controls,
            textvariable=self.perft_result_var,
            font=("TkDefaultFont", 11, "bold"),
        ).grid(row=12, column=1, sticky="e", pady=(6, 0))

        self.status_var = tk.StringVar(value="Select a square to inspect it.")
        ttk.Label(
            controls,
            textvariable=self.status_var,
            wraplength=260,
            justify="left",
        ).grid(row=13, column=0, columnspan=2, sticky="ew", pady=(14, 0))

        controls.columnconfigure(0, weight=1)
        controls.columnconfigure(1, weight=1)

    def load_fen(self, fen: str) -> None:
        try:
            board = parse_fen4(fen)
        except ValueError as exc:
            messagebox.showerror("Invalid FEN4", str(exc))
            return

        self.board = board
        self.selected = None
        self.move_count = 0
        self.base_fen = fen
        self.current_turn = fen.split("-", 1)[0]
        self.fen_entry.delete("1.0", "end")
        self.fen_entry.insert("1.0", fen)
        turn_names = {"R": "Red", "B": "Blue", "Y": "Yellow", "G": "Green"}
        self.turn_label.configure(text=turn_names.get(fen.split("-", 1)[0], "Red"))
        self.status_var.set("Position loaded.")
        self.move_history.clear()
        self.legal_moves.clear()
        self.highlighted_destinations.clear()
        if self.engine.process and self.engine.process.poll() is None:
            try:
                self.engine.set_position(fen=fen)
                self.legal_moves = set(self.engine.legal_moves())
                self.status_var.set(
                    "%d legal moves available." % len(self.legal_moves)
                )
            except EngineClientError as exc:
                messagebox.showerror("Engine error", str(exc))
        self.draw_board()

    def load_entry_fen(self) -> None:
        self.load_fen(self.fen_entry.get("1.0", "end").strip())

    def sync_engine_state(self) -> None:
        if not self.engine.process or self.engine.process.poll() is not None:
            raise EngineClientError("Engine is not connected.")

        self.engine.set_position(fen=self.base_fen, moves=self.move_history)
        turn_code = self.engine.get_turn()
        turn_names = {"R": "Red", "B": "Blue", "Y": "Yellow", "G": "Green"}
        turn_codes = {"R": "r", "B": "b", "Y": "y", "G": "g"}

        self.current_turn = turn_code
        self.turn_label.configure(text=turn_names[turn_code])
        self.legal_moves = set(self.engine.legal_moves())
        self.status_var.set(
            "%s to move: %d legal moves."
            % (turn_names[turn_code], len(self.legal_moves))
        )

    def sync_board_from_fen(self, fen: str) -> None:
        try:
            board = parse_fen4(fen)
        except ValueError as exc:
            messagebox.showerror("Invalid FEN4", str(exc))
            return

        self.board = board
        self.selected = None
        self.fen_entry.delete("1.0", "end")
        self.fen_entry.insert("1.0", fen)
        turn_names = {"R": "Red", "B": "Blue", "Y": "Yellow", "G": "Green"}
        self.turn_label.configure(text=turn_names.get(fen.split("-", 1)[0], "Red"))
        self.current_turn = fen.split("-", 1)[0]
        self.highlighted_destinations.clear()
        self.status_var.set("Engine position synchronized.")
        self.draw_board()

    def on_board_click(self, event: tk.Event) -> None:
        col = event.x // CELL_SIZE
        row = event.y // CELL_SIZE
        if not valid_square(row, col):
            self.selected = None
            self.status_var.set("That square is outside the playable board.")
            self.draw_board()
            return

        square = (row, col)
        piece = self.board.get(square)
        if self.selected is None:
            if piece:
                current_turn = {"R": "r", "B": "b", "Y": "y", "G": "g"}[self.current_turn]
                if piece[0] != current_turn:
                    names = {"r": "Red", "b": "Blue", "y": "Yellow", "g": "Green"}
                    self.status_var.set(
                        "It is %s's turn; %s cannot move now."
                        % (names.get(current_turn, "unknown"), names[piece[0]])
                    )
                    self.draw_board()
                    return

            self.selected = square
            if piece:
                color_name = {"r": "Red", "b": "Blue", "y": "Yellow", "g": "Green"}[piece[0]]
                source = square_to_uci(row, col)
                destinations = sorted({
                    move[2:4] for move in self.legal_moves
                    if len(move) >= 4 and move[:2] == source
                })
                self.highlighted_destinations = {
                    uci_to_square(destination)
                    for destination in destinations
                    if uci_to_square(destination) is not None
                }
                destination_text = (
                    "%d legal destination(s)." % len(destinations)
                    if destinations else "No legal destinations."
                )
                self.status_var.set(
                    "Selected %s at %s. %s"
                    % (color_name + " " + piece[1], source, destination_text)
                )
            else:
                self.status_var.set("Selected empty square %s." % square_to_uci(row, col))
        else:
            source_piece = self.board.get(self.selected)
            if source_piece:
                destination = square_to_uci(row, col)
                source = square_to_uci(*self.selected)
                self.apply_local_move(source + destination)
            else:
                self.status_var.set("Selected square contains no piece.")
            self.selected = None
        self.draw_board()

    def apply_move_entry(self) -> None:
        move_text = self.move_entry.get().strip().lower()
        self.apply_local_move(move_text)

    def apply_local_move(self, move_text: str) -> None:
        if len(move_text) < 4:
            messagebox.showerror("Invalid move", "Enter a UCI move such as d13d12.")
            return

        source = uci_to_square(move_text[:2])
        destination = uci_to_square(move_text[2:4])
        if source is None or destination is None:
            messagebox.showerror("Invalid move", "The move must use valid board coordinates.")
            return

        piece = self.board.get(source)
        if not piece:
            messagebox.showerror("Invalid move", "The source square is empty.")
            return

        if not self.engine.process or self.engine.process.poll() is not None:
            messagebox.showerror(
                "Engine required",
                "Connect the engine before applying and validating moves.",
            )
            self.status_var.set("Connect the engine to validate moves.")
            self.selected = None
            self.draw_board()
            return

        if self.engine.process and self.engine.process.poll() is None:
            try:
                if move_text not in self.legal_moves:
                    messagebox.showerror(
                        "Illegal move",
                        "%s is not legal in the current position." % move_text,
                    )
                    self.status_var.set("Rejected illegal move %s." % move_text)
                    self.selected = None
                    self.draw_board()
                    return
            except EngineClientError as exc:
                messagebox.showerror("Engine error", str(exc))
                return

        if len(move_text) == 5:
            promotion = move_text[4].upper()
            promotion_types = {"N": "N", "B": "B", "R": "R", "Q": "Q"}
            if promotion not in promotion_types:
                messagebox.showerror("Invalid move", "Promotion must be n, b, r, or q.")
                return
            moved_piece = piece[0] + promotion_types[promotion]
        else:
            moved_piece = piece

        del self.board[source]
        self.board[destination] = moved_piece

        self.move_history.append(move_text)
        self.status_var.set("Applied local move %s." % move_text)
        self.move_entry.delete(0, "end")
        self.selected = None
        self.highlighted_destinations.clear()
        if self.engine.process and self.engine.process.poll() is None:
            try:
                self.sync_engine_state()
            except EngineClientError as exc:
                messagebox.showerror("Engine error", str(exc))
        self.draw_board()

    def connect_engine(self) -> None:
        try:
            if not self.engine.executable:
                self.engine = EngineClient.find_default()
            if not self.engine.process or self.engine.process.poll() is not None:
                self.engine.start()
            fen = self.fen_entry.get("1.0", "end").strip()
            self.base_fen = fen
            self.sync_engine_state()
            self.engine_status_var.set("Engine: connected")
            self.status_var.set("Engine connected and position synchronized.")
        except EngineClientError as exc:
            self.engine_status_var.set("Engine: unavailable")
            messagebox.showerror("Engine error", str(exc))

    def run_perft(self) -> None:
        try:
            depth = int(self.perft_entry.get())
        except ValueError:
            messagebox.showerror("Invalid perft depth", "Enter a non-negative integer.")
            return

        try:
            if not self.engine.process or self.engine.process.poll() is not None:
                self.engine.start()
                fen = self.fen_entry.get("1.0", "end").strip()
                self.engine.set_position(fen=fen, moves=self.move_history)

            result = self.engine.perft(depth)
            self.perft_result_var.set(str(result))
            self.status_var.set("Perft depth %d completed." % depth)
        except EngineClientError as exc:
            messagebox.showerror("Engine error", str(exc))

    def close(self) -> None:
        self.engine.close()
        self.root.destroy()

    def draw_board(self) -> None:
        self.canvas.delete("all")

        for row in range(BOARD_SIZE):
            for col in range(BOARD_SIZE):
                x1 = col * CELL_SIZE
                y1 = row * CELL_SIZE
                x2 = x1 + CELL_SIZE
                y2 = y1 + CELL_SIZE

                if not valid_square(row, col):
                    self.canvas.create_rectangle(
                        x1, y1, x2, y2,
                        fill=HIDDEN_COLOR,
                        outline=HIDDEN_COLOR,
                    )
                    continue

                fill = "#f0d9b5" if (row + col) % 2 == 0 else "#b58863"
                if self.selected == (row, col):
                    fill = "#7fc8a9"
                if (row, col) in self.highlighted_destinations:
                    fill = "#85c1e9"

                self.canvas.create_rectangle(
                    x1, y1, x2, y2,
                    fill=fill,
                    outline="#6d4c41",
                    width=1,
                )

                piece = self.board.get((row, col))
                if piece:
                    symbol = PIECE_TEXT[piece[1]]
                    self.canvas.create_text(
                        x1 + CELL_SIZE // 2,
                        y1 + CELL_SIZE // 2,
                        text=symbol,
                        fill=PIECE_COLOR[piece[0]],
                        font=("Segoe UI Symbol", 30),
                    )


def main() -> None:
    root = tk.Tk()
    ChessGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
