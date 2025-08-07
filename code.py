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
import sys
import adafruit_displayio_ssd1306
from adafruit_display_text.label import Label
from adafruit_bitmap_font import bitmap_font
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff


# Release any existing displays
displayio.release_displays()

# Initialize I2C bus and OLED display
i2c = busio.I2C(scl=board.GP15, sda=board.GP14)
display_bus = displayio.I2CDisplay(i2c, device_address=0x3C)
display = adafruit_displayio_ssd1306.SSD1306(display_bus, width=128, height=64)

# Initialize joystick analog inputs
x_axis = analogio.AnalogIn(board.A0)
y_axis = analogio.AnalogIn(board.A1)

# Initialize joystick button
joystick_button = digitalio.DigitalInOut(board.GP7)
joystick_button.direction = digitalio.Direction.INPUT
joystick_button.pull = digitalio.Pull.UP

# Mapping of the button positions (use None for unused positions)
key_map = [
    10, 11, 12, None,
     1, 3, 5, None,
    0, 2, 4, 6
]

# Load font
font = bitmap_font.load_font("fonts/SpaceGrotesk38.bdf")

# Create display elements
splash = displayio.Group()
display.root_group = splash

# Background rectangle
color_bitmap = displayio.Bitmap(128, 64, 1)
color_palette = displayio.Palette(1)
color_palette[0] = 0xFFFFFF  # White
bg_sprite = displayio.TileGrid(color_bitmap, pixel_shader=color_palette, x=0, y=0)
splash.append(bg_sprite)

# Inner black rectangle
inner_bitmap = displayio.Bitmap(126, 62, 1)
inner_palette = displayio.Palette(1)
inner_palette[0] = 0x000000  # Black
inner_sprite = displayio.TileGrid(inner_bitmap, pixel_shader=inner_palette, x=1, y=1)
splash.append(inner_sprite)

#Loading screen
banner = Label(terminalio.FONT, text="ChoCo\nLoading...", color=0xFFFFFF)
banner.anchor_point = (0.5, 0.5)
banner.anchored_position = (64, 32)
splash.append(banner)
time.sleep(2)
splash.remove(banner)

# Label for displaying chord
chord_label = Label(font, text="", color=0xFFFFFF)
chord_label.anchor_point = (0.5, 0.5)
chord_label.anchored_position = (64, 32)
splash.append(chord_label)

status_label = Label(terminalio.FONT, text="", color=0xFFFFFF)
status_label.anchor_point = (0.5, 0)
status_label.anchored_position = (64, 0)  # Top center
splash.append(status_label)

# Define the row and column pins for the keypad
row_pins = [board.GP0, board.GP1, board.GP2]
col_pins = [board.GP3, board.GP4, board.GP5, board.GP6]

# Initialize the keypad matrix
keys = keypad.KeyMatrix(row_pins, col_pins, columns_to_anodes=False)

# Initialize MIDI
midi = adafruit_midi.MIDI(midi_out=usb_midi.ports[1], out_channel=0)

# Define the MIDI note offsets for major and minor scales
scale_offsets = {
    "major": [0, 2, 4, 5, 7, 9, 11],
    "minor": [0, 2, 3, 5, 7, 8, 10],
}

# Define chord names based on intervals
chord_names = {
    (0, 4, 7): "Maj",
    (0, 3, 7): "Min",
    (0, 4, 8): "Aug",
    (0, 3, 6): "Dim",
    (0, 5, 7): "Sus4",
    (0, 2, 7): "Sus2",
    (0, 4, 7, 11): "Maj7",
    (0, 3, 7, 10): "Min7",
    (0, 4, 7, 9): "Maj6",
    (0, 4, 7, 10): "D7",
    (0, 4, 7, 11, 14): "Maj9th",
    (0, 3, 7, 10, 14): "Min9th"
}

# Track the currently selected root note and scale type
root_note_index = 0
current_scale_type = "major"

pressed_keys = set()
current_chord_type = "major"

status_message_active = False
status_mesage_end_time = 0

inversion_mode_active = False
current_inversion = 0
inversions_per_key = {}  # dict: key_number -> inversion number (0,1,2,...)

# Track the currently playing chord
current_chord = None

last_direction = None
# Define the grace period in seconds
grace_period = 0.3
last_joystick_move_time = 0

def get_analog_value(pin):
    return int((pin.value / 65535) * 127)

# Function to play a chord
def play_chord(chord):
    global current_chord
    for note in chord:
        midi.send(NoteOn(note, 127))  # Note on with velocity 127
    current_chord = chord
    update_displayed_chord(chord, inversion=current_inversion)

# Function to stop playing a chord
def stop_chord(chord):
    global current_chord
    for note in chord:
        midi.send(NoteOff(note, 0))  # Note off with velocity 0
    current_chord = None
    update_displayed_chord([])  # Clear display when chord stops
    
# Function to update the displayed chord and name
def update_displayed_chord(chord, inversion=0):
    global chord_label
    if chord:
        chord_name = get_chord_name(chord)
        inversion_names = ["", "1st Inv", "2nd Inv", "3rd Inv"]  # Etichette per inversioni
        inv_text = inversion_names[inversion] if inversion < len(inversion_names) else f"{inversion}th Inv"
        chord_label.text = f"{chord_name} {inv_text}"
        print(f"Currently playing chord: {chord_label.text}")
    else:
        chord_label.text = ""
        print("Chord stopped, display cleared")

def show_status_message(message, duration=1.5):
    global status_message_active, status_message_end_time
    status_label.text = message
    display.refresh()
    status_message_end_time = time.monotonic() + duration
    status_message_active = True

def map_notes_to_scale(root_note, scale_type):
    scale_offset = scale_offsets[scale_type]
    return [(root_note + offset) % 12 + 60 for offset in scale_offset]

# Function to determine chord name based on intervals
def get_chord_name(chord):
    # Use raw intervals (not mod 12) for extended chords
    raw_intervals = tuple(note - chord[0] for note in chord)
    mod12_intervals = tuple((note - chord[0]) % 12 for note in chord)

    for key, name in chord_names.items():
        if len(key) > 4:
            if raw_intervals == key:
                return name
        else:
            if mod12_intervals == key:
                return name
    return "Unknown"

def get_joystick_chord_modifier(x_value, y_value, base_chord_type):
    CENTER = 64
    DEADZONE = 20

    dx = x_value - CENTER
    dy = y_value - CENTER

    up = dy > DEADZONE
    down = dy < -DEADZONE
    right = dx > DEADZONE
    left = dx < -DEADZONE

    # Example logic:
    if up and right:
        return "7th"
    elif down and right:
        if base_chord_type.startswith("min"):
            return "min9"
        else:
            return "maj9"
    elif right and not up and not down:
        if "min" in base_chord_type:
            return "min7"
        else:
            return "maj7"
    elif down and not left and not right:
        return "sus4"
    elif down and left:
        if "min" in base_chord_type:
            return "sus2"
        else:
            return "maj6"
    elif left and not up and not down:
        return "diminished"
    elif up and left:
        return "augmented"
    elif up and not left and not right:
        # Toggle major/minor maybe? Or base chord
        return "major" if base_chord_type == "minor" else "minor"
    else:
        return base_chord_type

def handle_joystick(x_value, y_value):
    global current_chord, last_joystick_move_time, last_direction

    current_time = time.monotonic()
    if current_time - last_joystick_move_time < grace_period:
        return  # Enforce grace period to avoid rapid repeats

    CENTER = 64
    DEADZONE = 10

    dx = x_value - CENTER
    dy = y_value - CENTER
    

    # Ignore small movements inside deadzone
    if abs(dx) < DEADZONE and abs(dy) < DEADZONE:
        return

    if current_chord is None:
        return

    root_note = current_chord[0]
    chord_name = get_chord_name(current_chord)

    def switch_chord(chord_type):
        nonlocal current_time
        stop_chord(current_chord)
        new_chord = generate_chord(root_note, chord_type)
        play_chord(new_chord)
        print(f"Joystick move: switched to {chord_type} chord")
        last_joystick_move_time = current_time

    # Directions
    up = dy > DEADZONE
    down = dy < -DEADZONE
    right = dx > DEADZONE
    left = dx < -DEADZONE
    
    current_direction = (up, down, left, right)
    
    if not any(current_direction):
        last_direction = None
        return
    
    if current_direction == last_direction:
        return
    
    last_direction = current_direction

    # Determine direction combos and switch chords accordingly
    if up and not left and not right:
        # UP: switch to maj or min root chord based on current
        if current_chord_type == "minor":
            switch_chord("major")
        else:
            switch_chord("minor")

    elif up and right:
        # UP-RIGHT: 7th chord
        switch_chord("7th")

    elif right and not up and not down:
        # RIGHT: maj7 or min7 depending on chord type
        if "Min" in chord_name or "min" in chord_name:
            switch_chord("min7")
        else:
            switch_chord("maj7")

    elif down and right:
        # DOWN-RIGHT: 9th chord
        if current_chord_type.startswith("min"):
            switch_chord("min9")
        else:
            switch_chord("maj9")

    elif down and not left and not right:
        # DOWN: sus4 chord
        switch_chord("sus4")

    elif down and left:
        # DOWN-LEFT: sus2 or maj6 depending on chord type
        if "Min" in chord_name or "min" in chord_name:
            switch_chord("sus2")
        else:
            # Maj6 chord not defined yet? Use "major" or add 'maj6' in generate_chord
            # Let's add 'maj6' in generate_chord with intervals [0,4,7,9]
            switch_chord("maj6")

    elif left and not up and not down:
        # LEFT: diminished chord
        switch_chord("diminished")

    elif up and left:
        # UP-LEFT: augmented chord
        switch_chord("augmented")

    else:
        # No recognized direction, do nothing
        pass

# Function to generate a chord based on root note and chord type
def generate_chord(root_note, chord_type, inversion=0):
    global current_chord_type
    current_chord_type = chord_type
    chord_intervals = {
        'major': [0, 4, 7],
        'minor': [0, 3, 7],
        'diminished': [0, 3, 6],
        'augmented': [0, 4, 8],
        'maj9': [0, 4, 7, 11, 14],
        'min9': [0, 3, 7, 10, 14],
        'maj7': [0, 4, 7, 11],
        'min7': [0, 3, 7, 10],
        'sus2': [0, 2, 7],
        'sus4': [0, 5, 7],
        'maj6': [0, 4, 7, 9],
        'dominant7': [0, 4, 7, 10],
    }
    if chord_type not in chord_intervals:
        print(f"[ERROR] Invalid chord type: {chord_type}")
        return []
    intervals = chord_intervals[chord_type]
    notes = [root_note + interval for interval in intervals]

    # Applica l'inversione: sposta le prime N note un'ottava sopra
    inversion = min(inversion, len(notes) -1)  # non oltre le note disponibili
    for i in range(inversion):
        notes[i] += 12  # sposta un'ottava sopra

    # Ordina per pitch (opzionale ma consigliato)
    notes.sort()
    return notes

def handle_function_keys(key, pressed):
    global root_note_index, current_scale_type
    global inversion_mode_active, current_inversion
    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

    if key == 10:  # Function key to change root note
        root_note_index = (root_note_index + 1) % 12
        note_name = note_names[root_note_index]
        print(f"Root note changed to {note_name}")
        show_status_message(f"Key: {note_name}")

    elif key == 11:  # Function key to change scale type
        scale_types = list(scale_offsets.keys())
        current_scale_index = scale_types.index(current_scale_type)
        current_scale_type = scale_types[(current_scale_index + 1) % len(scale_types)]
        print(f"Scale type changed to {current_scale_type}")
        show_status_message(f"Scale: {current_scale_type}")

    elif key == 12:  # Tasto funzione inversione
        inversion_mode_active = pressed
        if pressed:
            inversion_mode_active = True
            current_inversion = (current_inversion + 1) % 3  # esempio: 0,1,2 ciclicamente
            show_status_message(f"Inversion mode ON: {current_inversion}")
        else:
            inversion_mode_active = False
            current_inversion = 0
            show_status_message("Inversion mode OFF")

def get_chord_type_for_scale(scale_type, degree):
    """
    Given a scale type and a degree, return the chord type for that degree.
    """
    scale_chords = {
        'major': ['major', 'minor', 'minor', 'major', 'major', 'minor', 'diminished'],
        'minor': ['minor', 'diminished', 'major', 'minor', 'minor', 'major', 'major'],
    }
    return scale_chords[scale_type][degree]

# Main loop
while True:
    event = keys.events.get()
    if event:
        row, col = divmod(event.key_number, len(col_pins))
        mapped_key = key_map[event.key_number]

        if event.pressed:
            pressed_keys.add(event.key_number)

            if mapped_key is not None:
                if mapped_key in [10, 11]:  # Function keys
                    handle_function_keys(mapped_key, event.pressed)
                if mapped_key == 12:
                    inversion_mode_active = True
                    show_status_message("Inversion mode ON")
                elif inversion_mode_active:
                    # Mentre inversion_mode è attivo, cicla inversione per questo tasto accordo
                    current_inv = inversions_per_key.get(mapped_key, 0)
                    current_inv = (current_inv + 1) % 3  # cicla 0,1,2
                    if mapped_key is not [10, 11, 12]:
                        inversions_per_key[mapped_key] = current_inv
                        show_status_message(f"Inversion {current_inv} set for key {mapped_key}")
                    else:
                        pass
                else:
                    # Suona accordo con inversione memorizzata
                    inversion_to_use = inversions_per_key.get(mapped_key, 0)
                    scale_notes = map_notes_to_scale(root_note_index, current_scale_type)
                    safe_index = mapped_key % len(scale_notes)
                    root_note = scale_notes[safe_index]
                    base_chord_type = get_chord_type_for_scale(current_scale_type, safe_index)
                    modified_chord_type = get_joystick_chord_modifier(x_value, y_value, base_chord_type)
                    chord = generate_chord(root_note, modified_chord_type, inversion=inversion_to_use)

                    if chord:
                        if current_chord is not None:
                            stop_chord(current_chord)
                        current_chord = chord
                        play_chord(chord)
                        update_displayed_chord(current_chord, inversion=current_inversion)

        elif event.released:
            if event.key_number in pressed_keys:
                pressed_keys.remove(event.key_number)

            if event.key_number == 2:
                inversion_mode_active = False
                show_status_message("Inversion mode OFF")

            # Ferma il suono se non ci sono tasti premuti
            if not pressed_keys and current_chord:
                stop_chord(current_chord)
                current_chord = None
                chord_label.text = ""

    # Handle joystick events
    x_value = get_analog_value(x_axis)
    y_value = get_analog_value(y_axis)
    
    handle_joystick(x_value, y_value)
    
    if status_message_active:
        if time.monotonic() >= status_message_end_time:
            status_label.text = ""
            display.refresh()
            status_message_active = False
