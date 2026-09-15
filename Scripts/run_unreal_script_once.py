import json
import sys
import time

sys.path.insert(0, r"C:\Program Files\Epic Games\UE_5.4\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution

script_path = sys.argv[1].replace("\\", "/")
session = remote_execution.RemoteExecution()
try:
    session.start()
    deadline = time.time() + 8.0
    while time.time() < deadline and not session.remote_nodes:
        time.sleep(0.2)
    if len(session.remote_nodes) != 1:
        raise RuntimeError(f"Expected one Unreal node, found {len(session.remote_nodes)}")
    node = session.remote_nodes[0]
    session.open_command_connection(node["node_id"])
    command = f"exec(open(r'{script_path}', encoding='utf-8').read())"
    result = session.run_command(
        command,
        unattended=True,
        exec_mode=remote_execution.MODE_EXEC_FILE,
        raise_on_failure=True,
    )
    print(json.dumps(result, ensure_ascii=False))
finally:
    session.stop()
