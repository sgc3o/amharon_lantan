import json
from pathlib import Path

import unreal


LEVEL = "/Game/Main"
SAMPLE = "/Game/c4d/rantan/models/BP_Lantern_A_01"
DEST_DIR = "/Game/c4d/rantan/models"
REPORT = Path(unreal.Paths.project_saved_dir()) / "LanternBlueprintCreationReport.json"

# Shape_C3 is intentionally C_03.  Main contains no third D Shape.
TARGETS = [
    ("BP_Lantern_A_02", "Shape_A2"),
    ("BP_Lantern_A_03", "Shape_A3"),
    ("BP_Lantern_B_01", "Shape_B"),
    ("BP_Lantern_B_02", "Shape_B2"),
    ("BP_Lantern_C_01", "Shape_C"),
    ("BP_Lantern_C_02", "Shape_C2"),
    ("BP_Lantern_C_03", "Shape_C3"),
    ("BP_Lantern_D_01", "Shape_D"),
    ("BP_Lantern_D_02", "Shape_D2"),
]

MESH_PROPERTIES = ["visible", "hidden_in_game", "cast_shadow"]
LIGHT_PROPERTIES = [
    "mobility", "visible", "hidden_in_game", "intensity", "light_color",
    "temperature", "use_temperature", "attenuation_radius", "inner_cone_angle",
    "outer_cone_angle", "source_radius", "soft_source_radius", "source_length",
    "inverse_exposure_blend", "cast_shadows", "cast_volumetric_shadow",
    "indirect_lighting_intensity", "volumetric_scattering_intensity",
    "light_falloff_exponent", "use_inverse_squared_falloff", "ies_texture",
    "use_ies_brightness", "ies_brightness_scale", "affect_translucent_lighting",
    "transmission", "specular_scale", "shadow_bias", "shadow_slope_bias",
    "contact_shadow_length", "contact_shadow_length_in_world_space",
]


def path_name(value):
    return value.get_path_name() if value else None


def vec(value):
    return {"x": value.x, "y": value.y, "z": value.z}


def rot(value):
    return {"pitch": value.pitch, "yaw": value.yaw, "roll": value.roll}


def copy_properties(source, target, names):
    copied = []
    skipped = []
    for name in names:
        try:
            value = source.get_editor_property(name)
        except Exception:
            continue
        try:
            target.set_editor_property(name, value)
            copied.append(name)
        except Exception as exc:
            skipped.append({"property": name, "reason": str(exc)})
    return copied, skipped


def attached_children(actor):
    return list(actor.get_attached_actors())


def find_source_parts(shape):
    parts = {}
    source_actor_labels = {}
    for child in attached_children(shape):
        label = child.get_actor_label()
        static_mesh_components = child.get_components_by_class(unreal.StaticMeshComponent)
        spot_components = child.get_components_by_class(unreal.SpotLightComponent)
        if spot_components:
            parts["SpotLight"] = spot_components[0]
            source_actor_labels["SpotLight"] = label
        elif static_mesh_components:
            if "LanternBody" in label:
                key = "LanternBody_Paper"
            elif "Tassel" in label:
                key = "Tassel_Paper"
            elif "Frame" in label:
                key = "Frame"
            else:
                continue
            parts[key] = static_mesh_components[0]
            source_actor_labels[key] = label
    required = {"Frame", "LanternBody_Paper", "Tassel_Paper", "SpotLight"}
    missing = sorted(required - set(parts))
    if missing:
        raise RuntimeError("%s missing parts: %s" % (shape.get_actor_label(), missing))
    return parts, source_actor_labels


def normalized_transform(component, pivot_component):
    source_world = component.get_world_transform()
    pivot_world = pivot_component.get_world_transform()
    source_location = source_world.translation
    pivot_location = pivot_world.translation
    pivot_scale = pivot_world.scale3d
    source_scale = source_world.scale3d
    location = unreal.Vector(
        source_location.x - pivot_location.x,
        source_location.y - pivot_location.y,
        source_location.z - pivot_location.z,
    )
    scale = unreal.Vector(
        source_scale.x / pivot_scale.x,
        source_scale.y / pivot_scale.y,
        source_scale.z / pivot_scale.z,
    )
    # All source Frame pivots are identity-rotated. Keeping the source world rotation
    # therefore produces the exact child-relative rotation used by the sample's pivot model.
    rotation = source_world.rotation.rotator()
    return location, rotation, scale


def blueprint_objects(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    by_name = {}
    root_handle = None
    seen_paths = set()
    for handle in handles:
        data = library.get_data(handle)
        obj = library.get_object_for_blueprint(data, bp)
        if not obj:
            continue
        if library.is_root_component(data):
            root_handle = handle
        object_path = obj.get_path_name()
        if object_path in seen_paths:
            continue
        seen_paths.add(object_path)
        by_name[str(library.get_variable_name(data))] = obj
    if root_handle is None:
        raise RuntimeError("Blueprint root component handle was not found: " + bp.get_path_name())
    return subsystem, library, root_handle, by_name


def set_relative_transform(template, location, rotation, scale):
    template.set_editor_property("relative_location", location)
    template.set_editor_property("relative_rotation", rotation)
    template.set_editor_property("relative_scale3d", scale)


def component_report(template, source_label):
    data = {
        "component": template.get_name().replace("_GEN_VARIABLE", ""),
        "class": template.get_class().get_name(),
        "source_actor": source_label,
        "relative_location": vec(template.get_editor_property("relative_location")),
        "relative_rotation": rot(template.get_editor_property("relative_rotation")),
        "relative_scale": vec(template.get_editor_property("relative_scale3d")),
    }
    if isinstance(template, unreal.StaticMeshComponent):
        mesh = template.get_editor_property("static_mesh")
        data["mesh"] = path_name(mesh)
        data["materials"] = [path_name(template.get_material(i)) for i in range(template.get_num_materials())]
    if isinstance(template, unreal.SpotLightComponent):
        for name in ["intensity", "attenuation_radius", "inner_cone_angle", "outer_cone_angle", "source_radius"]:
            data[name] = template.get_editor_property(name)
    return data


def main():
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL):
        raise RuntimeError("Could not load Main level")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors_by_label = {actor.get_actor_label(): actor for actor in actor_subsystem.get_all_level_actors()}
    sample = unreal.EditorAssetLibrary.load_asset(SAMPLE)
    if not sample:
        raise RuntimeError("Sample Blueprint was not found: " + SAMPLE)

    existing = [name for name, _ in TARGETS if unreal.EditorAssetLibrary.does_asset_exist(DEST_DIR + "/" + name)]
    if existing:
        raise RuntimeError("Refusing to overwrite existing target assets: " + ", ".join(existing))

    report = {
        "sample_blueprint": SAMPLE,
        "level_saved": False,
        "source_actors_deleted": False,
        "created": [],
    }

    for asset_name, shape_label in TARGETS:
        shape = actors_by_label.get(shape_label)
        if not shape:
            raise RuntimeError("Source Shape was not found: " + shape_label)
        parts, source_labels = find_source_parts(shape)
        destination = DEST_DIR + "/" + asset_name
        bp = unreal.EditorAssetLibrary.duplicate_asset(SAMPLE, destination)
        if not bp:
            raise RuntimeError("Failed to duplicate sample to " + destination)

        subsystem, library, root_handle, templates = blueprint_objects(bp)
        required_templates = {"Frame", "LanternBody_Paper", "Tassel_Paper"}
        missing_templates = sorted(required_templates - set(templates))
        if missing_templates:
            raise RuntimeError("%s missing sample templates: %s" % (asset_name, missing_templates))

        skipped_properties = []
        pivot = parts["Frame"]
        for component_name in ["Frame", "LanternBody_Paper", "Tassel_Paper"]:
            source_component = parts[component_name]
            template = templates[component_name]
            template.set_editor_property("static_mesh", source_component.get_editor_property("static_mesh"))
            for material_index in range(source_component.get_num_materials()):
                template.set_material(material_index, source_component.get_material(material_index))
            location, rotation, scale = normalized_transform(source_component, pivot)
            set_relative_transform(template, location, rotation, scale)
            _, skipped = copy_properties(source_component, template, MESH_PROPERTIES)
            skipped_properties.extend(skipped)

        params = unreal.AddNewSubobjectParams(
            parent_handle=root_handle,
            new_class=unreal.SpotLightComponent,
            blueprint_context=bp,
            conform_transform_to_parent=False,
        )
        light_handle, fail_reason = subsystem.add_new_subobject(params)
        if not library.is_handle_valid(light_handle):
            raise RuntimeError("Failed to add SpotLight to %s: %s" % (asset_name, fail_reason))
        if not subsystem.rename_subobject(light_handle, unreal.Name("SpotLight")):
            raise RuntimeError("Failed to rename SpotLight component in " + asset_name)
        light_data = library.get_data(light_handle)
        light_template = library.get_object_for_blueprint(light_data, bp)
        if not light_template:
            raise RuntimeError("Could not resolve new SpotLight template in " + asset_name)
        location, rotation, scale = normalized_transform(parts["SpotLight"], pivot)
        set_relative_transform(light_template, location, rotation, scale)
        copied_light, skipped = copy_properties(parts["SpotLight"], light_template, LIGHT_PROPERTIES)
        skipped_properties.extend(skipped)

        # Motion Design Cloners are movable. Match the working BP_Lantern_A_01:
        # every scene component, including the root and light, must be Movable.
        _, _, _, all_templates = blueprint_objects(bp)
        for template in all_templates.values():
            if isinstance(template, unreal.SceneComponent):
                template.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        if not unreal.EditorAssetLibrary.save_asset(destination, only_if_is_dirty=False):
            raise RuntimeError("Failed to save " + destination)

        # Re-resolve templates after compile for the final report.
        _, _, _, final_templates = blueprint_objects(bp)
        components = []
        for component_name in ["Frame", "LanternBody_Paper", "Tassel_Paper", "SpotLight"]:
            components.append(component_report(final_templates[component_name], source_labels[component_name]))
        report["created"].append({
            "blueprint": destination,
            "source_shape": shape_label,
            "root_component": "Shape_A (sample structure; pivot at source Frame world transform)",
            "components": components,
            "light_properties_copied": copied_light,
            "skipped_properties": skipped_properties,
        })

    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("LANTERN_BLUEPRINT_CREATION_COMPLETE=" + str(REPORT))


main()
