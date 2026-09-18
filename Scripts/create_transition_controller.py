import unreal
import re


ASSET_PATH = "/Game/BP_TransitionController"
MAP_PATH = "/Game/Main"
SEQUENCE_PATH = "/Game/LS_Main_Trs"

asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if asset is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.TransitionController)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_TransitionController", "/Game", unreal.Blueprint, factory
    )
    if asset is None:
        raise RuntimeError("Could not create BP_TransitionController")
else:
    unreal.BlueprintEditorLibrary.reparent_blueprint(
        asset, unreal.TransitionController
    )

unreal.BlueprintEditorLibrary.compile_blueprint(asset)
if not unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False):
    raise RuntimeError("Could not save BP_TransitionController")

default_object = unreal.get_default_object(asset.generated_class())
default_object.get_editor_property("transition_actors")

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_actors = actor_subsystem.get_all_level_actors()
controller = next(
    (
        actor
        for actor in level_actors
        if actor.get_class() == asset.generated_class()
    ),
    None,
)
if controller is None:
    controller = actor_subsystem.spawn_actor_from_class(
        asset.generated_class(),
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if controller is None:
        raise RuntimeError("Could not place BP_TransitionController in Main")
controller.set_actor_label("BP_TransitionController")

def natural_key(actor):
    return [int(part) if part.isdigit() else part.lower()
            for part in re.split(r"(\d+)", actor.get_actor_label())]

lanterns = sorted(
    [actor for actor in level_actors
     if actor.get_actor_label().startswith("BP_Lantern_")],
    key=natural_key,
)
registered_actors = list(controller.get_editor_property("transition_actors"))
if len(registered_actors) == 0:
    registered_actors = lanterns
    controller.set_editor_property("transition_actors", registered_actors)
unreal.log_warning("TRANSITION_DISCOVERY|registered={}|discovered={}".format(
    len(registered_actors), len(lanterns)
))

cameras = [actor for actor in level_actors if isinstance(actor, unreal.CameraActor)]
if len(cameras) == 1:
    controller.set_editor_property("transition_camera", cameras[0])

controller.capture_final_transforms()
controller.set_progress(1.0)

sequence = unreal.EditorAssetLibrary.load_asset(SEQUENCE_PATH)
if sequence is None:
    sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "LS_Main_Trs", "/Game", unreal.LevelSequence, unreal.LevelSequenceFactoryNew()
    )
    sequence.set_display_rate(unreal.FrameRate(30, 1))
    sequence.set_playback_start(0)
    sequence.set_playback_end(90)
    binding = sequence.add_possessable(controller)
    track = binding.add_track(unreal.MovieSceneFloatTrack)
    track.set_property_name_and_path("Progress", "Progress")
    section = track.add_section()
    section.set_range(0, 90)
    channel = section.get_all_channels()[0]
    channel.add_key(unreal.FrameNumber(0), 0.0)
    channel.add_key(unreal.FrameNumber(90), 1.0)
    unreal.EditorAssetLibrary.save_asset(SEQUENCE_PATH, only_if_is_dirty=False)

sequence_actor = next(
    (actor for actor in level_actors
     if isinstance(actor, unreal.LevelSequenceActor)
     and actor.get_actor_label() == "LS_Main_Trs"),
    None,
)
if sequence_actor is None:
    sequence_actor = actor_subsystem.spawn_actor_from_class(
        unreal.LevelSequenceActor,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
sequence_actor.set_actor_label("LS_Main_Trs")
sequence_actor.set_sequence(sequence)
if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError("Could not save Main")

unreal.log_warning(
    "TRANSITION_CONTROLLER_CREATED|asset={}|map={}|sequence={}|parent=TransitionController|property=TransitionActors|placed=true|count={}".format(
        ASSET_PATH,
        MAP_PATH,
        SEQUENCE_PATH,
        len(controller.get_editor_property("transition_actors")),
    )
)
