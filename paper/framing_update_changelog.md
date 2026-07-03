# Framing Update Changelog

This pass changes the framing from "an Unreal benchmark" to "a benchmark for runtime-integrated / engine-integrated programming, instantiated in Unreal Engine." Use this file as the Overleaf copy checklist.

Line numbers refer to the current local `paper/main.tex` after this edit pass. They may differ slightly in Overleaf if your file has different spacing.

## Replacement Checklist

### Line 90 - Abstract

Old:

```tex
\begin{abstract}Coding models have made rapid progress on general software engineering tasks, but their capabilities on Unreal Engine work are less well understood. This paper presents \benchmarkname, a benchmark for Unreal Engine 5 native C++ tasks that evaluates models on real engine code through compilation, hidden tests, and LLM-as-judge calibration. The evaluated set spans 120 tasks from nine Unreal projects. These projects exercise several common kinds of Unreal C++ work, including multiplayer gameplay, combat, map logic, animation and movement code, UI and session code, loading behavior, online-service integration, save/persistence systems, data serialization, XR behavior, and rendering-oriented plugin code. The set is strongest for gameplay code and runtime systems that coordinate several parts of a project. It has thinner coverage of lower-level or specialist C++ work such as audio, performance, platform-specific code, security, and developer tools. Across ten evaluated configurations, calibrated pass rates remain below 30\%. Even after judge calibration for necessarily partial hidden tests, many tasks remain unsolved. Common failures include authority mistakes, lifecycle bugs, and incomplete integration with the surrounding game systems. Tasks sourced from real game projects provide the practical difficulty needed to evaluate generated Unreal C++ code.
\end{abstract}
```

New:

```tex
\begin{abstract}Current coding benchmarks measure important forms of repository-level software engineering, but they still underrepresent programs whose correctness depends on a live runtime, framework rules, distributed state, object lifecycles, and interactions among existing systems. Game engines concentrate these requirements in a form that can be compiled, executed, and tested. This paper presents \benchmarkname, a benchmark for engine-integrated C++ programming built from Unreal Engine 5 projects. The evaluated set spans 120 tasks from nine real game repositories and covers gameplay mechanics, multiplayer behavior, AI and world orchestration, animation and movement code, UI and session code, loading behavior, online-service integration, persistence, data serialization, XR behavior, and rendering-oriented plugin code. Across ten evaluated configurations, calibrated pass rates remain below 30\%. Even after judge calibration for necessarily partial hidden tests, many tasks remain unsolved. The dominant failures are not superficial syntax errors; they involve authority mistakes, lifecycle bugs, state-synchronization failures, and incomplete integration with surrounding game systems. \benchmarkname uses Unreal Engine as a demanding testbed for a broader question: whether coding agents can modify code that must work inside an existing runtime system.
\end{abstract}
```

### Lines 94-103 - Introduction Opening And Contributions

Old starts at:

```tex
\section{Introduction}
Game development is a demanding evaluation domain for AI coding agents because correct solutions must be correct not only in syntax and local logic, but also under the rules imposed by the game engine.
```

Old ends at:

```tex
By evaluating agents on realistic Unreal Engine C++ tasks, \benchmarkname provides a focused measure of current coding-agent capability in Unreal gameplay and systems programming.
```

Replace that whole block with:

```tex
\section{Introduction}
Many software systems are not just collections of functions. They run inside frameworks that decide when objects are created, which machine owns a decision, how state moves between processes, which callbacks fire during initialization or teardown, and how local code interacts with the rest of the project. A patch can compile and still be wrong if it runs at the wrong time, updates the wrong copy of state, misses a lifecycle transition, or breaks an interaction with another subsystem. These failures are central to production software, but they are difficult to expose with isolated programming tasks or repository benchmarks focused mainly on localized bug repair.

Game engines provide a dense and practical setting for evaluating this class of capability. Engine code must satisfy ordinary C++ requirements while also obeying runtime contracts imposed by gameplay frameworks, networking systems, actor lifecycles, asset loading, event timing, and subsystem initialization. In a networked game, for example, a change may need to run on the authoritative server, replicate state to clients, update UI only for the local player, and clean up correctly when actors are destroyed or reused. These are not Unreal-only concerns; they are examples of framework-integrated programming, where correctness depends on how generated code behaves inside a live system.

\benchmarkname uses Unreal Engine as the testbed because it exposes these runtime constraints through executable C++ projects. The benchmark does not claim to cover every part of game development. It is strongest for gameplay-facing systems work: mechanics, networking behavior, object lifetime rules, configuration logic, plugin integration, and coordination between existing classes. It is thinner for specialist areas such as audio, performance, platform support, editor tools, build systems, backend services, and security-sensitive systems. This scope is deliberate: the benchmark targets the engine-integrated programming failures that current agents still make, while leaving other game-engine disciplines for future expansion.

This framing matters for evaluating AI-assisted engineering tools. A generated patch that looks plausible in code review can still be unusable if it updates only local state, forgets to notify clients, registers a subsystem too late, leaks actor state across reuse, or breaks behavior during teardown. Measuring success only through syntax, compilation, or generic repository repair misses these failures. \benchmarkname evaluates code in working projects, after hidden PIE automation tests are injected and after judge calibration checks whether the implementation satisfies the requested behavior rather than merely matching one reference solution.

We make three contributions. First, we introduce a 120-task benchmark for engine-integrated C++ programming, instantiated in Unreal Engine projects. The models only edit native source files.\footnote{Some source projects use non-code assets during execution, but benchmarked edits are restricted to native C++ files.} Second, we define an evaluation protocol that combines scoped file edits, hidden runtime tests, and judge calibration to score behavioral correctness rather than reference similarity. Third, we analyze why current agents fail, showing that many errors concentrate in authority handling, state synchronization, object lifecycle handling, and initialization. By grounding these failures in executable game projects, \benchmarkname provides a concrete measure of current coding-agent capability on runtime-integrated software systems.
```

### Line 110 - Software Engineering Benchmarks Sentence

Old:

```tex
\benchmarkname targets a different part of the software-engineering spectrum by evaluating larger implementation tasks inside functioning game projects.
```

New:

```tex
\benchmarkname targets a different part of the software-engineering spectrum by evaluating larger implementation tasks inside functioning runtime systems, using game projects as the concrete execution environment.
```

### Line 112 - Frontier Agent Benchmarks Ending

Old:

```tex
\benchmarkname follows this direction while focusing on a different domain: native C++ edits inside functioning Unreal Engine projects, where correctness depends on engine runtime behavior such as authority, replication, object lifecycle, and subsystem initialization.
```

New:

```tex
\benchmarkname follows this direction while focusing on runtime-integrated C++ programming: native edits inside functioning Unreal Engine projects, where correctness depends on framework behavior such as authority, state synchronization, object lifecycle, and subsystem initialization.
```

### Line 114 - Game Development Benchmarks Ending

Old:

```tex
\benchmarkname instead evaluates native C++ changes inside existing Unreal projects. This focuses the benchmark on engine-integrated runtime behavior, where code must work with existing gameplay logic, networking, and engine architecture.
```

New:

```tex
\benchmarkname instead evaluates native C++ changes inside existing Unreal projects. This focuses the benchmark on runtime-integrated programming, where code must work with existing gameplay logic, networking, and engine architecture rather than only produce standalone game assets or local code snippets.
```

### Line 310 - Failure Analysis Opening

Old:

```tex
The remaining failures concentrate in a small set of Unreal-specific coordination problems: authority boundaries, replication, object lifecycle, initialization order, and default configuration.
```

New:

```tex
The remaining failures concentrate in a small set of engine-level coordination problems: authority boundaries, replication, object lifecycle, initialization order, and default configuration.
```

### Line 369 - Conclusion Paragraph

Old:

```tex
\benchmarkname marks a concrete step toward benchmark design for AI-assisted game-engine programming. Instead of evaluating simpler execution outcomes and syntactic accuracy, our benchmark aligns coding outputs with specified behavioral requirements, requiring a greater understanding of the surrounding game system by the agent. The tasks are extracted from functional Unreal Engine repositories, enabling the benchmark to cover production-like C++ runtime behaviors while leaving several kinds of C++ game programming for future expansion. The 120-task set shows that frontier coding agents do not solve this gameplay-facing C++ systems slice by default. The remaining failures are engine-semantic and integration-heavy rather than superficial syntax or compile errors. While our tasks are specific to Unreal Engine, the gaps in model performance reflect broader challenges in engine- and framework-integrated software development, where generated code must work inside an existing project, follow framework rules about where and when code should run, and preserve behavior under realistic runtime conditions. That makes \benchmarkname useful today as a focused measure of difficult Unreal C++ work and a foundation for broader game-development evaluation later.
```

New:

```tex
\benchmarkname marks a concrete step toward evaluating AI agents on runtime-integrated software, where generated code must work inside an existing system rather than only satisfy local syntax or isolated tests. We instantiate this problem in Unreal Engine because game projects expose many of the hard cases at once: framework rules, distributed state, object lifecycles, subsystem initialization, and interactions across existing gameplay code. The 120-task set shows that frontier coding agents do not solve this setting by default. Their failures are integration-heavy rather than superficial: the code often compiles and uses plausible APIs, but still violates the runtime contract required by the surrounding system. While the current benchmark is specific to Unreal Engine, the model gaps it reveals apply more broadly to framework-heavy software development. Agents need to understand not only what code to write, but where that code runs, when it runs, what state it owns, and which other systems depend on it. That is the broader capability \benchmarkname is designed to measure.
```