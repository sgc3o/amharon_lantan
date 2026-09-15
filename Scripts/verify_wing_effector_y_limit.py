import unreal
import math


unreal.EditorLoadingAndSavingUtils.load_map("/Game/Main")
actors = [
    actor
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    if actor.get_actor_label() == "WingEffector"
]
if len(actors) != 1:
    raise RuntimeError(f"Expected one WingEffector, found {len(actors)}")

components = actors[0].get_components_by_class(unreal.WingEffectorYLimitComponent)
if len(components) != 1:
    raise RuntimeError(f"Expected one Y limiter, found {len(components)}")

limiter = components[0]
values = (
    limiter.get_editor_property("limit_y_rotation"),
    limiter.get_editor_property("effector_mode"),
    limiter.get_editor_property("min_y_rotation"),
    limiter.get_editor_property("max_y_rotation"),
    limiter.get_editor_property("sway_frequency"),
    limiter.get_editor_property("enable_z_sway"),
    limiter.get_editor_property("min_z_rotation"),
    limiter.get_editor_property("max_z_rotation"),
    limiter.get_editor_property("z_sway_frequency"),
    limiter.get_editor_property("z_randomness"),
)
if not (
    values[0]
    and values[1] == unreal.WingEffectorMotionMode.DEFAULT
    and math.isclose(values[2], -5.0, abs_tol=1e-5)
    and math.isclose(values[3], 5.0, abs_tol=1e-5)
    and math.isclose(values[4], 0.2, abs_tol=1e-5)
):
    raise RuntimeError(f"Unexpected limiter settings: {values}")

unreal.log_warning(
    f"WING_ROTATION_LIMIT_VERIFIED|actor=WingEffector|effector_mode=default|mode=bounded_orientation_force|y={values[2]}..{values[3]}|z_enabled={values[5]}|z={values[6]}..{values[7]}|z_frequency={values[8]}|randomness={values[9]}"
)
