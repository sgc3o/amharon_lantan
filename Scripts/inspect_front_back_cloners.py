import unreal


MAP_PATH = "/Game/Main"


def safe_call(obj, name, default=None):
    try:
        return getattr(obj, name)()
    except Exception:
        return default


def safe_property(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return None


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
actors = unreal.EditorLevelLibrary.get_all_level_actors()

unreal.log_warning(f"CLONER_INSPECT_BEGIN|actor_count={len(actors)}")
class_counts = {}
for actor in actors:
    class_name = actor.get_class().get_name()
    class_counts[class_name] = class_counts.get(class_name, 0) + 1
unreal.log_warning("ACTOR_CLASSES|" + ",".join(f"{name}={count}" for name, count in sorted(class_counts.items())))
for actor in actors:
    class_name = actor.get_class().get_name()
    label = actor.get_actor_label()
    folder = str(safe_call(actor, "get_folder_path", "") or safe_property(actor, "folder_path") or "")
    if not any(token in (class_name + " " + label + " " + folder).lower() for token in ("cloner", "frontline", "backline")):
        continue
    parent = safe_call(actor, "get_attach_parent_actor")
    parent_chain = []
    while parent:
        parent_chain.append(parent.get_actor_label())
        parent = safe_call(parent, "get_attach_parent_actor")

    layout = safe_call(actor, "get_active_layout")
    layout_name = safe_call(actor, "get_layout_name", "None")
    layout_class = layout.get_class().get_name() if layout else "None"
    values = []
    if layout:
        for property_name in (
            "count",
            "count_x",
            "count_y",
            "count_z",
            "base_count",
            "height_count",
            "width_count",
        ):
            value = safe_property(layout, property_name)
            if value is not None:
                values.append(f"{property_name}={value}")

    tags = [str(tag) for tag in safe_property(actor, "tags") or []]
    unreal.log_warning(
        "CLONER|label={}|class={}|folder={}|parents={}|tags={}|layout_name={}|layout={}|{}".format(
            label,
            class_name,
            folder,
            ">".join(parent_chain),
            ",".join(tags),
            layout_name,
            layout_class,
            ",".join(values),
        )
    )
unreal.log_warning("CLONER_INSPECT_END")
