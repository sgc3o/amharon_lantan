import unreal


def prop(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def describe_output(owner, config):
    output = config.find_setting_by_class(unreal.MoviePipelineOutputSetting)
    if output is None:
        unreal.log_warning(f"MRQ_RANGE|owner={owner}|output_setting=None")
        return
    resolution = prop(output, "output_resolution")
    unreal.log_warning(
        "MRQ_RANGE|owner={}|custom={}|start={}|end={}|resolution={}x{}|handle_frames={}".format(
            owner,
            prop(output, "use_custom_playback_range"),
            prop(output, "custom_start_frame"),
            prop(output, "custom_end_frame"),
            resolution.x if resolution else None,
            resolution.y if resolution else None,
            prop(output, "handle_frame_count"),
        )
    )


def section_range(section):
    try:
        start = section.get_start_frame()
    except Exception:
        start = None
    try:
        end = section.get_end_frame()
    except Exception:
        end = None
    return start, end


def describe_tracks(asset_path, asset):
    try:
        master_tracks = asset.get_master_tracks()
    except Exception:
        master_tracks = []
    for track in master_tracks:
        for section in track.get_sections():
            start, end = section_range(section)
            unreal.log_warning(
                "SEQ_SECTION|sequence={}|scope=master|track={}|section={}|start={}|end={}".format(
                    asset_path,
                    track.get_class().get_name(),
                    section.get_class().get_name(),
                    start,
                    end,
                )
            )
    for binding in asset.get_bindings():
        for track in binding.get_tracks():
            for section in track.get_sections():
                start, end = section_range(section)
                unreal.log_warning(
                    "SEQ_SECTION|sequence={}|scope={}|track={}|section={}|start={}|end={}".format(
                        asset_path,
                        binding.get_name(),
                        track.get_class().get_name(),
                        section.get_class().get_name(),
                        start,
                        end,
                    )
                )


for asset_path in unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if isinstance(asset, unreal.LevelSequence):
        display_rate = asset.get_display_rate()
        tick_resolution = asset.get_tick_resolution()
        unreal.log_warning(
            "SEQ_RANGE|path={}|start={}|end={}|duration={}|display_rate={}/{}|tick_resolution={}/{}".format(
                asset_path,
                asset.get_playback_start(),
                asset.get_playback_end(),
                asset.get_playback_end() - asset.get_playback_start(),
                display_rate.numerator,
                display_rate.denominator,
                tick_resolution.numerator,
                tick_resolution.denominator,
            )
        )
        describe_tracks(asset_path, asset)
    elif isinstance(asset, unreal.MoviePipelinePrimaryConfig):
        describe_output(asset_path, asset)

try:
    queue = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem).get_queue()
    jobs = queue.get_jobs()
    unreal.log_warning(f"MRQ_QUEUE|jobs={len(jobs)}")
    for index, job in enumerate(jobs):
        describe_output(f"queue_job_{index}:{prop(job, 'job_name')}", job.get_configuration())
        unreal.log_warning(
            "MRQ_JOB|index={}|sequence={}|map={}".format(
                index, prop(job, "sequence"), prop(job, "map")
            )
        )
except Exception as exc:
    unreal.log_warning(f"MRQ_QUEUE_ERROR|{exc}")
