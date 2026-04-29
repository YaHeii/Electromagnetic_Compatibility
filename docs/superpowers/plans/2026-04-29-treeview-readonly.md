# TreeView Readonly Overview Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a readonly tree overview page that renders `DataModel` as a searchable hierarchy.

**Architecture:** Keep the existing `TreeView`, `T_TreeViewModel`, and `T_TreeItem` structure. Rebuild the model from `DataModel` on demand, then let `TreeView` provide refresh, expand, collapse, and first-match selection behaviors.

**Tech Stack:** Qt Widgets, `QAbstractItemModel`, Catch2, existing Ela UI controls

---

### Task 1: Add Failing Tree Model Tests

**Files:**
- Create: `Tests/TreeViewModelTests.cpp`
- Modify: `CMakeLists.txt`
- Test: `Tests/TreeViewModelTests.cpp`

- [ ] Add failing tests for tree reload and keyword search.
- [ ] Add the new test file and tree model sources to `SchemaDtoValidationTests`.
- [ ] Ask the user to rebuild tests and confirm the new red state before relying on the implementation.

### Task 2: Implement Readonly Tree Model

**Files:**
- Modify: `ModelView/T_TreeItem.h`
- Modify: `ModelView/T_TreeItem.cpp`
- Modify: `ModelView/T_TreeViewModel.h`
- Modify: `ModelView/T_TreeViewModel.cpp`
- Test: `Tests/TreeViewModelTests.cpp`

- [ ] Remove dead check-state/edit semantics from the tree model path.
- [ ] Implement `reloadFromDataModel()` to rebuild the three top-level groups from current `DataModel`.
- [ ] Implement readonly `data()`, `flags()`, `headerData()`, and recursive node counting.
- [ ] Implement case-insensitive contains-based `findItemIndex()`.

### Task 3: Implement TreeView Page Interaction

**Files:**
- Modify: `resource/ui/TreeView.h`
- Modify: `resource/ui/TreeView.cpp`
- Test: manual verification in UI

- [ ] Promote the local tree model instance to a page member.
- [ ] Add refresh button, search input, search button, and result text.
- [ ] Add `syncViewWithModel()`, `expandAll()`, `collapseAll()`, and `findAndSelect()`.
- [ ] Refresh once on construction and support later manual refresh.

### Task 4: Verification Handoff

**Files:**
- None

- [ ] Ask the user to rebuild.
- [ ] Ask the user to run `ctest --output-on-failure`.
- [ ] Ask the user to manually verify refresh, expand, collapse, and search on the Home tree page.
