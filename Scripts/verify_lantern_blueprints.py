import json
from pathlib import Path
import unreal


LEVEL = "/Game/Main"
DEST_DIR = "/Game/c4d/rantan/models"
RESULT = Path(unreal.Paths.project_saved_dir()) / "LanternBlueprintVerification.json"
TARGETS = [
    ("BP_Lantern_A_02", "Shape_A2"), ("BP_Lantern_A_03", "Shape_A3"),
    ("BP_Lantern_B_01", "Shape_B"), ("BP_Lantern_B_02", "Shape_B2"),
    ("BP_Lantern_C_01", "Shape_C"), ("BP_Lantern_C_02", "Shape_C2"),
    ("BP_Lantern_C_03", "Shape_C3"), ("BP_Lantern_D_01", "Shape_D"),
    ("BP_Lantern_D_02", "Shape_D2"),
]
LIGHT_PROPERTIES = [
    "intensity", "light_color", "temperature", "use_temperature", "attenuation_radius",
    "inner_cone_angle", "outer_cone_angle", "source_radius", "soft_source_radius",
    "source_length", "inverse_exposure_blend", "cast_shadows", "cast_volumetric_shadow",
    "indirect_lighting_intensity", "volumetric_scattering_intensity", "mobility",
]


def close(a, b, tolerance=0.001):
    return abs(float(a) - float(b)) <= tolerance


def same_vector(a, b):
    return close(a.x, b.x) and close(a.y, b.y) and close(a.z, b.z)


def find_parts(shape):
    parts = {}
    for child in shape.get_attached_actors():
        label = child.get_actor_label()
        lights = child.get_components_by_class(unreal.SpotLightComponent)
        meshes = child.get_components_by_class(unreal.StaticMeshComponent)
        if lights:
            parts["SpotLight"] = lights[0]
        elif meshes:
            if "LanternBody" in label:
                parts["LanternBody_Paper"] = meshes[0]
            elif "Tassel" in label:
                parts["Tassel_Paper"] = meshes[0]
            elif "Frame" in label:
                parts["Frame"] = meshes[0]
    return parts


def template_data(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    root = None
    values = {}
    attachments = {}
    seen = set()
    for handle in handles:
        data = library.get_data(handle)
        obj = library.get_object_for_blueprint(data, bp)
        if not obj:
            continue
        if library.is_root_component(data):
            root = handle
        path = obj.get_path_name()
        if path in seen:
            continue
        seen.add(path)
        name = str(library.get_variable_name(data))
        values[name] = obj
        attachments[name] = (data, handle)
    return library, root, values, attachments


def expected_relative(source, pivot):
    sw = source.get_world_transform()
    pw = pivot.get_world_transform()
    return unreal.Vector(sw.translation.x - pw.translation.x,
                         sw.translation.y - pw.translation.y,
                         sw.translation.z - pw.translation.z)


def main():
    errors = []
    checks = []
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL):
        raise RuntimeError("Could not load Main")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    by_label = {a.get_actor_label(): a for a in actors}

    for asset_name, shape_name in TARGETS:
        shape = by_label.get(shape_name)
        if not shape:
            errors.append(asset_name + ": source Shape missing")
            continue
        parts = find_parts(shape)
        bp_path = DEST_DIR + "/" + asset_name
        bp = unreal.EditorAssetLibrary.load_asset(bp_path)
        if not bp:
            errors.append(asset_name + ": Blueprint missing")
            continue
        library, root, templates, attachment_data = template_data(bp)
        if root is None or "Shape_A" not in templates:
            errors.append(asset_name + ": sample-compatible Shape_A root missing")
            continue
        required = ["Frame", "LanternBody_Paper", "Tassel_Paper", "SpotLight"]
        for name in required:
            if name not in templates:
                errors.append(asset_name + ": missing component " + name)
                continue
            data, _ = attachment_data[name]
            if not library.is_attached_to(data, root):
                errors.append(asset_name + ": component not attached to root: " + name)

        if any(name not in templates for name in required):
            continue
        pivot = parts["Frame"]
        for name in required:
            actual = templates[name].get_editor_property("relative_location")
            expected = expected_relative(parts[name], pivot)
            if not same_vector(actual, expected):
                errors.append("%s: %s relative location mismatch" % (asset_name, name))
        for name in ["Frame", "LanternBody_Paper", "Tassel_Paper"]:
            actual = templates[name]
            source = parts[name]
            if actual.get_editor_property("static_mesh") != source.get_editor_property("static_mesh"):
                errors.append("%s: %s mesh mismatch" % (asset_name, name))
            if actual.get_material(0) != source.get_material(0):
                errors.append("%s: %s material mismatch" % (asset_name, name))
        for prop in LIGHT_PROPERTIES:
            source_value = parts["SpotLight"].get_editor_property(prop)
            actual_value = templates["SpotLight"].get_editor_property(prop)
            if prop == "light_color":
                matches = all(getattr(source_value, channel) == getattr(actual_value, channel)
                              for channel in ["r", "g", "b", "a"])
            elif isinstance(source_value, float):
                matches = close(source_value, actual_value)
            else:
                matches = source_value == actual_value
            if not matches:
                errors.append("%s: SpotLight %s mismatch" % (asset_name, prop))
        checks.append({"blueprint": bp_path, "source": shape_name, "components": required, "pass": True})

    result = {
        "pass": not errors,
        "errors": errors,
        "checked": checks,
        "source_shapes_retained": all(name in by_label for _, name in TARGETS),
        "main_level_saved_by_workflow": False,
    }
    RESULT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    if errors:
        raise RuntimeError("Lantern Blueprint verification failed: " + " | ".join(errors))
    unreal.log("LANTERN_BLUEPRINT_VERIFICATION_PASS=" + str(RESULT))


main()
