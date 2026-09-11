import json
from pathlib import Path
import unreal


SEQUENCE_PATH = "/Game/Lantan"
MRQ_CONFIG_PATH = "/Game/centercoat"
OUTPUT_X = 1664
OUTPUT_Y = 2994
ASPECT = OUTPUT_X / OUTPUT_Y
SENSOR_WIDTH = 16.64
SENSOR_HEIGHT = 29.94
REPORT = Path(unreal.Paths.project_saved_dir()) / "PortraitCameraConfigurationReport.json"


def remove_existing_property_tracks(binding, property_paths):
    for track in list(binding.get_tracks()):
        try:
            path = str(track.get_property_path())
        except Exception:
            continue
        if path in property_paths:
            binding.remove_track(track)


def add_float_track(binding, property_name, property_path, value, start_frame, end_frame):
    track = binding.add_track(unreal.MovieSceneFloatTrack)
    track.set_property_name_and_path(property_name, property_path)
    section = track.add_section()
    section.set_range(start_frame, end_frame)
    channels = section.get_all_channels()
    if len(channels) != 1:
        raise RuntimeError("Expected one float channel for " + property_path)
    channels[0].set_default(float(value))
    return track


def add_bool_track(binding, property_name, property_path, value, start_frame, end_frame):
    track = binding.add_track(unreal.MovieSceneBoolTrack)
    track.set_property_name_and_path(property_name, property_path)
    section = track.add_section()
    section.set_range(start_frame, end_frame)
    channels = section.get_all_channels()
    if len(channels) != 1:
        raise RuntimeError("Expected one bool channel for " + property_path)
    channels[0].set_default(bool(value))
    return track


def main():
    sequence = unreal.EditorAssetLibrary.load_asset(SEQUENCE_PATH)
    if not isinstance(sequence, unreal.LevelSequence):
        raise RuntimeError("Level Sequence not found: " + SEQUENCE_PATH)
    camera_binding = next((binding for binding in sequence.get_bindings()
                           if binding.get_name() == "CameraComponent"), None)
    if not camera_binding:
        raise RuntimeError("CameraComponent binding not found in " + SEQUENCE_PATH)

    start_frame = sequence.get_playback_start()
    end_frame = sequence.get_playback_end()
    property_paths = {
        "Filmback.SensorWidth", "Filmback.SensorHeight", "AspectRatio",
        "bConstrainAspectRatio", "bOverrideAspectRatioAxisConstraint",
    }
    remove_existing_property_tracks(camera_binding, property_paths)
    add_float_track(camera_binding, "SensorWidth", "Filmback.SensorWidth",
                    SENSOR_WIDTH, start_frame, end_frame)
    add_float_track(camera_binding, "SensorHeight", "Filmback.SensorHeight",
                    SENSOR_HEIGHT, start_frame, end_frame)
    add_float_track(camera_binding, "AspectRatio", "AspectRatio",
                    ASPECT, start_frame, end_frame)
    add_bool_track(camera_binding, "bConstrainAspectRatio", "bConstrainAspectRatio",
                   True, start_frame, end_frame)
    add_bool_track(camera_binding, "bOverrideAspectRatioAxisConstraint",
                   "bOverrideAspectRatioAxisConstraint", True, start_frame, end_frame)
    if not unreal.EditorAssetLibrary.save_asset(SEQUENCE_PATH, only_if_is_dirty=False):
        raise RuntimeError("Failed to save Level Sequence: " + SEQUENCE_PATH)

    config = unreal.EditorAssetLibrary.load_asset(MRQ_CONFIG_PATH)
    if not isinstance(config, unreal.MoviePipelinePrimaryConfig):
        raise RuntimeError("MRQ config not found: " + MRQ_CONFIG_PATH)
    output = config.find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    output.set_editor_property("output_resolution", unreal.IntPoint(OUTPUT_X, OUTPUT_Y))
    if not unreal.EditorAssetLibrary.save_asset(MRQ_CONFIG_PATH, only_if_is_dirty=False):
        raise RuntimeError("Failed to save MRQ config: " + MRQ_CONFIG_PATH)

    report = {
        "sequence": SEQUENCE_PATH,
        "camera_binding": "CameraComponent",
        "filmback": {"sensor_width_mm": SENSOR_WIDTH, "sensor_height_mm": SENSOR_HEIGHT},
        "aspect_ratio": ASPECT,
        "constrain_aspect_ratio": True,
        "override_aspect_ratio_axis_constraint": True,
        "mrq_config": MRQ_CONFIG_PATH,
        "output_resolution": {"x": OUTPUT_X, "y": OUTPUT_Y},
        "main_level_saved": False,
    }
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("PORTRAIT_CAMERA_CONFIGURATION_COMPLETE=" + str(REPORT))


main()
