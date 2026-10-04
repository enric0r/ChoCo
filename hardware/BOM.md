# ChoCo prototype BOM

This records the hardware described by the repository and visible in the prototype
photos, for **one controller**. It is the starting point for documenting the
builder's actual parts. Exact purchase links, manufacturers and variants remain
to be confirmed; a library label is not proof of the part purchased.

## Electronic parts

| Qty | Part | Reference | Evidence / assembly detail |
| --- | --- | --- | --- |
| 1 | Raspberry Pi Pico, RP2040 | A1 | Schematic value `RaspberryPi_Pico`; PCB footprint `ScottoKeebs_MCU:Raspberry_Pi_Pico`; module visible in the front photo. The symbol description also mentions Pico 2, but this does not establish firmware support for Pico 2. |
| 1 | 128×64 I²C OLED module | J1 | Schematic value `OLED_128x64`; footprint `ScottoKeebs_Components:OLED_128x64`. Firmware targets SSD1306 at `0x3C`. The connector reference represents the display module, not a second display. Exact module size and supplier are not recorded. |
| 1 | Two-axis analog joystick with push switch | ALPS_3318_DS4_Joystick1 (schematic) | Footprint `PS4_Joystick:PS4_joystick`. The PCB uses `REF**` and value `XDCR_COM-09032`; these inconsistent labels are not treated as a confirmed manufacturer part number. Confirm the installed joystick before using a purchase link. |
| 10 | Mechanical key switches | S1–S10 | `Scotto_MX:MX_PCB_1.00u` footprints. Switch brand, variant and operating force are not recorded. |
| 10 | Matrix diodes, axial DO-35 | D1–D10 | Value `Diode`, footprint `Scotto:Diode_DO-35`. The symbol description mentions 1N4148 / 1N4148W, but the PCB footprint is axial DO-35, not the SOD-123 package also mentioned by that generic description. Exact installed diode part number is unconfirmed. |

The schematic contains **23 physical electronic items in five groups**. It has no
populated manufacturer/MPN fields. Counts and footprints above were extracted
from the schematic and compared with the PCB; this is an inventory consistency
check, not an electrical or manufacturer-datasheet validation.

## Board and assembly items

| Qty | Item | Evidence / what remains to record |
| --- | --- | --- |
| 1 | ChoCo PCB | Front photo shows REV-02; source PCB and a REV-02 archive are included. Check the actual revision before fabrication. |
| 10 | Keycaps | Front photo shows 3 dark, 3 brown and 4 light caps. Material, dimensions and purchase or fabrication source are unconfirmed. |
| 1 assembly | Enclosure / base | Visible in front/back photos. Material, fabrication files and method are not established by these KiCad files. |
| 4 visible positions | Mounting hardware | PCB has H1–H4, `MountingHole_2.2mm_M2`; fasteners are visible in the photos. Actual screw length, nuts and spacers need confirmation. Hole footprints are not four extra electronic components. |
| As fitted | Headers / sockets | Pico and OLED connection method and separate header quantities need confirmation; do not double-count headers already supplied with a module. |
| 1 | USB data cable | Required for power and USB MIDI; choose the connector matching the installed module. |

## Details to add from the actual build

- Purchase links or manufacturer/model for the Pico, OLED, joystick and switches.
- Installed diode model and manufacturer, if known.
- Keycap and enclosure source: purchased parts or fabrication files/materials.
- Headers, sockets, screw lengths and spacers actually fitted.

Prices, stock and alternative parts are intentionally omitted: this page records
the prototype rather than selecting replacement parts or preparing an order.

## Sources

- [KiCad schematic](ChoCo.kicad_sch): quantities, values and intended footprints.
- [KiCad PCB](ChoCo.kicad_pcb): placed footprints and four mounting holes.
- [Front photo](front.jpg) and [back photo](back.jpg): visible assembly and REV-02 marking.
- [Firmware configuration](../lib/Config/Config.h): display and input configuration.
- [Project README](../README.md#prototype-bill-of-materials): short BOM.

When a purchased part is confirmed, update this page and the English/Italian
handbook together. Add confirmed manufacturer/MPN properties to the schematic
when maintaining the electrical BOM; do not infer them from a footprint name.
