import json
from pathlib import Path
import unreal


LEVEL = "/Game/Main"
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
OUTPUT = Path(unreal.Paths.project_saved_dir()) / "LanternClonerAttachmentVerification.json"


def main():
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL):
        raise RuntimeError("Could not load Main")
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cloner = next((a for a in subsystem.get_all_level_actors() if a.get_actor_label() == "Cloner2"), None)
    if not cloner:
        raise RuntimeError("Cloner2 was not found")
    results = []
    spawned = []
    try:
        for asset_path in ASSETS:
            bp = unreal.EditorAssetLibrary.load_asset(asset_path)
            actor = subsystem.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), transient=True)
            if not actor:
                raise RuntimeError("Could not spawn " + asset_path)
            spawned.append(actor)
            attached = actor.attach_to_actor(
                cloner,
                socket_name=unreal.Name("None"),
                location_rule=unreal.AttachmentRule.KEEP_WORLD,
                rotation_rule=unreal.AttachmentRule.KEEP_WORLD,
                scale_rule=unreal.AttachmentRule.KEEP_WORLD,
                weld_simulated_bodies=False,
            )
            parent_matches = actor.get_attach_parent_actor() == cloner
            results.append({"blueprint": asset_path, "attach_return": attached, "parent_matches": parent_matches})
            if not attached or not parent_matches:
                raise RuntimeError("Cloner2 attachment failed for " + asset_path)
    finally:
        for actor in spawned:
            if actor:
                subsystem.destroy_actor(actor)
    OUTPUT.write_text(json.dumps({"pass": True, "results": results, "main_saved": False}, indent=2), encoding="utf-8")
    unreal.log("LANTERN_CLONER_ATTACHMENT_PASS=" + str(OUTPUT))


main()
