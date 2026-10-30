**Track challenge: prove the chain, rotate the secret and close the leak.**

The capstone proves unauthorized access. Extend it to verify a complete repair rather than reporting that a configuration was changed. Keep the supplied functions and add:

```go
func ChainAndRotate(lab Lab, ports []int) (Finding, bool)
```

Discover and confirm a chain using the lesson's protocol and scope checks. If no chain works, return zero Finding,false. Once a snapshot proof is confirmed, perform the following steps in order; do not move on to a different candidate after starting repair:

1. POST `/admin/rotate` on the management port, using the leaked token. Require 200 with a nonempty replacement Token different from the leaked token. The model invalidates the old token and grants equivalent access to the replacement.
2. GET `/admin/snapshot` with the old token and require 401 or 403. Even a non-marker 200 is not sufficient proof of revocation.
3. GET `/admin/snapshot` with the replacement token and require 200 with **the same original proof body**. Rotation should retain legitimate access.
4. DELETE `/public/value/<TokenKey>` on the backup port, using the replacement token. This cross-service authorization is explicitly part of the toy model. Regardless of its returned status, GET that public value without a token and require 404.

Once a valid replacement token is received, attempt all verification and deletion steps even if one check fails. Return the original Finding,true only when every verification succeeds. If rotation returns an invalid response, stop with false; no valid repair credential is available. The new token is never included in the Finding or printed. The model has no concurrent requests. Do not create accounts or services in this challenge.

**Why this is harder:** fixing a leak and rotating a credential address different parts of the compromise. A service may claim rotation succeeded while the old token remains valid, or claim deletion while the backup still serves it. Independent reads establish both access revocation and availability for the new credential. A 200 response alone is not an authorization guarantee.

**What the tests establish:** scope preservation, varying leaked keys and tokens, vulnerable/hardened paths, failed rotation, continued old-token access, broken replacement access, success-looking deletion failures and mandatory repair attempts after verification failures. These changes affect only the fictional lab state.
