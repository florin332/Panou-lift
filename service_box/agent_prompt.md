
Execute the following task from service_box/todo.md:

[TASK NAME / TODO ITEM]

Before making any changes:
- read and follow service_box/AGENTS.md;
- inspect the relevant parts of service_box/ui_requirements.md;
- inspect service_box/svcbox_menu.drawio when the task involves UI or navigation;
- inspect service_box/hardware_map.md when the task involves hardware;
- inspect the existing implementation before modifying it.

Treat AGENTS.md, hardware_map.md and svcbox_menu.drawio as protected documentation
according to AGENTS.md.

Do not guess missing requirements or hardware details.

Work only within the boundaries of the requested TODO task.
Preserve existing working functionality and make the minimum necessary changes.

If the task is too complex to execute or verify as a single unit, do not
restructure todo.md autonomously. Propose temporary task granularization first,
according to section 35 of AGENTS.md.

If implementation conflicts with protected documentation or an authoritative
requirement:
- stop before modifying the documentation;
- report the discrepancy;
- add an Agent Proposal according to AGENTS.md;
- wait for human decision if required.

Do not automatically build after every modification.
Build only when appropriate according to AGENTS.md and do not repeatedly retry
builds automatically.

At completion, report:
- files modified;
- implementation completed;
- tests/verification performed;
- build status;
- hardware verification status;
- remaining issues or blockers;
- any Agent Proposal created.