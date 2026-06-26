"""
make_shell.py - turn the CS2 Zeus x27 GLB into a printable, hollow, two-part
shell with a screen window and USB-C slot.

Run headless:
    blender --background --python make_shell.py
or open Blender, load this in the Scripting tab, and Run (so you can eyeball /
tweak the cut positions visually before exporting).

The booleans operate on a heavy game mesh, so treat the coordinates below as a
starting point: import the GLB once, hover over the numeral panel / grip to read
real coordinates, then adjust SCREEN_* and USB_* and re-run. Outputs land in
cad/output/zeus_left.stl and zeus_right.stl (millimetres).
"""

import bpy, os, math

# ---------------------------------------------------------------- parameters
HERE       = os.path.dirname(os.path.abspath(__file__))
GLB        = os.path.join(HERE, "..", "taser-zeus-x27-gun-model-cs2", "source", "TASER.glb")
OUT_DIR    = os.path.join(HERE, "output")

SCALE_MM       = 1000.0   # glTF is in metres; *1000 -> millimetres for printing
DECIMATE_RATIO = 0.35     # 0<r<=1, lower = fewer polys = faster/robuster booleans (0 to skip)
WALL_MM        = 2.0      # shell wall thickness

# Screen window: centred on the in-game StatTrak numeral plane (mm, post-scale).
# Numeral plane measured from the GLB: ~ (0, 152.2, -40.8), 21 x 8 mm, facing -Z.
SCREEN_CENTER  = (0.0, 152.2, -30.0)   # Z pushed out so the cutter pierces the front wall
SCREEN_SIZE    = (26.0, 15.0, 45.0)    # X,Y = window opening (panel + bezel); Z = pierce depth

# USB-C slot: ADJUST after looking at the model (grip base / rear). mm.
USB_CENTER     = (0.0, 12.0, 55.0)
USB_SIZE       = (14.0, 9.0, 40.0)

SPLIT_AXIS     = "X"      # weapon is symmetric about X=0 -> clean left/right halves
# ---------------------------------------------------------------------------


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for block in (bpy.data.meshes, bpy.data.objects):
        for b in list(block):
            try: block.remove(b)
            except Exception: pass


def import_and_prep():
    bpy.ops.import_scene.gltf(filepath=GLB)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not meshes:
        raise RuntimeError("No mesh imported from GLB")
    # Join everything into one object.
    bpy.ops.object.select_all(action="DESELECT")
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = "zeus"

    # Metres -> millimetres, apply transforms, recentre transforms on geometry.
    obj.scale = (SCALE_MM, SCALE_MM, SCALE_MM)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    # Recalculate normals outward.
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def decimate(obj):
    if DECIMATE_RATIO and DECIMATE_RATIO < 1.0:
        m = obj.modifiers.new("dec", "DECIMATE")
        m.ratio = DECIMATE_RATIO
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=m.name)


def solidify(obj):
    """Surface mesh -> walled shell."""
    m = obj.modifiers.new("solid", "SOLIDIFY")
    m.thickness = WALL_MM
    m.offset = -1.0               # keep the outer surface, grow walls inward
    m.use_even_offset = True
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=m.name)


def cube(name, center, size):
    bpy.ops.mesh.primitive_cube_add(location=center)
    c = bpy.context.active_object
    c.name = name
    c.scale = (size[0] / 2.0, size[1] / 2.0, size[2] / 2.0)
    bpy.ops.object.transform_apply(scale=True)
    return c


def boolean(obj, cutter, op):
    m = obj.modifiers.new("bool", "BOOLEAN")
    m.operation = op             # 'DIFFERENCE' or 'INTERSECT'
    m.solver = "EXACT"
    m.object = cutter
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def export_stl(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    try:
        bpy.ops.wm.stl_export(filepath=path, export_selected_objects=True)   # Blender 4.x
    except AttributeError:
        bpy.ops.export_mesh.stl(filepath=path, use_selection=True)           # Blender <4.0
    print("wrote", path)


def split_and_export(obj):
    os.makedirs(OUT_DIR, exist_ok=True)
    axis = {"X": 0, "Y": 1, "Z": 2}[SPLIT_AXIS]
    BIG = 1000.0
    for side, sign in (("left", -1), ("right", 1)):
        half = obj.copy()
        half.data = obj.data.copy()
        bpy.context.collection.objects.link(half)
        center = [0, 0, 0]
        center[axis] = sign * BIG / 2.0
        box = cube(f"halfbox_{side}", tuple(center), (BIG, BIG, BIG))
        boolean(half, box, "INTERSECT")
        export_stl(half, os.path.join(OUT_DIR, f"zeus_{side}.stl"))
        bpy.data.objects.remove(half, do_unlink=True)


def main():
    clear_scene()
    obj = import_and_prep()
    print("imported dims (mm):", tuple(round(d, 1) for d in obj.dimensions))
    decimate(obj)
    solidify(obj)
    boolean(obj, cube("screen_cut", SCREEN_CENTER, SCREEN_SIZE), "DIFFERENCE")
    boolean(obj, cube("usb_cut", USB_CENTER, USB_SIZE), "DIFFERENCE")
    split_and_export(obj)
    print("done. Check cad/output/. Tweak SCREEN_*/USB_* and re-run if cuts are off.")


if __name__ == "__main__":
    main()
