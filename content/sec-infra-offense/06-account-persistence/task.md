Implement `ExerciseAccount(lab Lab, port int, adminToken string, account Account) (Finding, bool)`. account is a fresh, valid lab username/password with UID 0 and Hidden true. Use encoding/json for Account; names contain letters, digits and hyphens only.

- POST `/accounts` using adminToken and the JSON account. A 201 means it was created; otherwise stop with false.
- GET `/accounts/all` and `/accounts/visible` with adminToken; their 200 Items are usernames. Confirm the name is present in all but absent from visible.
- POST `/login` without a token and JSON `{"user":account.Name,"password":account.Password}`. On 200 with nonempty Token, GET `/root/proof` with that new token. Only 200 `LAB-PROOF:` body proves administrative access.
- After every successful creation, DELETE `/accounts/<name>` with adminToken, then GET `/accounts/all` to confirm absence. Repeat the login and require 401. If an account session was issued, GET `/root/proof` with it and require 401 or 403.

Return Finding,true only if all persistence evidence and verified cleanup succeed; otherwise zero Finding,false. Attempt cleanup after accepted creation even if verification fails. These are the only allowed requests. Deletion revokes the toy user's active sessions. The original administrative token is not a persistence finding.
