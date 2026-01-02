# **Mini Git | C++**

A minimal Git-like version control system implemented in C++, built from scratch to understand how Git works internally — commits, snapshots, staging, status tracking, and history traversal.

This project is educational by design, focusing on correctness, filesystem mechanics, and clean architecture rather than full Git feature parity.

## **Features**

* Repository initialization (init)
* File staging (add)
* Snapshot-based commits (commit)
* Commit history traversal (log)
* Modified / staged / untracked file detection (status)
* Revert to previous commit (revert)
* File-level snapshot inheritance
* Binary-safe file comparison
* Clean separation of responsibilities (Core / Manager / CLI)
* Architecture Overview


### The project is intentionally split into three layers: ###

1️. **Core Layer (core.cpp)**

* Handles low-level version control mechanics:
* Commit creation
* Parent commit inheritance
* Snapshot storage
* Commit metadata
* Commit history traversal

This layer never interacts with CLI arguments.

2️. **Manager Layer (manager.cpp)**

* Acts as the logic bridge:
* Implements add, commit, status, log, revert
* Tracks file states:
   * Staged
   * Modified but not staged
   * Untracked
* Handles staging area lifecycle
* Compares working directory ↔ commit snapshots

This layer orchestrates behavior, not storage.

3️. **CLI Layer (main.cpp)**

The user interface:
* Parses terminal commands
* Validates arguments
* Routes commands to the manager
* Displays clean, readable output

## **Build Instructions**

### Compile
```bash
g++ main.cpp -o mygit
```
Run 
```bash
.\mygit <command>
```
## Supported Commands
1. Initialize Repository
```bash
.\mygit init
```
Creates:

- .git/
- .git/staging_area/
- .git/commits/
- .git/HEAD

2. Add Files
```bash
.\mygit add .
.\mygit add file1.cpp
```
- Adds only changed files
- Skips unchanged files automatically
- Preserves directory structure
  
3. Commit Changes
```bash
.\mygit commit -m "your message"
```
- Creates a full snapshot
- Inherits unchanged files from parent commit
- Clears staging area safely

4. Check Status
```bash
.\mygit status
```
Displays:
- Changes to be committed
- Changes not staged for commit
- Untracked files
- Or a clean working tree message

5. View Commit History
```bash
.\mygit log
```
Shows:
- Commit ID
- Commit message
- Timestamp
- Traverses parents backwards

6. Revert Commit
```bash
.\mygit revert <commitHash>
.\mygit HEAD
```
Creates a new commit that reverts the project to a previous snapshot.

## **Design Decisions**

- Snapshot-based storage (like Git, not diff-based)
- Binary-safe file comparisons
- Filesystem-first implementation
- No global state
- Clear lifecycle rules:
   * Only commit clears staging
   * add is incremental
   * status is read-only

## **Limitations (By Design)**
- No branches
- No merges
- No .gitignore
- No diff output
- No networking / remote support

These are intentionally omitted to keep the core concepts clear.
