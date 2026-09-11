import json
import os
import time
import traceback
from pathlib import Path
import unreal

MAP = '/Game/LanternCloner/Maps/L_LanternCloner_Test'
RESULT = Path(unreal.Paths.project_saved_dir()) / 'LanternClonerSafety' / 'linear_cloner_result.json'
steps = []
index = 0
next_time = time.monotonic() + 6.0
busy = False

def record(step, **data):
    payload = dict(step=step, pid=os.getpid(), **data)
    RESULT.write_text(json.dumps(payload, indent=2), encoding='utf-8')
    unreal.log('LANTERN_LINEAR|' + json.dumps(payload))

def actor():
    found = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
             if isinstance(a, unreal.LanternCloner)]
    assert len(found) == 1
    return found[0]

def load():
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)

def configure():
    a = actor()
    a.set_editor_property('clone_mode', unreal.LanternCloneMode.SHUFFLE)
    a.set_editor_property('random_seed', 1337)
    a.set_editor_property('vertical_count', 13)
    a.set_editor_property('vertical_spacing', 110.0)
    a.set_editor_property('scale_random', 0.05)
    a.set_editor_property('random_position_offset', unreal.Vector(0.0, 0.0, 0.0))
    a.generate()

def verify_and_move():
    a = actor()
    comps = a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    assert a.get_editor_property('generated_lantern_count') == 13
    assert sum(c.get_instance_count() for c in comps) == 39
    assert a.is_generated_data_consistent()
    original = a.get_actor_location()
    before = comps[0].get_world_location()
    delta = unreal.Vector(137.0, -83.0, 41.0)
    assert a.set_actor_location(original + delta, False, False)
    after = comps[0].get_world_location()
    moved = after - before
    assert abs(moved.x - delta.x) < 0.1 and abs(moved.y - delta.y) < 0.1 and abs(moved.z - delta.z) < 0.1
    assert a.set_actor_location(original, False, False)

def save():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().startswith(MAP + '.')
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    record('PASS', clone_count=13, mesh_instances=39, spacing=110.0,
           seed=1337, clone_mode='Shuffle', scale_random=0.05,
           position_jitter=[0.0, 0.0, 0.0], actor_move_follow=True,
           hism_consistent=True)

def finish():
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()

steps = [load, configure, verify_and_move, save, finish]

def tick(delta):
    global index, next_time, busy
    if busy or time.monotonic() < next_time:
        return
    busy = True
    try:
        steps[index]()
        index += 1
        next_time = time.monotonic() + 3.0
    except Exception:
        record('FAIL', error=traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    finally:
        busy = False

handle = unreal.register_slate_post_tick_callback(tick)
