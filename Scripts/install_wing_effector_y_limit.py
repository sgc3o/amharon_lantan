import unreal


MAP_PATH = "/Game/Main"
ACTOR_LABEL = "WingEffector"


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
matches = [
    actor
    for actor in unreal.EditorLevelLibrary.get_all_level_actors()
    if actor.get_actor_label() == ACTOR_LABEL
]
if len(matches) != 1:
    raise RuntimeError(
        f"Expected exactly one actor labelled {ACTOR_LABEL!r}, found {len(matches)}"
    )

wing_effector = matches[0]
limiter = unreal.WingEffectorYLimitComponent.add_to_actor(wing_effector)
if limiter is None:
    raise RuntimeError("Could not add WingEffectorYLimitComponent")

limiter.set_editor_property("limit_y_rotation", True)
limiter.set_editor_property("min_y_rotation", -5.0)
limiter.set_editor_property("max_y_rotation", 5.0)
limiter.set_editor_property("sway_frequency", 0.2)
limiter.set_editor_property("enable_z_sway", True)
limiter.set_editor_property("min_z_rotation", -1.5)
limiter.set_editor_property("max_z_rotation", 1.5)
limiter.set_editor_property("z_sway_frequency", 0.1)
limiter.set_editor_property("z_randomness", 0.7)
limiter.set_editor_property("z_smoothing", 0.8)
limiter.restart_sway_from_center()

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Could not save Main after adding the Y limiter")

unreal.log_warning(
    "WING_Y_ROTATION_LIMIT_INSTALLED|actor={}|class={}|min={}|max={}|current={}".format(
        wing_effector.get_actor_label(),
        wing_effector.get_class().get_name(),
        limiter.get_editor_property("min_y_rotation"),
        limiter.get_editor_property("max_y_rotation"),
        limiter.get_editor_property("current_y_rotation"),
    )
)
