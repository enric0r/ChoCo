import time

def handle_inversion_assignment(keys, key_map, chord_inversions):
    print("Inversion assignment mode: Press a chord key to cycle its inversion.")
    while True:
        event = keys.events.get()
        if event:
            mapped_key = key_map[event.key_number]
            if event.pressed and mapped_key is not None and mapped_key < 7:
                chord_inversions[mapped_key] = (chord_inversions[mapped_key] + 1) % 4
                print(f"Chord key {mapped_key} inversion set to {chord_inversions[mapped_key]}")
            elif event.released and mapped_key == 10:
                break
        time.sleep(0.05)