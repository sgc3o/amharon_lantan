import unreal


unreal.EditorLoadingAndSavingUtils.load_map("/Game/Main")
subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
controller_class = getattr(unreal, "ClonerBatchController", None)
if controller_class is None:
    raise RuntimeError("ClonerBatchController class is not exposed to Unreal Python")

controller = subsystem.spawn_actor_from_class(
    controller_class,
    unreal.Vector(0.0, 0.0, 0.0),
    unreal.Rotator(0.0, 0.0, 0.0),
    transient=True,
)
if controller is None:
    raise RuntimeError("Could not spawn ClonerBatchController")

controller.validate_target_counts()
status = controller.get_editor_property("last_status")
if "Validation" not in status:
    raise RuntimeError(f"Unexpected validation status: {status}")

unreal.log_warning(f"CLONER_BATCH_SMOKE_OK|{status}")
subsystem.destroy_actor(controller)
