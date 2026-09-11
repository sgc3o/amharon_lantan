import json
from pathlib import Path
import unreal

LEVEL = "/Game/Main"
ASSETS = [
    "/Game/c4d/rantan/models/BP_Lantern_A_01",
    "/Game/c4d/rantan/models/BP_Lantern_A_02",
    "/Game/c4d/rantan/models/BP_Lantern_A_03",
    "/Game/c4d/rantan/models/BP_Lantern_B_01",
    "/Game/c4d/rantan/models/BP_Lantern_B_02",
    "/Game/c4d/rantan/models/BP_Lantern_C_01",
    "/Game/c4d/rantan/models/BP_Lantern_C_02",
    "/Game/c4d/rantan/models/BP_Lantern_C_03",
    "/Game/c4d/rantan/models/BP_Lantern_D_01",
    "/Game/c4d/rantan/models/BP_Lantern_D_02",
]


def component_mobility(component):
    try:
        return str(component.get_editor_property("mobility"))
    except Exception:
        return None


def bp_info(path):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    result = []
    seen = set()
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = library.get_data(handle)
        obj = library.get_object_for_blueprint(data, bp)
        if not obj or obj.get_path_name() in seen:
            continue
        seen.add(obj.get_path_name())
        if isinstance(obj, unreal.SceneComponent):
            result.append({
                "name": str(library.get_variable_name(data)),
                "class": obj.get_class().get_name(),
                "mobility": component_mobility(obj),
                "is_root": library.is_root_component(data),
            })
    return result


unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
cloner = next((a for a in actors if a.get_actor_label() == "Cloner2"), None)
cloner_components = []
if cloner:
    for component in cloner.get_components_by_class(unreal.SceneComponent):
        cloner_components.append({
            "name": component.get_name(),
            "class": component.get_class().get_name(),
            "mobility": component_mobility(component),
        })
result = {"blueprints": {p: bp_info(p) for p in ASSETS}, "cloner2": cloner_components}
Path(unreal.Paths.project_saved_dir(), "LanternMobilityInspection.json").write_text(
    json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
