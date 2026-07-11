---
name: log-ai
description: Appends a row to the AI-Assisted Development table in README.md to declare AI usage as required by course policy. Use when user says "log AI", "log-ai", "log what we did", "declare AI usage", "registrar uso de IA", or asks to document AI assistance for the project. Collects model name, type of use, and prompt description if not provided.
metadata:
  author: Abstractize
  version: 1.0.0
  category: documentation
---

# Log AI Usage

## Instructions

Append a new row to the AI-Assisted Development table in README.md to declare AI usage as required by the course policy (see `docs/Enunciado.md`).

### Step 1: Gather information

**Priority order for each field:**

1. Explicit arguments — if the user provided pipe-separated values (`Model | Type of use | Prompt`), use those.
2. Conversation context — infer from the current chat session:
   - **Model**: read the model name from the session metadata or system context (e.g. `Claude Sonnet 4.6`). The interface is always `Claude Code` when running inside Claude Code.
   - **Type of use**: classify what actually happened in the session. Choose all that apply from: `concept lookup`, `code generation`, `documentation generation`, `diagram generation`, `code review`, `debugging`, `writing improvement`.
   - **Prompt**: summarize the core request of the session in one sentence — what the user asked for, not a list of every step taken.
3. Ask the user — only if a field cannot be confidently inferred from the conversation.

Prefer inference over asking. If the session covered multiple distinct topics, propose one combined row that captures the overall request.

### Step 2: Locate the table

Open README.md and find the `## AI-Assisted Development` section. The table has three columns: **Model**, **Type of use**, **Prompt**.

### Step 3: Append the row

Add a new row at the bottom of the table, matching the existing format exactly:

```
| <Model> ([Claude Code](https://claude.ai/code)) | <Type of use> | *"<Prompt>"* |
```

Do not modify any other part of README.md.

### Step 4: Confirm

Report the exact row that was added so the user can verify it.

## Examples

**User:** "log-ai — Claude Sonnet 4.6 | code generation | implement the Bus module routing logic"

**Action:** Appends:
```
| Claude Sonnet 4.6 ([Claude Code](https://claude.ai/code)) | code generation | *"implement the Bus module routing logic"* |
```

## Common Issues

### README.md not found
Cause: Not running from the project root.
Solution: Confirm the working directory contains README.md before editing.

### Table not found
Cause: README.md structure was changed.
Solution: Search for "AI-Assisted Development" heading; if the table is missing, recreate it with the three-column header before appending.
