import bpy
import math
import os

ROOT = r"C:\Users\ZenoxArt\Documents\!CODING\.claude\worktrees\nintendo-ds-homebrew-game-e07d4e\NDSGame\assets\Starfighter"
OBJ_PATH = os.path.join(ROOT, "Starfighter.obj")
OUT_DIR = os.path.join(ROOT, "render")
os.makedirs(OUT_DIR, exist_ok=True)

FRAME_COUNT = 48
RES = 96
TARGET_TRIS = 1400

# Background color chosen to match the HUD panel's gradient tone at the
# viewport's vertical midpoint (see hud.c's buildPalette/drawBackground),
# so the rendered square blends reasonably into the surrounding bottom
# screen UI instead of looking like a foreign rectangle.
BG_COLOR = (0.094, 0.161, 0.549, 1.0)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=OBJ_PATH)

obj = [o for o in bpy.context.selected_objects if o.type == 'MESH'][0]
bpy.context.view_layer.objects.active = obj

# Center the mesh on its own bounding-box center and normalize scale so
# framing is independent of the model's native (arbitrary) unit size.
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
obj.location = (0, 0, 0)
dims = obj.dimensions
maxdim = max(dims.x, dims.y, dims.z)
scale = 1.6 / maxdim
obj.scale = (scale, scale, scale)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

# Decimate to a low-poly count. Ratio is approximate (Decimate's collapse
# algorithm doesn't hit an exact face count), so an extra tweak pass
# nudges it closer to the target.
face_count = len(obj.data.polygons)
ratio = min(1.0, TARGET_TRIS / face_count)
mod = obj.modifiers.new(name="Decimate", type='DECIMATE')
mod.ratio = ratio
bpy.ops.object.modifier_apply(modifier=mod.name)
print(f"DECIMATED_FACES={len(obj.data.polygons)} (target {TARGET_TRIS}, ratio {ratio:.5f})")

# Camera: orthographic, fixed, slightly elevated 3/4 view. The model itself
# rotates in place ("um die eigene Achse") rather than the camera orbiting.
cam_data = bpy.data.cameras.new("Cam")
cam_data.type = 'ORTHO'
cam_data.ortho_scale = 2.1
cam_obj = bpy.data.objects.new("Cam", cam_data)
bpy.context.collection.objects.link(cam_obj)
cam_obj.location = (0, -3.2, 1.1)
direction = -cam_obj.location
cam_obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
bpy.context.scene.camera = cam_obj

# Lighting: key sun + soft fill so the hull reads clearly at a tiny resolution.
sun = bpy.data.lights.new("Sun", type='SUN')
sun.energy = 3.0
sun_obj = bpy.data.objects.new("Sun", sun)
sun_obj.location = (2, -2, 3)
sun_obj.rotation_euler = (math.radians(50), 0, math.radians(35))
bpy.context.collection.objects.link(sun_obj)

fill = bpy.data.lights.new("Fill", type='SUN')
fill.energy = 1.0
fill_obj = bpy.data.objects.new("Fill", fill)
fill_obj.rotation_euler = (math.radians(110), 0, math.radians(-120))
bpy.context.collection.objects.link(fill_obj)

world = bpy.data.worlds.new("World")
world.use_nodes = True
bg_node = world.node_tree.nodes["Background"]
bg_node.inputs[0].default_value = BG_COLOR
bpy.context.scene.world = world

# Turntable: rotate the model itself around its own vertical (Z) axis.
obj.rotation_mode = 'XYZ'
scene = bpy.context.scene
scene.frame_start = 0
scene.frame_end = FRAME_COUNT - 1
obj.keyframe_insert(data_path="rotation_euler", frame=0)
obj.rotation_euler = (0, 0, 2 * math.pi)
obj.keyframe_insert(data_path="rotation_euler", frame=FRAME_COUNT)
for fc in obj.animation_data.action.fcurves:
    for kp in fc.keyframe_points:
        kp.interpolation = 'LINEAR'
    fc.extrapolation = 'LINEAR'

scene.render.engine = 'BLENDER_EEVEE_NEXT'
scene.render.resolution_x = RES
scene.render.resolution_y = RES
scene.render.film_transparent = False
scene.view_settings.view_transform = 'Standard'
scene.render.image_settings.file_format = 'PNG'
scene.eevee.taa_render_samples = 32

for f in range(FRAME_COUNT):
    scene.frame_set(f)
    scene.render.filepath = os.path.join(OUT_DIR, f"frame_{f:04d}.png")
    bpy.ops.render.render(write_still=True)

print(f"RENDERED {FRAME_COUNT} frames to {OUT_DIR}")
