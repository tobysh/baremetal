Use the all-accounts audit view to confirm an account omitted by the normal view. Authenticate with the new credentials, not the original token.
===
Deletion is incomplete if a previously issued session still works. Verify both fresh login rejection and session revocation.
===
Order matters: create, compare `all` against `visible`, log in as the new account, read the proof with the *new* token. Only then start cleanup.
===
Keep the login body and the session token from the first login. After deleting, repeat the same login and expect 401, and reuse the old token against `/root/proof` expecting 401 or 403.
