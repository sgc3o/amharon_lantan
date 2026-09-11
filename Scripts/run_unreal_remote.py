import json
import sys
import time

sys.path.insert(0, r"C:\Program Files\Epic Games\UE_5.4\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution

session = remote_execution.RemoteExecution()
try:
    session.start()
    deadline = time.time() + 6.0
    nodes = []
    while time.time() < deadline:
        nodes = session.remote_nodes
        if nodes:
            break
        time.sleep(0.2)
    if not nodes:
        raise RuntimeError("No Unreal Editor remote execution node was discovered")
    # There is one active UnrealEditor process for this project.
    node = nodes[0]
    session.open_command_connection(node["node_id"])
    command = "exec(open(r'C:/Users/杉原千尋/Documents/Unreal Projects/amharon_lantan/Scripts/fix_lantern_blueprint_mobility.py', encoding='utf-8').read())"
    result = session.run_command(command, unattended=True,
                                 exec_mode=remote_execution.MODE_EXEC_FILE,
                                 raise_on_failure=True)
    print(json.dumps({"node": node, "result": result}, ensure_ascii=False, indent=2))
finally:
    session.stop()
