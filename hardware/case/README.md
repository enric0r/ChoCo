# ChoCo case

Three STL parts from the Fusion 360 design of the ChoCo prototype: an open top frame, a bottom with the chocolate-bar pattern, and a joystick cap.

![Back of the assembled ChoCo prototype](../back.jpg)

## Files

Download one copy of each part. On a GitHub STL page, use **Download raw file** to save the model.

| File | Part | Bounding dimensions at millimeter scale (X × Y × Z) |
| --- | --- | --- |
| [ChoCo-top.stl](ChoCo-top.stl) | Open top frame | 148.5 × 70.5 × 7 mm |
| [ChoCo-bar-bottom.stl](ChoCo-bar-bottom.stl) | Chocolate-bar bottom | 148.5 × 70.5 × 7 mm |
| [ChoCo-thumbstick.stl](ChoCo-thumbstick.stl) | Joystick cap | 14.5 × 14.5 × 7.5 mm |

STL does not encode units. Import these files as **millimeters**, at **100% scale**, and check the dimensions above in your slicer.

Each file contains one connected mesh. The parts have been separated from the multi-body exports and translated to center X/Y with the lowest point at Z=0. Their shape, scale and orientation are preserved. This origin placement does not specify the best printing orientation.

## Printing and assembly

The photos show the assembled prototype, printed on a **Bambu Lab P2S**. **PLA** is the intended material. The original prints used PLA and/or PLA+; the exact filament or mix was not recorded.

Layer height, supports and fastening details have not yet been documented, and no tested slicer profile is included.

Check the orientation in your slicer and the fit of the frame, mounting holes and joystick socket against your components. The [front photo](../front.jpg) shows the placement of the frame and joystick cap; the photo above shows the outer face of the bottom.

The mesh checks confirm closed, consistently connected surfaces; they do not verify printer tolerances or assembly fit.

## Sharing and source files

These STL files can be used for a model upload on MakerWorld or Printables. A MakerWorld print profile needs a project saved from Bambu Studio with the intended printer and print settings; the STL files themselves do not include those settings.

The editable Fusion 360 project is not included yet. Links to the MakerWorld and Printables listings will be added once those listings are published.

See the repository [LICENSE](../../LICENSE) and the [ChoCo handbook](https://enric0r.github.io/choco.github.io/docs/) for the rest of the project.
