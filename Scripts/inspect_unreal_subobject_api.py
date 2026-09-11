from pathlib import Path
import unreal

out = Path(unreal.Paths.project_saved_dir()) / "UnrealSubobjectApi.txt"
classes = [
    unreal.SubobjectDataSubsystem,
    unreal.SubobjectDataBlueprintFunctionLibrary,
    unreal.BlueprintEditorLibrary,
    unreal.BlueprintFactory,
    unreal.AddNewSubobjectParams,
    unreal.SubobjectData,
    unreal.SubobjectDataHandle,
]
lines = []
for cls in classes:
    lines.append("=== " + cls.__name__ + " ===")
    for name in dir(cls):
        if name.startswith("_"):
            continue
        try:
            value = getattr(cls, name)
            doc = getattr(value, "__doc__", "") or ""
            lines.append(name + "\n" + doc[:3000])
        except Exception as exc:
            lines.append(name + " ERROR " + str(exc))
out.write_text("\n".join(lines), encoding="utf-8")
unreal.log("WROTE_SUBOBJECT_API=" + str(out))
