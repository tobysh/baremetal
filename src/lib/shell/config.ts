/** Production shell access requires an explicit operator attestation after host validation. */
export function shellBrokerSocket(env: NodeJS.ProcessEnv = process.env): string | undefined {
  if (env.NODE_ENV === "production" && env.SHELL_HOST_VERIFIED !== "1") return undefined;
  return env.SHELL_BROKER_SOCKET?.trim() || undefined;
}
