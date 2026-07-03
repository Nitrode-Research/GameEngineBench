param(
    [Parameter(Mandatory = $true)]
    [string]$RunId,

    [Parameter(Mandatory = $true)]
    [string]$WorkerId,

    [Parameter(Mandatory = $true)]
    [string]$S3Root,

    [string]$TasksDir = "tasks_unreal",

    [string]$ResultsDir = "results",

    [string]$OutputFile,

    [string[]]$SnapshotDirs = @(),

    [string[]]$LogFiles = @(),

    [switch]$DryRun,

    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

function Join-S3Path {
    param([string[]]$Parts)
    $clean = @()
    foreach ($part in $Parts) {
        if ($null -eq $part -or $part -eq "") {
            continue
        }
        $clean += $part.Trim("/")
    }
    if ($clean.Count -eq 0) {
        throw "Cannot build empty S3 path"
    }
    if ($clean[0] -match "^s3://") {
        $first = $clean[0].TrimEnd("/")
        if ($clean.Count -eq 1) {
            return $first
        }
        $rest = $clean[1..($clean.Count - 1)] | Where-Object { $_ -ne $null -and $_ -ne "" }
        return $first + "/" + ($rest -join "/")
    }
    return $clean -join "/"
}

function Invoke-Aws {
    param([string[]]$AwsArgs)
    Write-Host ("aws " + ($AwsArgs -join " "))
    & aws @AwsArgs
    if ($LASTEXITCODE -ne 0) {
        throw "aws command failed with exit code $LASTEXITCODE"
    }
}

function Get-RecordArray {
    param([string]$Path)
    $json = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    if ($json -is [System.Array]) {
        return @($json)
    }
    if ($null -ne $json.tasks) {
        return @($json.tasks)
    }
    if ($null -ne $json.results) {
        return @($json.results)
    }
    throw "Unsupported results JSON shape in $Path"
}

function Same-Field {
    param($A, $B, [string]$Name)
    $av = $A.$Name
    $bv = $B.$Name
    return [string]$av -eq [string]$bv
}

function Resolve-SnapshotForRecord {
    param($Record, [string]$TestResultRoot)

    $taskId = [string]$Record.task_id
    $agent = [string]$Record.agent
    if ([string]::IsNullOrWhiteSpace($taskId) -or [string]::IsNullOrWhiteSpace($agent)) {
        throw "Result record is missing task_id or agent"
    }

    $timestamp = [string]$Record.timestamp
    if (-not [string]::IsNullOrWhiteSpace($timestamp)) {
        try {
            $dirStamp = ([DateTimeOffset]::Parse($timestamp)).UtcDateTime.ToString("yyyyMMdd_HHmmss")
            $exactDir = Join-Path $TestResultRoot "$taskId`_$agent`_$dirStamp"
            if (Test-Path -LiteralPath $exactDir) {
                $resultPath = Join-Path $exactDir "result.json"
                if (-not (Test-Path -LiteralPath $resultPath)) {
                    throw "Matched snapshot directory is missing result.json: $exactDir"
                }
                $candidateRecord = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
                if (-not (Same-Field $Record $candidateRecord "task_id")) {
                    throw "Matched snapshot has task_id mismatch: $exactDir"
                }
                if ($candidateRecord.agent -and -not (Same-Field $Record $candidateRecord "agent")) {
                    throw "Matched snapshot has agent mismatch: $exactDir"
                }
                if ($candidateRecord.model -and -not (Same-Field $Record $candidateRecord "model")) {
                    throw "Matched snapshot has model mismatch: $exactDir"
                }
                return (Resolve-Path -LiteralPath $exactDir).Path
            }
        } catch {
            if ($_.Exception.Message -like "Matched snapshot*") {
                throw
            }
        }
    }

    $matches = @()
    $candidates = Get-ChildItem -LiteralPath $TestResultRoot -Directory -Filter "$taskId`_$agent`_*" -ErrorAction SilentlyContinue
    foreach ($candidate in $candidates) {
        $resultPath = Join-Path $candidate.FullName "result.json"
        if (-not (Test-Path -LiteralPath $resultPath)) {
            continue
        }
        try {
            $candidateRecord = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
        } catch {
            continue
        }
        if (-not (Same-Field $Record $candidateRecord "task_id")) { continue }
        if (-not (Same-Field $Record $candidateRecord "agent")) { continue }
        if (-not (Same-Field $Record $candidateRecord "model")) { continue }
        if (-not (Same-Field $Record $candidateRecord "reasoning_level_applied")) { continue }
        if (-not (Same-Field $Record $candidateRecord "timestamp")) { continue }
        $matches += $candidate.FullName
    }

    if ($matches.Count -eq 0) {
        throw "Could not map result record to a snapshot directory: $taskId / $agent / $($Record.model) / $($Record.timestamp)"
    }
    if ($matches.Count -gt 1) {
        throw "Result record maps to multiple snapshot directories: $($matches -join ', ')"
    }
    return $matches[0]
}

if (-not (Get-Command aws -ErrorAction SilentlyContinue)) {
    throw "aws CLI was not found in PATH"
}

$testResultRoot = Join-Path $TasksDir "test_result"
if (-not (Test-Path -LiteralPath $testResultRoot)) {
    throw "Missing test_result directory: $testResultRoot"
}

$prefix = Join-S3Path @($S3Root, "runs", $RunId, "workers", $WorkerId)
$resolvedSnapshots = New-Object System.Collections.Generic.List[string]

if ($OutputFile) {
    if (-not (Test-Path -LiteralPath $OutputFile)) {
        throw "Output file does not exist: $OutputFile"
    }
    foreach ($record in (Get-RecordArray $OutputFile)) {
        $resolvedSnapshots.Add((Resolve-SnapshotForRecord $record $testResultRoot))
    }
}

foreach ($snapshot in $SnapshotDirs) {
    if (-not (Test-Path -LiteralPath $snapshot)) {
        throw "Snapshot directory does not exist: $snapshot"
    }
    $resolvedSnapshots.Add((Resolve-Path -LiteralPath $snapshot).Path)
}

$uniqueSnapshots = $resolvedSnapshots | Sort-Object -Unique
if ($uniqueSnapshots.Count -eq 0) {
    throw "No snapshots selected. Pass -OutputFile for solve runs or -SnapshotDirs for retest runs."
}

$dryArgs = @()
if ($DryRun) {
    $dryArgs = @("--dryrun")
}

foreach ($snapshot in $uniqueSnapshots) {
    $name = Split-Path -Leaf $snapshot
    $dest = Join-S3Path @($prefix, "test_result", $name)
    $quietArgs = @()
    if ($Quiet -and -not $DryRun) {
        $quietArgs = @("--only-show-errors")
    }
    Write-Host "Uploading snapshot: $name"
    Invoke-Aws (@("s3", "sync", $snapshot, $dest) + $dryArgs + $quietArgs)
}

if ($OutputFile) {
    $outName = Split-Path -Leaf $OutputFile
    $quietArgs = @()
    if ($Quiet -and -not $DryRun) {
        $quietArgs = @("--only-show-errors")
    }
    Write-Host "Uploading result summary: $outName"
    Invoke-Aws (@("s3", "cp", $OutputFile, (Join-S3Path @($prefix, "results", $outName))) + $dryArgs + $quietArgs)
}

foreach ($log in $LogFiles) {
    if (-not (Test-Path -LiteralPath $log)) {
        throw "Log file does not exist: $log"
    }
    $logName = Split-Path -Leaf $log
    $quietArgs = @()
    if ($Quiet -and -not $DryRun) {
        $quietArgs = @("--only-show-errors")
    }
    Write-Host "Uploading log: $logName"
    Invoke-Aws (@("s3", "cp", $log, (Join-S3Path @($prefix, "logs", $logName))) + $dryArgs + $quietArgs)
}

Write-Host "Uploaded $($uniqueSnapshots.Count) snapshot(s) to $prefix"
