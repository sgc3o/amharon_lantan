import unreal


unreal.EditorLoadingAndSavingUtils.load_map("/Game/Main")
actors = unreal.EditorLevelLibrary.get_all_level_actors()
controllers = [
    actor for actor in actors if actor.get_actor_label() == "ClonerBatchController"
]
if len(controllers) != 1:
    raise RuntimeError(f"Expected one ClonerBatchController, found {len(controllers)}")

controller = controllers[0]
front = controller.get_editor_property("front_line_cloners")
back = controller.get_editor_property("back_line_cloners")
global_count = controller.get_editor_property("global_clone_count")

if len(front) != 8 or len(back) != 12 or global_count != 10:
    raise RuntimeError(
        f"Unexpected controller state: front={len(front)}, back={len(back)}, count={global_count}"
    )

targets = list(front) + list(back)
target_paths = {actor.get_path_name() for actor in targets}
child_counts = {path: 0 for path in target_paths}
for actor in actors:
    try:
        parent = actor.get_attach_parent_actor()
    except Exception:
        parent = None
    if parent and parent.get_path_name() in child_counts:
        child_counts[parent.get_path_name()] += 1

if any(count != 10 for count in child_counts.values()):
    raise RuntimeError(f"Existing child assignments changed: {child_counts}")

controller.validate_target_counts()
validation_status = controller.get_editor_property("last_status")
unreal.log_warning(
    "CLONER_BATCH_VERIFY_OK|front=8|back=12|global_count=10|existing_children=10_each|status={}".format(
        validation_status
    )
)
