import unreal


MAP_PATH = "/Game/Main"
ASSET_PATH = "/Game/LanternCloner/BP_ClonerBatchController"
INITIAL_COUNT = 10


def cloner_actors():
    return [
        actor
        for actor in unreal.EditorLevelLibrary.get_all_level_actors()
        if actor.get_class().get_name() == "CEClonerActor"
    ]


def direct_child_counts(cloners):
    counts = {cloner.get_path_name(): 0 for cloner in cloners}
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            parent = actor.get_attach_parent_actor()
        except Exception:
            parent = None
        if parent and parent.get_path_name() in counts:
            counts[parent.get_path_name()] += 1
    return counts


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
before_cloners = cloner_actors()
before_children = direct_child_counts(before_cloners)

asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if asset is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ClonerBatchController)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(
        "BP_ClonerBatchController",
        "/Game/LanternCloner",
        unreal.Blueprint,
        factory,
    )
    if asset is None:
        raise RuntimeError("Could not create BP_ClonerBatchController")

unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
controller = None
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if actor.get_actor_label() == "ClonerBatchController":
        controller = actor
        break

if controller is None:
    generated_class = asset.generated_class()
    controller = actor_subsystem.spawn_actor_from_class(
        generated_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if controller is None:
        raise RuntimeError("Could not place BP_ClonerBatchController in Main")

controller.set_actor_label("ClonerBatchController")
try:
    controller.set_folder_path("ClonerControl")
except Exception:
    pass
controller.set_editor_property("global_clone_count", INITIAL_COUNT)
controller.discover_cloners()

front = controller.get_editor_property("front_line_cloners")
back = controller.get_editor_property("back_line_cloners")
if len(front) != 8 or len(back) != 12:
    raise RuntimeError(f"Unexpected discovery result: FrontLine={len(front)}, BackLine={len(back)}")

after_cloners = cloner_actors()
after_children = direct_child_counts(after_cloners)
if before_children != after_children:
    raise RuntimeError("Cloner child assignments changed during controller setup")

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Could not save Main level")
unreal.log_warning(
    "CLONER_BATCH_CREATED|asset={}|front={}|back={}|global_count={}|counts_unchanged=true".format(
        ASSET_PATH, len(front), len(back), INITIAL_COUNT
    )
)
