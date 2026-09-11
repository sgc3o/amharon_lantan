import unreal
import json
import os
import time
import traceback
import hashlib
from pathlib import Path

ROOT = Path(unreal.Paths.project_saved_dir()) / 'LanternClonerSafety'
PHASE = (ROOT / 'phase.txt').read_text().strip()
MAP = '/Game/LanternCloner/Maps/L_LanternCloner_Test'
RESULT = ROOT / ('results_' + PHASE + '.json')
events = []
state = {}
index = 0
busy = False
next_time = time.monotonic() + 8

def log(step, **data):
    event = dict(step=step, pid=os.getpid(), **data)
    events.append(event)
    RESULT.write_text(json.dumps(events, indent=2), encoding='utf-8')
    unreal.log('LANTERN_SAFETY|' + json.dumps(event))

def actor():
    found = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
             if isinstance(a, unreal.LanternCloner)]
    assert len(found) == 1, 'Expected one LanternCloner: ' + str(len(found))
    return found[0]

def snapshot():
    a = actor()
    assert a.is_generated_data_consistent(), 'HISM tree/build/reorder inconsistency'
    components = sorted(a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent), key=lambda c: c.get_name())
    digest = hashlib.sha256()
    counts = {}
    for c in components:
        n = c.get_instance_count()
        counts[c.get_name()] = n
        digest.update(c.get_name().encode())
        for i in range(n):
            t = c.get_instance_transform(i, False)
            numbers = [t.translation.x, t.translation.y, t.translation.z,
                       t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w,
                       t.scale3d.x, t.scale3d.y, t.scale3d.z]
            digest.update(json.dumps(numbers).encode())
    return dict(counts=counts, total=sum(counts.values()), digest=digest.hexdigest(), tree_consistent=True)

def open_map():
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    log('open_requested')

def loaded():
    s = snapshot()
    state['initial'] = s
    a = actor()
    log('loaded', snapshot=s, columns=a.get_editor_property('column_count'),
        depth=a.get_editor_property('depth_count'), vertical=a.get_editor_property('vertical_count'))
    if PHASE == 'restart':
        expected = json.loads((ROOT / 'expected_saved.json').read_text())
        assert s == expected, 'Saved instances changed on editor restart/load'
        assert a.get_editor_property('generated_mesh_instance_count') == s['total'], 'Saved stats lost'
        log('restart_persistence_pass')

def close_map():
    state.pop('actor', None)
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    log('closed_to_unsaved_blank_map')

def reopened():
    assert snapshot() == state['initial'], 'Load/reopen changed saved data'
    log('reopen_unchanged_pass')

def clear():
    state['before_clear'] = snapshot()
    state['retained'] = list(actor().get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent))
    actor().clear()

def cleared():
    s = snapshot()
    assert s['total'] == 0
    assert all(unreal.SystemLibrary.is_valid(c) for c in state['retained']), 'Clear invalidated components'
    assert set(s['counts']) == set(state['before_clear']['counts']), 'Clear destroyed components'
    log('clear_pass', snapshot=s)

def generate():
    actor().generate()

def generated():
    s = snapshot()
    assert s['total'] > 0
    assert s['total'] == actor().get_editor_property('generated_mesh_instance_count')
    state['generated'] = s
    state['generated_components'] = list(actor().get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent))
    log('generate_pass', snapshot=s, lanterns=actor().get_editor_property('generated_lantern_count'))

def regenerate():
    actor().regenerate()

def regenerated():
    assert snapshot() == state['generated'], 'Regenerate changed identities/counts/transforms'
    assert set(actor().get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)) == set(state['generated_components'])
    log('regenerate_reuse_determinism_pass')

def property_edit():
    a = actor()
    state['seed'] = a.get_editor_property('random_seed')
    a.set_editor_property('random_seed', state['seed'] + 1)

def property_checked():
    assert snapshot() == state['generated'], 'Details change auto-generated'
    actor().set_editor_property('random_seed', state['seed'])
    log('details_no_auto_generate_pass')

def preview():
    a = actor()
    values = dict(column_count=10, depth_count=10, vertical_count=100, density=1.0, b_enable_foreground_center_gap=False)
    # Python strips the leading C++ boolean prefix.
    values['enable_foreground_center_gap'] = values.pop('b_enable_foreground_center_gap')
    state['preview_settings'] = {k: a.get_editor_property(k) for k in values}
    for k, v in values.items():
        a.set_editor_property(k, v)
    assert snapshot() == state['generated'], 'Large layout edit auto-generated'
    a.preview()

def preview_checked():
    s = snapshot()
    assert actor().get_editor_property('generated_lantern_count') == 64
    assert s['total'] <= 192
    log('preview_cap_pass', snapshot=s)
    for k, v in state['preview_settings'].items():
        actor().set_editor_property(k, v)
    actor().generate()

def save():
    s = snapshot()
    assert s == state['generated'], 'Generate after preview not deterministic'
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().startswith(MAP + '.'), 'Refusing to save a different map'
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP), 'Test map save failed'
    (ROOT / 'expected_saved.json').write_text(json.dumps(s), encoding='utf-8')
    log('test_level_saved', snapshot=s)

def finish():
    log('PASS', phase=PHASE)
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.quit_editor()

steps = ([open_map, loaded, close_map, open_map, reopened, clear, cleared, generate, generated,
          regenerate, regenerated, property_edit, property_checked, preview, preview_checked, save, finish]
         if PHASE == 'sequence' else [open_map, loaded, close_map, open_map, reopened, finish])

def tick(delta):
    global index, next_time, busy
    if busy or time.monotonic() < next_time:
        return
    busy = True
    try:
        fn = steps[index]
        log('running', action=fn.__name__)
        fn()
        index += 1
        next_time = time.monotonic() + 4
    except Exception:
        log('FAIL', error=traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    finally:
        busy = False

log('editor_script_started', phase=PHASE)
handle = unreal.register_slate_post_tick_callback(tick)
