# Offensive Infrastructure Lab

`sec-infra-offense` follows Security Investigation Labs and uses the existing Go harness runner. The eight lessons emphasize active probes and verified state changes rather than static finding extraction. Every service, credential, file, identity, account and unit is a fictional in-memory object. `Lab.Send` is an injected callback; its HTTP-like methods and status codes do not use HTTP, sockets, a shell or real system administration.

| Lesson | Learner action | Evidence and explanation |
| --- | --- | --- |
| Exposed store | Enumerate only scoped ports, list keys and read a protected marker without credentials. | Banners guide discovery; returned data proves unauthorized read. A hardened store uses the same banner. |
| Management login | Try only a supplied bounded dictionary, obtain a token and request a protected management marker. | A successful login can yield a low-privilege session. Verify authorization with the issued token. |
| Outdated agent | Inject a second command into an invented old agent's tiny command language. | A separate marker read proves execution in target memory. Old version text alone does not prove the bug. Patched controls can advertise the old version. |
| Artifact escape | Construct a raw path that passes a naive prefix check but normalizes outside the download root. | A fresh protected-file marker proves traversal. Reflected paths and success-looking replies do not. |
| Backup escalation | Rewrite a writable root-job configuration, trigger the job and access the model's root-only marker. | Correlate ownership, write access and execution. Restore the original configuration and drop session privilege, verifying both by fresh reads. |
| Account persistence | Create a privileged account omitted by the normal listing, then authenticate through a new session. | The full audit view must show the account. Delete it and verify both login rejection and existing-session revocation. |
| Service persistence | Create an enabled root unit, simulate reboot and observe the unit's marker. | Enabled and running are different states. Stop and remove the unit, then reboot again to verify no persistent effect remains. |
| Chained compromise | Discover a backup leak, retrieve its service token and use it on an in-scope management service. | Scope does not expand because a target provides another port. A protected snapshot read proves the whole chain. |

## Hidden accounts and services

The account lesson's Hidden property controls only the fictional management application's normal listing. Its full audit listing always includes the account. It is not a recipe for making a real Linux account invisible. UID 0 supplies the model's privileged identity; it does not create a host user. The service lesson models enablement, running state and reboot; it does not install systemd units or run commands on the host. A proof establishes behavior in this explicit model, not an undocumented claim about a real distribution or product.

The job commands `grant-root` and `emit <marker>` are inert interpreter operations on test state. There is no process execution, payload deployment or native exploitation. Software names and versions are fictional, with no real CVE claims. These boundaries keep learners focused on permission failures and evidence while using the existing networkless sandbox unchanged.

## Final challenge: repair the entire compromise chain

After the capstone passes, the optional challenge asks learners to confirm compromise, rotate the leaked token, verify the old token is rejected, confirm the replacement still works, remove the leaked backup value and verify its absence. Negative controls include rotation failure, an old token that still works, broken replacement access and deletion that reports success while preserving the leak. The solver must still attempt the remaining repair steps after a failed verification when a valid replacement token exists.

This is harder than the base lesson because exposure, credential validity and legitimate service availability are independent properties. Checking all three avoids claiming a complete repair based on one success response.

## Validation and maintenance

Run `npm run verify:content -- sec-infra-offense` and `npm run check:ids`. The verifier requires each reference solution to pass and each starter to compile but fail. It also verifies the optional challenge is not already solved by the base lesson solution. Target state machines vary ports, users, keys, markers and credentials across scenarios. Vulnerable and hardened cases share the same API contracts, and cleanup failures must produce a false verdict. Test callbacks reject operations outside the supplied scope.

No learner-language extension, runner privilege change, public network target or deployment is needed. Curriculum loading discovers the track and its final challenge automatically. The existing same-process verdict limitations in `docs/security.md` continue to apply.
