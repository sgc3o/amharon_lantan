import json
from pathlib import Path
import unreal


ASSETS = [
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
REPORT = Path(unreal.Paths.project_saved_dir()) / "LanternMobilityFixReport.json"


def main():
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    report = []
    for asset_path in ASSETS:
        bp = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not bp:
            raise RuntimeError("Missing Blueprint: " + asset_path)
        changed = []
        seen = set()
        for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
            data = library.get_data(handle)
            obj = library.get_object_for_blueprint(data, bp)
            if not obj or obj.get_path_name() in seen:
                continue
            seen.add(obj.get_path_name())
            if isinstance(obj, unreal.SceneComponent):
                obj.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                changed.append(str(library.get_variable_name(data)))
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        if not unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False):
            raise RuntimeError("Failed to save Blueprint: " + asset_path)
        report.append({"blueprint": asset_path, "movable_components": changed})
    REPORT.write_text(json.dumps({"fixed": report}, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("LANTERN_MOBILITY_FIX_COMPLETE=" + str(REPORT))


main()
