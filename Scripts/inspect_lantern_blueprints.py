import json
from pathlib import Path

import unreal


PROJECT_LEVEL = "/Game/Main"
SAMPLE_BP = "/Game/c4d/rantan/models/BP_Lantern_A_01"
OUTPUT = Path(unreal.Paths.project_saved_dir()) / "LanternBlueprintInspection.json"


def obj_path(value):
    if not value:
        return None
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def vector(value):
    return {"x": value.x, "y": value.y, "z": value.z}


def rotator(value):
    return {"pitch": value.pitch, "yaw": value.yaw, "roll": value.roll}


def transform(value):
    return {
        "location": vector(value.translation),
        "rotation": rotator(value.rotation.rotator()),
        "scale": vector(value.scale3d),
    }


def safe_property(obj, name):
    try:
        value = obj.get_editor_property(name)
        if hasattr(value, "get_path_name"):
            return obj_path(value)
        if isinstance(value, (str, int, float, bool)) or value is None:
            return value
        return str(value)
    except Exception:
        return None


def component_info(component):
    data = {
        "name": component.get_name(),
        "class": component.get_class().get_name(),
        "path": component.get_path_name(),
    }
    if isinstance(component, unreal.SceneComponent):
        data["relative_transform"] = transform(component.get_relative_transform())
        data["world_transform"] = transform(component.get_world_transform())
        parent = component.get_attach_parent()
        data["attach_parent"] = parent.get_name() if parent else None
    if isinstance(component, unreal.StaticMeshComponent):
        data["static_mesh"] = obj_path(component.static_mesh)
        data["materials"] = [obj_path(component.get_material(i)) for i in range(component.get_num_materials())]
    if isinstance(component, unreal.LightComponentBase):
        for prop in [
            "intensity", "light_color", "temperature", "use_temperature",
            "cast_shadows", "cast_volumetric_shadow", "indirect_lighting_intensity",
            "volumetric_scattering_intensity", "mobility", "attenuation_radius",
            "inner_cone_angle", "outer_cone_angle", "source_radius", "source_length",
            "soft_source_radius", "inverse_exposure_blend",
        ]:
            value = safe_property(component, prop)
            if value is not None:
                data[prop] = value
    return data


def actor_info(actor):
    root = safe_property(actor, "root_component")
    attached = []
    try:
        attached = [child.get_actor_label() for child in actor.get_attached_actors()]
    except Exception:
        pass
    parent = None
    try:
        p = actor.get_attach_parent_actor()
        parent = p.get_actor_label() if p else None
    except Exception:
        pass
    return {
        "label": actor.get_actor_label(),
        "name": actor.get_name(),
        "class": actor.get_class().get_path_name(),
        "folder": str(actor.get_folder_path()),
        "parent": parent,
        "children": attached,
        "actor_transform": transform(actor.get_actor_transform()),
        "root_component": root.rsplit(".", 1)[-1] if isinstance(root, str) and root else None,
        "components": [component_info(c) for c in actor.get_components_by_class(unreal.ActorComponent)],
    }


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(PROJECT_LEVEL):
        raise RuntimeError("Could not load Main level")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    relevant = []
    for actor in actors:
        label = actor.get_actor_label()
        component_names = [c.get_name() for c in actor.get_components_by_class(unreal.ActorComponent)]
        joined = " ".join([label] + component_names).lower()
        if any(token in joined for token in ["lantern", "shape_", "frame", "tassel", "spotlight"]):
            relevant.append(actor_info(actor))

    sample = unreal.EditorAssetLibrary.load_asset(SAMPLE_BP)
    if not sample:
        raise RuntimeError("Could not load sample Blueprint")
    generated_class = sample.generated_class()
    cdo = unreal.get_default_object(generated_class)
    sample_data = {
        "asset": sample.get_path_name(),
        "generated_class": generated_class.get_path_name(),
        "cdo_components": [component_info(c) for c in cdo.get_components_by_class(unreal.ActorComponent)],
    }

    result = {
        "level": PROJECT_LEVEL,
        "actor_count": len(actors),
        "relevant_actors": relevant,
        "sample_blueprint": sample_data,
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("WROTE_LANTERN_INSPECTION=" + str(OUTPUT))


main()
