import unreal


def prop(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


queue = unreal.get_editor_subsystem(unreal.MoviePipelineQueueSubsystem).get_queue()
jobs = queue.get_jobs()
unreal.log_warning(f"LIVE_MRQ_QUEUE|jobs={len(jobs)}")
for index, job in enumerate(jobs):
    config = job.get_configuration()
    output = config.find_setting_by_class(unreal.MoviePipelineOutputSetting)
    sequence_ref = prop(job, "sequence")
    sequence = sequence_ref.try_load() if sequence_ref else None
    resolution = prop(output, "output_resolution") if output else None
    unreal.log_warning(
        "LIVE_MRQ_JOB|index={}|name={}|sequence={}|seq_start={}|seq_end={}|custom={}|custom_start={}|custom_end={}|resolution={}x{}".format(
            index,
            prop(job, "job_name"),
            sequence.get_path_name() if sequence else sequence_ref,
            sequence.get_playback_start() if sequence else None,
            sequence.get_playback_end() if sequence else None,
            prop(output, "use_custom_playback_range") if output else None,
            prop(output, "custom_start_frame") if output else None,
            prop(output, "custom_end_frame") if output else None,
            resolution.x if resolution else None,
            resolution.y if resolution else None,
        )
    )
