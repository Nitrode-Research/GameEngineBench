#!/usr/bin/env python3
"""
Centralized prompt creation for gamedev benchmark solvers.

This module provides unified prompt creation functions used by all solver implementations,
ensuring consistency across different agents.
"""

import json
from typing import Optional


def load_task_config() -> Optional[dict]:
    """Load task configuration from task_config.json in current directory.

    Returns:
        Parsed task configuration dict, or None if loading fails
    """
    try:
        with open("task_config.json", "r") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading config: {e}")
        return None


def create_task_prompt(config: dict, use_runtime_video: bool = False, use_mcp: bool = False) -> str:
    """Create task prompt. Routes to engine-specific helper based on config['engine'].

    Args:
        config: Task configuration dict containing 'instruction' field
        use_runtime_video: Whether to append Godot runtime video instructions (Godot only)
        use_mcp: Whether to include MCP tool references (Godot only)

    Returns:
        The task prompt string
    """
    engine = config.get("engine", "godot") if config else "godot"
    if engine == "unreal":
        return _create_unreal_prompt(config, use_mcp)
    return _create_godot_prompt(config, use_runtime_video, use_mcp)


def _create_godot_prompt(config: dict, use_runtime_video: bool = False, use_mcp: bool = False) -> str:
    try:
        if not config or "instruction" not in config:
            raise ValueError("Invalid config: 'instruction' field missing")
    except Exception as e:
        print(f"Error creating task prompt: {e}")
        return ""
    instruction = config.get("instruction")

    instruction += "\n You must complete the full task without any further assistance."
    instruction += "\n Godot is installed and you can run godot using the `godot` command. It is recommended to run this with a timeout (e.g., `timeout 10 godot` for 10 second timeout) to prevent hanging."
    instruction += "You are a visual agent and can use images and videos to help you understand the state of the game."

    if use_runtime_video:
        runtime_guidance = """
    - You can run the game and get an image with `godot --path . --quit-after 1
    --write-movie output.png`.
    - You can save a movie file as avi instead with `timeout 60s godot --path . --quit-after 60 --write-movie output.avi`. This is a 1 second or 60 frame video. You can adjust as necessary.
    - It is very important that you ensure godot closes after running, or else the task will hang indefinitely.
    - You should use the video or images to verify that your changes worked as expected.
    """
        instruction += runtime_guidance

    if use_mcp:
        mcp_guidance = """

You have access to a Godot MCP (Model Context Protocol) server that provides specialized tools for working with Godot projects.

Available MCP Tools:
- `godot-screenshot`: Takes a screenshot of the Godot editor to help you visualize the current state of the project.
  - The game directory is the current directory (`./`)
  - This is useful for understanding the scene hierarchy, node structure, and visual layout
  - You can use this before making changes to understand the current state, and after to verify your changes

When to use the MCP tools:
- Before starting work: Use `godot-screenshot` to understand the current project structure
- After making changes: Use `godot-screenshot` to verify your changes are correct
- When debugging: Use `godot-screenshot` to see what the editor looks like and identify issues
"""
        instruction += mcp_guidance

    return instruction


def _create_unreal_prompt(config: dict, use_mcp: bool = False) -> str:
    """Create prompt for Unreal Engine C++ tasks.

    Reads files_to_edit relative to cwd — the runner has already os.chdir'd
    into the workspace before this is called via BaseSolver.get_task_prompt().

    When use_mcp=True (hybrid mode), appends guidance on calling MCP tools for
    editor actions and writing editor_action_log.json.
    """
    if not config or "instruction" not in config:
        print("Error creating Unreal prompt: 'instruction' field missing")
        return ""

    files_to_edit = config.get("files_to_edit", [])
    instruction = config.get("instruction", "")
    capability_mode = config.get("capability_mode", "code_only")

    config_files_to_edit = config.get("config_files_to_edit", [])
    all_editable = files_to_edit + config_files_to_edit
    all_editable_list = "\n".join(f"- {f}" for f in all_editable)

    base_prompt = f"""You are implementing an Unreal Engine 5.7 task.

## Task
{instruction}

## Files to edit
{all_editable_list}

Edit only the files listed above. Do not modify .uproject, Target.cs, or Build.cs files.
You must complete the full task without any further assistance.
Complete the task and make sure the implementation is correct and compiles cleanly.
You may run shell commands and local build tools to verify compilation. Before you finish, verify that the code compiles and fix any compile errors you encounter.
Do not launch the Unreal Editor, do not run automation tests, and do not write test files.
"""

    if use_mcp and capability_mode == "hybrid":
        agent_must_place = config.get("agent_must_place", [])
        agent_must_wire = config.get("agent_must_wire", [])
        authoring_domains = config.get("authoring_domains", [])

        place_list = "\n".join(f"- {a}" for a in agent_must_place) or "  (see task description)"
        wire_list = "\n".join(f"- {w}" for w in agent_must_wire) or "  (see task description)"

        mcp_guidance = f"""
## Editor actions required
This task requires level authoring in addition to C++ edits. You have access to an Unreal MCP server
(tool prefix: `mcp__unreal__`) that drives a live Unreal Editor session. Use it to complete the
editor-side requirements.

Authoring domains: {', '.join(authoring_domains) if authoring_domains else 'see task description'}

Actors you must place:
{place_list}

Properties / wiring you must set:
{wire_list}

Available MCP tools (use these exact names with the `mcp__unreal__` prefix):
- `spawn_actor` — place an actor in the level (params: name, type, location, rotation)
- `set_actor_property` — set a property on a placed actor (params: name, property_name, value)
- `get_actors_in_level` — list all actors currently in the level
- `find_actors_by_name` — find actors by name pattern
- `set_actor_transform` — set position/rotation/scale of an actor
- `spawn_blueprint_actor` — spawn an actor from a Blueprint class
- `create_blueprint` — create a new Blueprint class
- `set_blueprint_property` — set a property on a Blueprint default object
- `compile_blueprint` — compile a Blueprint after editing
- `create_input_mapping` — add an axis/action input mapping to the project

Note: there is no save_map tool. The runner saves the project automatically after the solver completes.

After completing all editor actions, write a file named `editor_action_log.json` in the current
directory containing a JSON array of action records. Each record must include at minimum:
  {{"action": "<tool_name>", "timestamp": "<ISO8601>", ...relevant params...}}

Example:
  [{{"action": "spawn_actor", "name": "DayNightCycleActor", "type": "ADayNightCycleActor", "location": [0,0,0], "timestamp": "2026-03-27T19:01:03Z"}}]

The editor_action_log.json is a first-class deliverable — write it even if some actions failed.
"""
        base_prompt += mcp_guidance

    return base_prompt


def create_system_prompt(use_mcp: bool = False) -> str:
    """Create system prompt for Godot game development tasks.

    Args:
        use_mcp: Deprecated - MCP guidance is now in create_task_prompt

    Returns:
        System prompt string
    """
    return "You are a Godot game development expert."
