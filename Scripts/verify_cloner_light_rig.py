from pathlib import Path

import unreal


ASSET_PATH = "/Game/c4d/rantan/models/BP_ClonerLightRig"
EXPECTED_COUNT = 20
EXPECTED_SPACING = 75.0
EXPECTED_MINIMUM_INTENSITY = 0.0
EXPECTED_MAXIMUM_INTENSITY = 5.0
EXPECTED_MINIMUM_DIFFERENCE = 2.0
EXPECTED_TRANSITION_DURATION = 1.0
GENERATED_TAG = "GeneratedByLightCloner"

blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Could not load {ASSET_PATH}")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = actor_subsystem.spawn_actor_from_class(
    blueprint.generated_class(),
    unreal.Vector(0.0, 0.0, 0.0),
    unreal.Rotator(0.0, 0.0, 0.0),
)
if actor is None:
    raise RuntimeError("Could not spawn BP_ClonerLightRig")

try:
    components = list(actor.get_components_by_class(unreal.PointLightComponent))
    visible = [
        component
        for component in components
        if component.get_editor_property("visible")
        and not component.get_editor_property("hidden_in_game")
    ]
    generated = [
        component
        for component in components
        if GENERATED_TAG in [str(tag) for tag in component.get_editor_property("component_tags")]
    ]

    if len(visible) != EXPECTED_COUNT:
        raise RuntimeError(
            f"Expected {EXPECTED_COUNT} visible point lights, found {len(visible)}"
        )
    if len(generated) != EXPECTED_COUNT - 1:
        raise RuntimeError(
            f"Expected {EXPECTED_COUNT - 1} generated point lights, found {len(generated)}"
        )

    if not actor.get_editor_property("enable_light_flicker"):
        raise RuntimeError("Light flicker is not enabled")
    if abs(actor.get_editor_property("flicker_minimum_intensity") - EXPECTED_MINIMUM_INTENSITY) > 0.01:
        raise RuntimeError("Unexpected flicker minimum intensity")
    if abs(actor.get_editor_property("flicker_maximum_intensity") - EXPECTED_MAXIMUM_INTENSITY) > 0.01:
        raise RuntimeError("Unexpected flicker maximum intensity")
    if abs(actor.get_editor_property("flicker_minimum_difference") - EXPECTED_MINIMUM_DIFFERENCE) > 0.01:
        raise RuntimeError("Unexpected flicker minimum difference")
    if abs(actor.get_editor_property("flicker_transition_duration") - EXPECTED_TRANSITION_DURATION) > 0.01:
        raise RuntimeError("Unexpected flicker transition duration")

    intensities = [component.get_editor_property("intensity") for component in visible]
    if any(
        intensity < EXPECTED_MINIMUM_INTENSITY - 0.01
        or intensity > EXPECTED_MAXIMUM_INTENSITY + 0.01
        for intensity in intensities
    ):
        raise RuntimeError(f"Flicker intensity outside configured range: {intensities}")
    if len({round(intensity, 3) for intensity in intensities}) < 2:
        raise RuntimeError("Independent light phases did not produce varied intensities")

    generated.sort(key=lambda component: component.get_name())
    template = next(component for component in visible if component not in generated)
    template_location = template.get_editor_property("relative_location")
    for index, component in enumerate(generated, start=1):
        location = component.get_editor_property("relative_location")
        expected_z = template_location.z + EXPECTED_SPACING * index
        if (
            abs(location.x - template_location.x) > 0.01
            or abs(location.y - template_location.y) > 0.01
            or abs(location.z - expected_z) > 0.01
        ):
            raise RuntimeError(
                f"Unexpected clone position for {component.get_name()}: {location}; "
                f"expected Z={expected_z}"
            )

    report = (
        f"PASS|asset={ASSET_PATH}|visible={len(visible)}|generated={len(generated)}|"
        f"axis=Z|spacing_cm={EXPECTED_SPACING}|flicker_min={EXPECTED_MINIMUM_INTENSITY}|"
        f"flicker_max={EXPECTED_MAXIMUM_INTENSITY}|minimum_difference={EXPECTED_MINIMUM_DIFFERENCE}|"
        f"transition_seconds={EXPECTED_TRANSITION_DURATION}|distinct_initial_values="
        f"{len({round(intensity, 3) for intensity in intensities})}"
    )
    Path(unreal.Paths.project_saved_dir(), "ClonerLightRigVerification.txt").write_text(
        report, encoding="utf-8"
    )
    unreal.log_warning(f"CLONER_LIGHT_RIG_VERIFY|{report}")
finally:
    actor_subsystem.destroy_actor(actor)
