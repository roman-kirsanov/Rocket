# Rocket

Declarative native UI for modern C++.

## Responses

- Keep responses concise and to the point - unless the user asks otherwise

## Planning mode

- Always ask clarifying questions
- Never assume design, tech stack or features
- Use deep-dive sub-agents to assist with research
- Use deep-dive sub-agents to review the different aspects of your plan before presenting to the user

## Git

- Never run a git command that changes anything yourself (commit, push, revert, reset, checkout, fetch, pull, merge, stash, branch or tag changes, submodule updates, `git config`) - in this repo or in the `lib/` submodules - unless the user explicitly asks for it in their current message
- Read-only git is allowed without asking: `status`, `log`, `diff`, `show`, `rev-parse`, `ls-files`, `blame`, listing branches and remotes
- An earlier request or approval in the chat does not carry over: every git action needs a fresh, explicit instruction
- The same applies to sub-agents - pass this rule on when delegating
- When git work is needed, give the user the exact commands and stop
