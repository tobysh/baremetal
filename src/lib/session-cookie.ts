/**
 * Cookie names and flags shared by auth, the proxy and the test scripts (no server-only import).
 * Cookies are `Secure` and use the `__Host-` prefix (browser-enforced: Secure, Path=/, no Domain)
 * unless BM_INSECURE_COOKIES=1, an explicit opt-out for plain-HTTP development in browsers that
 * refuse Secure cookies on localhost (Safari). The same flag decides the name and the attribute.
 */
export const SECURE_COOKIES = process.env.BM_INSECURE_COOKIES !== "1";
const prefix = SECURE_COOKIES ? "__Host-" : "";
export const SESSION_COOKIE = `${prefix}bm_session`;
export const PENDING_2FA_COOKIE = `${prefix}bm_2fa_pending`;
