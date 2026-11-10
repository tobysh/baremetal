import { defineConfig } from "@playwright/test";

export default defineConfig({
  testDir: "./scripts/browser",
  workers: 1,
  timeout: 45_000,
  expect: { timeout: 10_000 },
  retries: 0,
  reporter: "list",
  use: {
    // These tests mock API responses; service-worker fetches can bypass page.route.
    // The separate production CSP suite validates real service-worker registration.
    serviceWorkers: "block",
    baseURL: process.env.TEST_BASE_URL ?? "http://127.0.0.1:3100",
    browserName: "chromium",
    trace: "retain-on-failure",
    launchOptions: {
      ...(process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE ? { executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE } : {}),
      // Trust only the isolated harness certificate, including service-worker fetches.
      args: process.env.BM_REGRESSION_HTTPS === "1" && process.env.BM_REGRESSION_TLS_SPKI
        ? [`--ignore-certificate-errors-spki-list=${process.env.BM_REGRESSION_TLS_SPKI}`] : [],
    },
  },
});
