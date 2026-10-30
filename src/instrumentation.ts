export async function register() {
  if (process.env.NEXT_RUNTIME !== "nodejs") return;
  const hops = process.env.TRUSTED_PROXY_HOPS;
  if ((process.env.NODE_ENV === "production" || hops !== undefined) && (!hops || !/^[1-9][0-9]*$/.test(hops) || !Number.isSafeInteger(Number(hops)))) {
    throw new Error(
      "TRUSTED_PROXY_HOPS is not set. Per-IP rate limits trust X-Forwarded-For, so production needs the number of reverse proxies in front of the app (see docs/deployment-hardening.md).",
    );
  }
  const { assertRunnerConfiguration } = await import("./lib/runner-client");
  assertRunnerConfiguration();
  if (!process.env.RUNNER_SOCKET) {
    const { cleanupStaleWorkdirs } = await import("./lib/runner-engine");
    await cleanupStaleWorkdirs();
  }
  console.log(`[boot] TRUSTED_PROXY_HOPS=${process.env.TRUSTED_PROXY_HOPS ?? "(unset, dev)"}`);
}
