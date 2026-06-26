"""
make_shell.py - turn the base Zeus x27 STL into a printable, hollow, two-part
shell with (optionally) a screen window and USB-C slot.

Run headless:
    blender --background --python make_shell.py
or open it in Blender's Scripting tab and Run (recommended first time, so you can
read the cut coordinates off the model visually).

Pipeline:
    import STL -> auto-scale to real mm -> clean -> (optional REMESH to manifold)
    -> SOLIDIFY (hollow shell wall) -> (optional boolean screen + USB cuts)
    -> BISECT into left/right halves -> export STL (mm).
Outputs: cad/output/zeus_left.stl, zeus_right.stl

FIRST RUN: leave DO_CUTS = False. The script prints the model bounds. Open the
model in Blender, hover over the StatTrak numeral panel and the grip/USB spot to
read coordinates, fill in SCREEN_* / USB_* below, set DO_CUTS = True, re-run.
If the booleans collapse the mesh (tiny triangle count), set USE_REMESH = True.
"""

import bpy, bmesh, os

# ---------------------------------------------------------------- parameters
HERE    = os.path.dirname(os.path.abspath(__file__))
STL_IN  = os.path.join(HERE, "..", "taser-zeus-x27-gun-model-cs2", "zeus.stl")
OUT_DIR = os.path.join(HERE, "output")

TARGET_LONGEST_MM = 247.0   # real size: scale so the longest dimension = this (mm)
WALL_MM           = 2.0     # shell wall thickness
SPLIT_AXIS        = "Z"     # split plane normal. Z = thickness axis (left/right halves)

USE_REMESH   = True         # this base mesh is non-manifold -> remesh needed for clean shell
OCTREE_DEPTH = 9            # Remesh detail (~0.5mm at depth 9; +1 = 2x finer/heavier)

DO_CUTS      = False        # True once SCREEN_*/USB_* are set for this model's orientation
# Coordinates are in mm, in the SCALED model space. Read them in Blender first.
SCREEN_CENTER = (0.0, 0.0, 0.0)
SCREEN_SIZE   = (26.0, 15.0, 40.0)   # X,Y = opening (panel + bezel); last = pierce depth
USB_CENTER    = (0.0, 0.0, 0.0)
USB_SIZE      = (14.0, 9.0, 40.0)
# ---------------------------------------------------------------------------


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for coll in (bpy.data.meshes, bpy.data.objects):
        for b in list(coll):
            try: coll.remove(b)
            except Exception: pass


def activate(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def import_stl():
    try:
        bpy.ops.wm.stl_import(filepath=STL_IN)          # Blender 4.x/5.x
    except AttributeError:
        bpy.ops.import_mesh.stl(filepath=STL_IN)        # Blender <4.0
    obj = bpy.context.view_layer.objects.active or bpy.context.selected_objects[0]
    obj.name = "zeus"
    return obj


def prep(obj):
    # Auto-scale to real mm based on the longest dimension.
    longest = max(obj.dimensions)
    factor = TARGET_LONGEST_MM / longest if longest else 1.0
    obj.scale = (factor, factor, factor)
    activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # Clean: merge doubles, close open holes (-> closed surface so SOLIDIFY behaves
    # instead of spiking), consistent outward normals.
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.01)
    bpy.ops.mesh.fill_holes(sides=0)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")


def manifold_report(obj):
    bm = bmesh.new(); bm.from_mesh(obj.data)
    nonman = sum(1 for e in bm.edges if not e.is_manifold)
    bm.free()
    print("non-manifold edges: %d  (%s)" % (
        nonman, "manifold solid - remesh not needed" if nonman == 0
        else "NOT a closed solid - set USE_REMESH=True if booleans fail"))
    return nonman


def remesh(obj):
    activate(obj)
    m = obj.modifiers.new("rem", "REMESH")
    m.mode = "SHARP"
    m.octree_depth = OCTREE_DEPTH
    m.sharpness = 1.0
    m.use_remove_disconnected = True
    bpy.ops.object.modifier_apply(modifier=m.name)


def solidify(obj):
    activate(obj)
    m = obj.modifiers.new("solid", "SOLIDIFY")
    m.thickness = WALL_MM
    m.offset = -1.0
    m.use_even_offset = False   # Even Thickness spikes on sharp/thin areas -> off
    bpy.ops.object.modifier_apply(modifier=m.name)


def cube(name, center, size):
    bpy.ops.mesh.primitive_cube_add(location=center)
    c = bpy.context.active_object
    c.name = name
    c.scale = (size[0] / 2.0, size[1] / 2.0, size[2] / 2.0)
    bpy.ops.object.transform_apply(scale=True)
    return c


def boolean_difference(obj, cutter):
    activate(obj)
    m = obj.modifiers.new("bool", "BOOLEAN")
    m.operation = "DIFFERENCE"
    m.solver = "EXACT"
    m.object = cutter
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def export_stl(obj, path):
    activate(obj)
    try:
        bpy.ops.wm.stl_export(filepath=path, export_selected_objects=True)
    except AttributeError:
        bpy.ops.export_mesh.stl(filepath=path, use_selection=True)
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print("wrote %s  (%d triangles)" % (path, tris))


def bisect_export(obj, mid):
    os.makedirs(OUT_DIR, exist_ok=True)
    axis = {"X": 0, "Y": 1, "Z": 2}[SPLIT_AXIS]
    normal = [0.0, 0.0, 0.0]; normal[axis] = 1.0
    plane_co = [0.0, 0.0, 0.0]; plane_co[axis] = mid
    for side, clear_outer in (("left", True), ("right", False)):
        me = obj.data.copy()
        half = bpy.data.objects.new("zeus_" + side, me)
        bpy.context.collection.objects.link(half)
        bm = bmesh.new(); bm.from_mesh(me)
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(
            bm, geom=geom, dist=0.0,
            plane_co=plane_co, plane_no=normal,
            clear_outer=clear_outer, clear_inner=not clear_outer,
        )
        bm.to_mesh(me); bm.free()
        export_stl(half, os.path.join(OUT_DIR, "zeus_%s.stl" % side))
        bpy.data.objects.remove(half, do_unlink=True)


def main():
    clear_scene()
    obj = import_stl()
    prep(obj)
    d = obj.dimensions
    print("scaled dims (mm): X=%.1f Y=%.1f Z=%.1f" % (d.x, d.y, d.z))
    bb = obj.bound_box
    xs = [v[0] for v in bb]; ys = [v[1] for v in bb]; zs = [v[2] for v in bb]
    print("bounds mm: X[%.1f,%.1f] Y[%.1f,%.1f] Z[%.1f,%.1f]" % (
        min(xs), max(xs), min(ys), max(ys), min(zs), max(zs)))
    manifold_report(obj)

    if USE_REMESH:
        remesh(obj)
        print("after remesh: %d polys" % len(obj.data.polygons))
    # Capture the split plane from the clean (pre-solidify) bounds, so any solidify
    # artifact verts can't skew it.
    axis = {"X": 0, "Y": 1, "Z": 2}[SPLIT_AXIS]
    vals = [v[axis] for v in obj.bound_box]
    split_mid = (min(vals) + max(vals)) / 2.0
    solidify(obj)
    if DO_CUTS:
        boolean_difference(obj, cube("screen_cut", SCREEN_CENTER, SCREEN_SIZE))
        boolean_difference(obj, cube("usb_cut", USB_CENTER, USB_SIZE))
        print("after cuts: %d polys" % len(obj.data.polygons))
    else:
        print("DO_CUTS=False -> no screen/USB holes. Set coords from the bounds above, then enable.")
    bisect_export(obj, split_mid)
    print("done. Check cad/output/.")


if __name__ == "__main__":
    main()
