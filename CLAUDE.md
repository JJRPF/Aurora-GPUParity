# CLAUDE.md: Aurora-GPUParity

Research repo for GPU parity with macOS on the M2 Pro (T6020, J414s). See README.md and docs/status.md.

* Every number in docs/ cites a folder in data/. Do not write a claim without its data.
* This machine is the test machine. A boot with a non-default GPU or kernel setting, or anything that
  can wedge the GPU, is logged in docs/worklog.md first and confirmed with the user before it happens.
* Commits here carry the session's `Co-Authored-By` trailer (as Aurora-DeepIdle does). Anything headed
  into an Aurora Silicon repository follows https://aurorasilicon.org/docs/contributing/ instead:
  commits with the human's identity only, no AI trailers; the PR description ends with
  `Assisted-by: LLM (<Vendor>:<Model>)`.
* Aurora's RE rule: reverse-engineering output stays private; published code is clean-room. Keep project
  findings in Aurora-controlled spaces and do not send them to upstream projects.
