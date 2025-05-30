from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff

def generate_chord(root_note, chord_type):
    chord_intervals = {
        'major': (0, 4, 7),
        'minor': (0, 3, 7),
        'diminished': (0, 3, 6),
        'augmented': (0, 4, 8),
        '7th': (0, 4, 7, 10),
        'maj7': (0, 4, 7, 11),
        'min7': (0, 3, 7, 10),
        'sus2': (0, 2, 7),
        'sus4': (0, 5, 7),
        'dominant7': (0, 4, 7, 10),
        'maj6': (0, 4, 7, 9),
        'maj9': (0, 4, 7, 11, 14),
        'min9': (0, 3, 7, 10, 14),
    }
    return [root_note + interval for interval in chord_intervals[chord_type]]

def invert_chord(chord, inversion):
    chord = chord[:]
    for _ in range(inversion):
        chord.append(chord.pop(0) + 12)
    return chord

def play_chord(midi, chord):
    for note in chord:
        midi.send(NoteOn(note, 127))

def stop_chord(midi, chord):
    for note in chord:
        midi.send(NoteOff(note, 0))