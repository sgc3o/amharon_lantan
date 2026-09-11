import json
from pathlib import Path
import unreal


LEVEL = "/Game/Main"
OUTPUT = Path(unreal.Paths.project_saved_dir()) / "PortraitCameraInspection.json"


def obj_path(value):
    return value.get_path_name() if value else None


def property_value(obj, name):
    try:
        value = obj.get_editor_property(name)
        if hasattr(value, "get_path_name"):
            return obj_path(value)
        if isinstance(value, (str, int, float, bool)) or value is None:
            return value
        if hasattr(value, "to_tuple"):
            return str(value.to_tuple())
        return str(value)
    except Exception as exc:
        return "<unavailable: %s>" % exc


def camera_info(actor):
    component = actor.get_cine_camera_component()
    if not component:
        components = actor.get_components_by_class(unreal.CineCameraComponent)
        component = components[0] if components else None
    if not component:
        return {
            "label": actor.get_actor_label() if hasattr(actor, "get_actor_label") else actor.get_name(),
            "error": "CineCameraComponent was not resolved from template",
            "all_components": [c.get_path_name() for c in actor.get_components_by_class(unreal.ActorComponent)],
        }
    filmback = component.get_editor_property("filmback")
    lens = component.get_editor_property("lens_settings")
    return {
        "label": actor.get_actor_label(),
        "actor_path": actor.get_path_name(),
        "component": component.get_path_name(),
        "filmback": {
            "sensor_width": filmback.sensor_width,
            "sensor_height": filmback.sensor_height,
            "sensor_aspect_ratio": filmback.sensor_aspect_ratio,
        },
        "lens": {
            "min_focal_length": lens.min_focal_length,
            "max_focal_length": lens.max_focal_length,
            "min_f_stop": lens.min_f_stop,
            "max_f_stop": lens.max_f_stop,
        },
        "current_focal_length": component.get_editor_property("current_focal_length"),
        "current_aperture": component.get_editor_property("current_aperture"),
        "aspect_ratio": property_value(component, "aspect_ratio"),
        "constrain_aspect_ratio": property_value(component, "constrain_aspect_ratio"),
        "override_aspect_ratio_axis_constraint": property_value(component, "override_aspect_ratio_axis_constraint"),
        "aspect_ratio_axis_constraint": property_value(component, "aspect_ratio_axis_constraint"),
    }


def sequence_info(sequence):
    bindings = []
    try:
        for binding in sequence.get_bindings():
            item = {
                "name": binding.get_name(),
                "display_name": str(binding.get_display_name()),
                "id": str(binding.get_id()),
            }
            try:
                template = binding.get_object_template()
                item["object_template"] = obj_path(template)
                item["object_class"] = template.get_class().get_path_name() if template else None
                if isinstance(template, unreal.CineCameraActor):
                    try:
                        item["nested_objects"] = [
                            {"path": o.get_path_name(), "class": o.get_class().get_path_name()}
                            for o in unreal.get_objects_with_outer(template, include_nested_objects=True)
                        ]
                    except Exception as nested_exc:
                        item["nested_objects_error"] = str(nested_exc)
                    item["camera"] = camera_info(template)
            except Exception as exc:
                item["object_template_error"] = str(exc)
            tracks = []
            for track in binding.get_tracks():
                track_item = {
                    "class": track.get_class().get_path_name(),
                    "display_name": str(track.get_display_name()),
                    "track_name": property_value(track, "track_name"),
                    "property_name": property_value(track, "property_name"),
                    "property_path": property_value(track, "property_path"),
                    "sections": [],
                }
                for section in track.get_sections():
                    section_item = {"class": section.get_class().get_path_name(), "channels": []}
                    try:
                        for channel in section.get_all_channels():
                            section_item["channels"].append({
                                "class": channel.get_class().get_path_name(),
                                "default": str(channel.get_default()),
                                "num_keys": channel.get_num_keys(),
                            })
                    except Exception as exc:
                        section_item["channel_error"] = str(exc)
                    track_item["sections"].append(section_item)
                tracks.append(track_item)
            item["tracks"] = tracks
            bindings.append(item)
    except Exception as exc:
        bindings.append({"error": str(exc)})
    return {"path": sequence.get_path_name(), "bindings": bindings}


def output_settings(config):
    setting = config.find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    resolution = setting.get_editor_property("output_resolution")
    return {
        "config": config.get_path_name(),
        "resolution": {"x": resolution.x, "y": resolution.y},
        "output_directory": str(setting.get_editor_property("output_directory")),
        "file_name_format": setting.get_editor_property("file_name_format"),
    }


def main():
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL):
        raise RuntimeError("Could not load Main")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    cameras = [camera_info(a) for a in actors if isinstance(a, unreal.CineCameraActor)]
    level_sequence_actors = []
    for actor in actors:
        if isinstance(actor, unreal.LevelSequenceActor):
            sequence = actor.get_editor_property("level_sequence_asset")
            level_sequence_actors.append({
                "actor": actor.get_actor_label(),
                "sequence": sequence_info(sequence) if sequence else None,
            })

    configs = []
    sequences = []
    for asset_path in unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.MoviePipelinePrimaryConfig):
            configs.append(output_settings(asset))
        elif isinstance(asset, unreal.LevelSequence):
            sequences.append(sequence_info(asset))

    queue_jobs = []
    try:
        queue = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem).get_queue()
        for job in queue.get_jobs():
            configuration = job.get_configuration()
            queue_jobs.append({
                "job_name": property_value(job, "job_name"),
                "sequence": obj_path(job.get_editor_property("sequence")),
                "map": str(job.get_editor_property("map")),
                "output": output_settings(configuration),
            })
    except Exception as exc:
        queue_jobs.append({"error": str(exc)})

    result = {
        "target_resolution": {"x": 1664, "y": 2994, "aspect": 1664.0 / 2994.0},
        "cameras": cameras,
        "level_sequence_actors": level_sequence_actors,
        "level_sequences": sequences,
        "mrq_config_assets": configs,
        "mrq_queue_jobs": queue_jobs,
    }
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("PORTRAIT_CAMERA_INSPECTION=" + str(OUTPUT))


main()
