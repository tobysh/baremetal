Pass the decoded capsule to LoadCapsule, not the original XOR envelope.
===
Entry is already an instruction index. Slice at that index and pass the caller’s budgets to Run; return Trace{} on earlier errors.
