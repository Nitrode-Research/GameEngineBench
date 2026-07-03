#!/usr/bin/env bash
# Setup a new Bomber benchmark task from the Bomber repo.
# Usage: ./setup_bomber_task.sh <task_id> <strip_commit> <solution_commit>
# Example: ./setup_bomber_task.sh ue_task_0026_npc_controller abc1234 def5678

set -e

TASK_ID="$1"
STRIP_COMMIT="$2"
SOLUTION_COMMIT="$3"

if [[ -z "$TASK_ID" || -z "$STRIP_COMMIT" || -z "$SOLUTION_COMMIT" ]]; then
  echo "Usage: $0 <task_id> <strip_commit> <solution_commit>"
  exit 1
fi

BOMBER_REPO=/Users/sejoonchang/Desktop/Bomber
TASKS_DIR=/Users/sejoonchang/Desktop/gamedevbench/tasks_unreal
TASK_DIR="$TASKS_DIR/$TASK_ID"
PLUGIN_SOURCE="$TASKS_DIR/ue_task_0031_generated_map_orchestrator/solution/Plugins"

export PATH="$HOME/bin:$PATH"

# 1. Create directories
mkdir -p "$TASK_DIR/start" "$TASK_DIR/solution" "$TASK_DIR/tests"

# 2. Extract source files from Bomber repo
echo "Extracting start/ from $STRIP_COMMIT..."
cd "$BOMBER_REPO"
git archive "$STRIP_COMMIT" Source Config Content Bomber.uproject | tar -x -C "$TASK_DIR/start"
# Also extract Plugins/ — succeeds for newer commits where plugins are vendored (not gitlinks).
# This is critical: it gets the correctly STRIPPED versions of any plugin files.
# Failures are safe (older commits have plugins as gitlinks; task 53 copy fills gaps below).
git archive "$STRIP_COMMIT" -- Plugins 2>/dev/null | tar -x -C "$TASK_DIR/start" || true

echo "Extracting solution/ from $SOLUTION_COMMIT..."
git archive "$SOLUTION_COMMIT" Source Config Content Bomber.uproject | tar -x -C "$TASK_DIR/solution"
git archive "$SOLUTION_COMMIT" -- Plugins 2>/dev/null | tar -x -C "$TASK_DIR/solution" || true

# 3. Copy only missing plugins from task 53 (fills gaps left by gitlinks in older commits)
echo "Copying missing plugins from task 53 reference..."
mkdir -p "$TASK_DIR/start/Plugins" "$TASK_DIR/solution/Plugins"
for p in AdvancedSessions AdvancedSteamSessions DataAssetsLoader FunctionPicker \
          GameFeatures InstancedStaticMeshConverter MetaCheatManager MorphsPlayer \
          MyEditorUtils PoolManager SettingsWidgetConstructor; do
  for dir in start solution; do
    DEST="$TASK_DIR/$dir/Plugins/$p"
    if [ -z "$(ls -A "$DEST" 2>/dev/null)" ]; then
      cp -R "$PLUGIN_SOURCE/$p" "$DEST"
      echo "  Copied $p -> $dir/"
    else
      echo "  Skipped $p ($dir/ already has content — from git archive)"
    fi
  done
done

# 4. Fix DDC path (UE crashes if workspace path > 119 chars)
echo "Patching DDC path..."
for dir in start solution; do
  INI="$TASK_DIR/$dir/Config/DefaultEngine.ini"
  if [ -f "$INI" ]; then
    sed -i '' 's|Path=%GAMEDIR%DerivedDataCache/|Path=/tmp/ue_ddc/bomber/|' "$INI"
  fi
done

echo ""
echo "Done. Next steps:"
echo "  1. Create spec.yaml from the branch's public_prompt.md"
echo "  2. Create evaluator notes in ~/Desktop/benchmark-private/"
echo "  3. Run the authoring pipeline:"
echo "     cd /Users/sejoonchang/Desktop/gamedevbench && env -u PYTHONHOME -u PYTHONPATH -u __PYVENV_LAUNCHER__ .venv/bin/python -m unrealbench.src.authoring.generate_task_tests --task-id $TASK_ID --authoring-mode cli_agent --authoring-agent claude-code --evaluator-notes ~/Desktop/benchmark-private/<name>-evaluator.md"
