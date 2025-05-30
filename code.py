import time
import board
import keypad
import analogio
import digitalio
import usb_midi
import adafruit_midi
import displayio
import busio
import supervisor
import terminalio
import adafruit_displayio_ssd1306
from adafruit_bitmap_font import bitmap_font

from config import key_map, row_pins, col_pins, scale_offsets, chord_names
from midi_utils import generate_chord, invert_chord, play_chord, stop_chord
from display_utils import setup_display, update_displayed_chord
from input_utils import handle_inversion_assignment

from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff

displayio.release_displays()
i2c = busio.I2C(scl=board.GP15, sda=board.GP14)
display_bus = displayio.I2CDisplay(i2c, device_address=0x3C)
display = adafruit_displayio_ssd1306.SSD1306(display_bus, width=128, height=64)

x_axis = analogio.AnalogIn(board.A0)
y_axis = analogio.AnalogIn(board.A1)

joystick_button = digitalio.DigitalInOut(board.GP7)
joystick_button.direction = digitalio.Direction.INPUT
joystick_button.pull = digitalio.Pull.UP

font = bitmap_font.load_font("fonts/SpaceGrotesk38.bdf")
chord_label = setup_display(display, font)

keys = keypad.KeyMatrix(row_pins, col_pins, columns_to_anodes=False)
midi = adafruit_midi.MIDI(midi_out=usb_midi.ports[1], out_channel=0)

root_note_index = 0
current_scale_type = "major"
current_chord = None
grace_period = 0.3
last_joystick_move_time = 0
chord_inversions = {k: 0 for k in range(7)}

def get_analog_value(pin):
    return int((pin.value / 65535) * 127)

def map_notes_to_scale(root_note, scale_type):
    scale_offset = scale_offsets[scale_type]
    return [(root_note + offset) % 12 + 60 for offset in scale_offset]

def get_chord_name(chord):
    intervals = tuple((note - chord[0]) % 12 for note in chord)
    for key, name in chord_names.items():
        if intervals == key:
            return name
    return "Unknown"

def handle_joystick(x_value, y_value):
    global current_chord
    global last_joystick_move_time
    
    current_time = time.monotonic()
    if current_time - last_joystick_move_time < grace_period:
        return  # Skip processing if within the grace period

    if current_chord is None:
        return  # No chord to alter

    root_note = current_chord[0]
    current_chord_name = get_chord_name(current_chord)
    if "Min" in current_chord_name:
        if y_value > 100:  # UP: Switch to Min7
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "min7")
            play_chord(current_chord)
            print("Changed to Min7 chord")
            last_joystick_move_time = current_time
        elif y_value < 27:  # DOWN: Switch to Maj
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "major")
            play_chord(current_chord)
            print("Changed to Maj chord")
            last_joystick_move_time = current_time
    elif "Maj" in current_chord_name:
        if y_value > 100:  # UP: Switch to Maj7
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "maj7")
            play_chord(current_chord)
            print("Changed to Maj7 chord")
            last_joystick_move_time = current_time
        elif y_value < 27:  # DOWN: Switch to Min
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "minor")
            play_chord(current_chord)
            print("Changed to Min chord")
            last_joystick_move_time = current_time
    elif "min7" in current_chord_name or "maj7" in current_chord_name:
        if y_value < 27:  # DOWN: Switch to root chord
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "minor" if "min7" in current_chord_name else "major")
            play_chord(current_chord)
            print("Changed to root chord")
            last_joystick_move_time = current_time
            
    if x_value < 27:  # LEFT: Change chord to sus2
        stop_chord(current_chord)
        current_chord = generate_chord(root_note, "sus2")
        play_chord(current_chord)
        print("Changed to sus2 chord")
        last_joystick_move_time = current_time
    elif x_value > 100:  # RIGHT: Change chord to sus4
        stop_chord(current_chord)
        current_chord = generate_chord(root_note, "sus4")
        play_chord(current_chord)
        print("Changed to sus4 chord")
        last_joystick_move_time = current_time


def handle_function_keys(key):
    global root_note_index, current_scale_type
    if key == 10:
        if joystick_button.value == 0:
            handle_inversion_assignment(keys, key_map, chord_inversions)
        else:
            root_note_index = (root_note_index + 1) % 12
    elif key == 11:
        scale_types = list(scale_offsets.keys())
        current_scale_index = scale_types.index(current_scale_type)
        current_scale_type = scale_types[(current_scale_index + 1) % len(scale_types)]
    elif key == 12:
        supervisor.reload()

def get_chord_type_for_scale(scale_type, degree):
    scale_chords = {
        'major': ('major', 'minor', 'minor', 'major', 'major', 'minor', 'diminished'),
        'minor': ('minor', 'diminished', 'major', 'minor', 'minor', 'major', 'major'),
        'harmonic_minor': ('minor', 'diminished', 'augmented', 'minor', 'major', 'major', 'diminished'),
        'melodic_minor': ('minor', 'minor', 'augmented', 'major', 'major', 'diminished', 'diminished'),
        'dorian': ('minor', 'minor', 'major', 'major', 'minor', 'diminished', 'major'),
        'phrygian': ('minor', 'major', 'major', 'minor', 'minor', 'major', 'diminished'),
        'lydian': ('major', 'major', 'minor', 'diminished', 'major', 'minor', 'minor'),
        'mixolydian': ('major', 'minor', 'diminished', 'major', 'minor', 'minor', 'major'),
        'locrian': ('diminished', 'major', 'minor', 'minor', 'major', 'major', 'minor'),
    }
    return scale_chords[scale_type][degree]

while True:
    event = keys.events.get()
    if event:
        row, col = divmod(event.key_number, len(col_pins))
        mapped_key = key_map[event.key_number]
        if event.pressed:
            if mapped_key is not None:
                if mapped_key in [10, 11, 12]:
                    handle_function_keys(mapped_key)
                else:
                    scale_notes = map_notes_to_scale(root_note_index, current_scale_type)
                    root_note = scale_notes[mapped_key]
                    chord_type = get_chord_type_for_scale(current_scale_type, mapped_key)
                    chord = generate_chord(root_note, chord_type)
                    inversion = chord_inversions.get(mapped_key, 0)
                    chord = invert_chord(chord, inversion)
                    if chord:
                        if current_chord is not None:
                            stop_chord(midi, current_chord)
                        current_chord = chord
                        play_chord(midi, chord)
                        update_displayed_chord(chord_label, get_chord_name(current_chord))
        elif event.released:
            if mapped_key is not None:
                if current_chord:
                    stop_chord(midi, current_chord)
                    current_chord = None
                    update_displayed_chord(chord_label, "")

    x_value = get_analog_value(x_axis)
    y_value = get_analog_value(y_axis)
    handle_joystick(x_value, y_value)

