# Reference Update Changelog

Use this as the copy-over checklist for Overleaf. Line numbers refer to the local `paper/main.tex` after this edit pass.

## Main Text Changes

### Line 88 - Abstract result count
Replaced the stale eight-configuration / below-40% wording with:

```tex
Across ten evaluated configurations, calibrated pass rates remain below 30\%.
```

### Line 108 - Software Engineering Benchmarks
Added HumanEval/MBPP, Terminal-Bench, and a DeepSWE citation for the reference-edit-size comparison. Replace the paragraph with:

```tex
\textbf{Software Engineering Benchmarks}. Software engineering has become a central domain for evaluating LLM-based coding agents. Earlier code-generation benchmarks such as HumanEval and MBPP measure isolated function synthesis \citep{chen2021evaluating,austin2021programsynthesis}. SWE-bench evaluates whether models can resolve real GitHub issues in conventional software repositories \citep{jimenez2023swebench}; SWE-bench-Live and SWE-rebench emphasize freshness, continuous task collection, and contamination-resistant evaluation \citep{zhang2025swebenchlive,badertdinov2025swerebench}; Terminal-Bench evaluates agents on realistic command-line tasks that require operating inside an execution environment \citep{merrill2026terminalbench}. These benchmarks establish bug fixing and environment-level execution as important alternatives to isolated coding problems. \benchmarkname targets a different part of the software-engineering spectrum by evaluating larger implementation tasks inside functioning game projects. Across the 120 evaluated tasks, reference solutions add a mean of 504 lines and a median of 363 lines within the editable source files, making the tasks larger than the SWE-bench Verified and SWE-bench Pro reference edits reported by DeepSWE \citep{huang2026deepswe}. This shifts the evaluation from localized bug repair toward implementing missing behavior inside an existing runtime system.
```

### Line 110 - Frontier Agent Benchmarks
Added ProgramBench and kept DeepSWE/FrontierCode citations. Replace the paragraph with:

```tex
\textbf{Frontier Agent Benchmarks}. Recent agent benchmarks have shifted from overspecified implementation specifications to realistic behavior-focused instructions that require deeper reasoning and understanding. DeepSWE evaluates frontier coding agents on original, long-horizon software engineering tasks with broad repository coverage and behavior-focused verifiers \citep{huang2026deepswe}. ProgramBench evaluates whether agents can rebuild complete software projects from documentation and executable behavior, showing that agents often produce codebases that behave partially correctly but diverge sharply from human-written implementations \citep{yang2026programbench}. FrontierCode shares the emphasis on realistic codebase tasks, but uses maintainer-authored tasks and rubric-based grading to shift the target from behavioral correctness alone toward mergeability, evaluating whether generated code would meet production codebase standards \citep{lu2026frontiercode}. \benchmarkname follows this direction while focusing on a different domain: native C++ edits inside functioning Unreal Engine projects, where correctness depends on engine runtime behavior such as authority, replication, object lifecycle, and subsystem initialization.
```

### Line 112 - Game Development Benchmarks
Added AutoUE as the closest Unreal-generation comparison. Replace the paragraph with:

```tex
\textbf{Game Development Benchmarks}. GameDevBench establishes game development as a meaningful domain for agent evaluation with tasks derived from web and video tutorials, emphasizing multimodal game-development work such as visual scenes, shaders, sprites, and animations \citep{chi2026gamedevbench}. AutoUE studies automated 3D game generation in Unreal Engine through multi-agent systems and automated play-testing, but targets end-to-end game creation rather than constrained C++ changes inside existing projects \citep{yin2026autoue}. GameDevBench does not target multiplayer or server/client correctness, which are central sources of failure in networked game development. \benchmarkname instead evaluates native C++ changes inside existing Unreal projects. This focuses the benchmark on engine-integrated runtime behavior, where code must work with existing gameplay logic, networking, and engine architecture.
```

### Line 114 - Game Development definitions
Added the game-engine subsystem-coupling citation. The added sentence is:

```tex
This emphasis on cross-system behavior is consistent with prior work showing that game-engine subsystems are tightly coupled and hard to understand in isolation \citep{ullmann2023visualising}.
```

### Line 120 - Authoring figure reference
Changed the stale table reference to the task-authoring figure:

```tex
Figure~\ref{fig:task-authoring} summarizes this authoring, validation, and calibration process.
```

### Line 199 - Execution setup reasoning effort
Replaced stale wrapper-default wording with:

```tex
Reasoning effort & Explicit where supported by the wrapper: \texttt{gpt-5.5} uses \texttt{xhigh}/\texttt{high}/\texttt{medium}; Claude uses \texttt{max}/\texttt{high}; other wrappers use their configured default. \\
```

Also fixed the missing space before `\texttt{Kimi for Coding}` in the evaluated-model row.

### Lines 246--248 - Pass-rate figure caption
Replaced the stale caption sentence listing only GPT/Claude/Gemini with:

```tex
The set includes ten solver configurations
across GPT, Claude, Gemini, DeepSeek, Qwen, and Kimi model families.
```

### Line 263 - GPT-5.5 behavior paragraph
Replaced the incorrect claim that GPT-5.5 has the lowest calibrated rate with:

```tex
We also observe a speed-quality tradeoff within \texttt{gpt-5.5}. Its lower-effort configurations are faster but leave most tasks unsolved, while \texttt{xhigh} reaches the strongest calibrated result in the current table. This distinction matters for Unreal work because many model outputs are not simply wrong or right; they compile, satisfy some tested behaviors, and fail only when integrated into the surrounding actor, replication, or object lifecycle logic.
```

### Line 304 - Failure-mode figure discussion
Replaced the Gemini-specific compile-failure claim with a broader, safer sentence:

```tex
several other configurations lose a larger share to compile
failures, a qualitatively different failure profile that a single pass-rate
number would hide.
```

## Small Compile Guard

Added neutral definitions near the preamble so existing result-figure wrappers compile cleanly without showing review markup:

```tex
\newcommand{\added}[1]{#1}
\newcommand{\addedtag}{}
```

## Bibliography Entries Added

Add these to `references.bib` if Overleaf does not already have them:

- `chen2021evaluating` - HumanEval / Codex paper
- `austin2021programsynthesis` - MBPP / program synthesis paper
- `huang2026deepswe` - DeepSWE
- `lu2026frontiercode` - FrontierCode
- `yang2026programbench` - ProgramBench
- `yin2026autoue` - AutoUE
- `ullmann2023visualising` - game-engine subsystem coupling

The local `paper/references.bib` contains the full BibTeX entries.