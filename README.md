amanda
======

Functional programming language implemented in C, initially written by
Dick Bruin.

original fork message (acda6a917f)
======

Completely barebones version of Amanda as provided by Dick Bruin.

This version provided by Dick Bruin is free of any rights. Exact quote:

"onder geen enkele licentie.
Het staat ze vrij te veranderen wat ze willen. Als ze het publiceren,
dan graag vermelden dat er geen rechten aan kunnen worden verbonden.
Mijn naam hoeft niet genoemd te worden."

This fork will only accept contributions licensed (or not) under an
(un)license that has yet to be decided. If a completely public domain,
unedited version of Amanda is desired, this commit is the desired commit
to fork.

Java GUI removed. Any UI implementations of Amanda should not have to
re-implement Amanda proper. Any UI implementations henceforth should be
their own project, and should have Amanda as a dependency rather than as
an extra component.

Several improvements have been applied to Amanda by students of NHL
Hogeschool. This version - insofar I am concerned - does not contain any
of those patches. These patches may be reapplied upon further
inspection, under the assumption that their patches were public domain
contributions.

This repository will (most likely) be abandoned by the end of the school
year (Summer 2014). Two teams consisting of approximately four members
each will be working on this repository for three months. One team will
work on the core. The other team will implement additional
functionality.

Future implementations of Amanda should preferably fork from the last
commit to this repository. Future IDE implementations of Amanda should
treat Amanda as a separate dependency for their program, and start a new
blank repository. In case core Amanda tweaks are necessary to implement
IDE functionality, such a team should both fork from the last commit to
this repository (where the core patches will be applied) and create a
new blank repository (where the IDE will be developed from scratch). The
same applies to any other non-core projects involving Amanda, unless
such projects aim to create new standard libraries. Sensibility is
required in that decision-making.
