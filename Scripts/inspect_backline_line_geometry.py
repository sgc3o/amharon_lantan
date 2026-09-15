import unreal
from pathlib import Path

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Main")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
cloner = next(a for a in actors if a.get_actor_label() == "BackLine01")
layout = cloner.get_active_layout()
origin, extent = cloner.get_actor_bounds(False)
message = (
    "BACKLINE_GEOMETRY "
    f"actor_location={cloner.get_actor_location()} actor_rotation={cloner.get_actor_rotation()} "
    f"actor_scale={cloner.get_actor_scale3d()} bounds_origin={origin} bounds_extent={extent} "
    f"layout={layout.get_class().get_name()} count={layout.get_count()} "
    f"spacing={layout.get_spacing()} axis={layout.get_axis()} "
    f"direction={layout.get_direction()} rotation={layout.get_rotation()}"
)
Path(unreal.Paths.project_saved_dir(), "backline_geometry.txt").write_text(message, encoding="utf-8")
unreal.log_warning(message)
