# Workflow

How work on this project is done: how the assistant responds, plans and uses git, and how
a change is made. There are two kinds of change, each with its own section and its own
checks: a [rule change](#rule-changes) and a [code change](#code-changes).

# Responses

- Keep responses concise and to the point - unless the user asks otherwise
- Use U.S. English, not U.K. English.

# Planning mode

- Always ask clarifying questions
- Never assume design, tech stack or features
- Use deep-dive sub-agents to assist with research
- Use deep-dive sub-agents to review the different aspects of your plan before presenting to the user

# Git

- Never run a git command that changes anything yourself (commit, push, revert, reset, checkout, fetch, pull, merge, stash, branch or tag changes, submodule updates, `git config`) - in this repo or in the `lib/` submodules - unless the user explicitly asks for it in their current message
- Read-only git is allowed without asking: `status`, `log`, `diff`, `show`, `rev-parse`, `ls-files`, `blame`, listing branches and remotes
- An earlier request or approval in the chat does not carry over: every git action needs a fresh, explicit instruction
- The same applies to sub-agents - pass this rule on when delegating
- When git work is needed, give the user the exact commands and stop

# Rule changes

A rule change is a change to the content of `md/`.

`md/` is one knowledge base, not a set of separate files: a rule written in one file is
relied on by the others. A change is therefore not finished when the file it was made in
reads well. It is finished when the whole of `md/` still does.

Every change, however small, is followed by four checks. The first three are made over
all of `md/`, not only over the file that was changed. The fourth is made over the code.

A rule is never changed silently: the question is raised first, unless the user asked for
that change explicitly.

## Consistency

Nothing in `md/` may contradict the change.

A rule is usually stated in one place and assumed in several others: in a later section
of the same file, in a code example, in another file that builds on it. After a change,
each of those places is found and brought in line. A code example is a statement of the
rule like any other and is changed with it.

When the change contradicts something that was decided on purpose, the contradiction is
not resolved silently in either direction. It is raised, and one of the two is changed
deliberately.

## Redundancy

A change must not pile up on top of what is already there.

Before something is added, what the knowledge base already says on the subject is found.
If the new text repeats it, restates it more strongly or covers the same ground from
another side, the two are settled into one: the existing text is changed, or the new text
replaces it. Each thing is said once, in the file it belongs to, and other files link to
it instead of saying it again.

A change that makes older text unnecessary removes that text. A rule that has been
replaced is deleted, not left beside its replacement, and an open question that the change
answers is taken off the list of open questions.

## Integrity

Nothing in `md/` may refer to what no longer exists.

After something is renamed, moved or removed, every reference to it is found and fixed or
removed:

- links to a file or to a section of a file;
- lists of rules, such as the list of general rules in the code style checklist of
  [Component](Component.md);
- names of classes, functions, fields, files and folders mentioned in text or in code
  examples;
- terms defined in one file and used in others.

This holds in both directions: a new file is linked from where a reader would look for it,
and a removed file leaves no link behind.

## Conformance

The code must do what the rules say.

After a rule is changed or added, the code is checked against it, all of the code it
applies to and not only the code that prompted it. Code that does not conform is brought
in line. A rule that the code does not follow is not a rule yet.

Pending: the code has not yet been brought in line with [Code style](Codestyle.md) or with
U.S. English. Until it is, code that breaks those rules is not changed unless the user asks.

# Code changes

A code change is a change to `src/` or `tests/`.

Every change, however small, is followed by five checks. A change is not finished until
they are done.

## Rules

A code change does not contradict the rules. Before code is written, the rules that apply
to it are read; after it is written, it is checked against them.

When a change cannot be made without contradicting a rule, the code is not written against
the rule and the rule is not bent quietly. The question is raised: either the change is
made another way, or the rule is changed first, as a change to the rules with its own
checks.

## Tests

Everything is covered with tests. A change is not finished until what it adds or changes
is tested and the whole suite passes.

## Consistency

The change does not leave the code doing one thing in two ways.

What was already written the old way is brought in line, or the change follows the way
that is there.

## Redundancy

The change does not add what already exists.

A function, a type or a constant that is already there is used. When the new one is
better, it replaces the old one and the old one is removed. Code that the change makes
unnecessary is deleted.

Exception: each platform backend (the `_GLES`, `_MacOS`, `_SDL3` and `_Win32` files of one
module) keeps its own copy of the code the backends have in common, as `Painter_GLES.cpp`,
`Painter_MacOS.mm`, `Painter_SDL3.cpp` and `Painter_Win32.cpp` do. Neither Consistency nor
Redundancy above applies to these copies, but a change to what the backends do in common
is made in every backend, so they keep behaving alike.

## Integrity

Nothing is left referring to what no longer exists.

After something is renamed, moved or removed, every use of it is fixed: in the code, in
the tests, in the build files and in `md/`, where names of classes, functions and files
appear in text and in examples.
