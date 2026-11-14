// Literal colour values for the few places that cannot use CSS variables: the web manifest, the
// viewport themeColor, and global-error.tsx (rendered outside the root layout, so no stylesheet).
// scripts/check-theme.mts verifies these match the tokens in src/app/globals.css.
export const THEME_COLORS = {
  dark: { bg: "#0c0d0f", text: "#ecebe8", muted: "#9aa3b2", accent: "#f5a524", accentInk: "#1a1203" },
  light: { bg: "#fbfaf7" },
} as const;
