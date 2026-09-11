from pathlib import Path
import unreal

bp = unreal.EditorAssetLibrary.load_asset("/Game/c4d/rantan/models/BP_Lantern_A_01")
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
library = unreal.SubobjectDataBlueprintFunctionLibrary
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
lines = ["count=" + str(len(handles))]
for index, handle in enumerate(handles):
    data = library.get_data(handle)
    obj = library.get_object_for_blueprint(data, bp)
    lines.append("%d handle_valid=%s object=%s class=%s root_actor=%s root_component=%s default_root=%s" % (
        index,
        library.is_handle_valid(handle),
        obj.get_path_name() if obj else None,
        obj.get_class().get_path_name() if obj else None,
        library.is_root_actor(data),
        library.is_root_component(data),
        library.is_default_scene_root(data),
    ))
Path(unreal.Paths.project_saved_dir(), "BlueprintSubobjects.txt").write_text("\n".join(lines), encoding="utf-8")
