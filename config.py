import board

key_map = [
    10, 11, 12, None,
     1, 3, 5, None,
    0, 2, 4, 6
]

row_pins = [board.GP0, board.GP1, board.GP2]
col_pins = [board.GP3, board.GP4, board.GP5, board.GP6]

scale_offsets = {
    "major": (0, 2, 4, 5, 7, 9, 11),
    "minor": (0, 2, 3, 5, 7, 8, 10),
    "harmonic_minor": (0, 2, 3, 5, 7, 8, 11),
    "melodic_minor": (0, 2, 3, 5, 7, 9, 11),
    "dorian": (0, 2, 3, 5, 7, 9, 10),
    "phrygian": (0, 1, 3, 5, 7, 8, 10),
    "lydian": (0, 2, 4, 6, 7, 9, 11),
    "mixolydian": (0, 2, 4, 5, 7, 9, 10),
    "locrian": (0, 1, 3, 5, 6, 8, 10),
}

chord_names = {
    (0, 4, 7): "Maj",
    (0, 3, 7): "Min",
    (0, 4, 8): "Aug",
    (0, 3, 6): "Dim",
    (0, 5, 7): "Sus4",
    (0, 2, 7): "Sus2",
    (0, 4, 7, 9): "Maj6",
    (0, 4, 7, 11): "Maj7",
    (0, 3, 7, 10): "Min7",
    (0, 4, 7, 11, 14): "Maj9",
    (0, 3, 7, 10, 14): "Min9",
    (0, 4, 7, 10): "Dom7"
}