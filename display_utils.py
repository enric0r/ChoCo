import displayio
from adafruit_display_text.label import Label
import terminalio

def setup_display(display, font):
    splash = displayio.Group()
    display.root_group = splash
    chord_label = Label(font, text="", color=0xFFFFFF)
    chord_label.anchor_point = (0.5, 0.5)
    chord_label.anchored_position = (64, 32)
    splash.append(chord_label)
    return chord_label

def update_displayed_chord(chord_label, chord_name):
    chord_label.text = chord_name