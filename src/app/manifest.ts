import type { MetadataRoute } from "next";
import { SITE } from "@/lib/site";
import { THEME_COLORS } from "@/lib/theme-colors";

export default function manifest(): MetadataRoute.Manifest {
  return {
    name: `${SITE.name} — Learn low-level programming`,
    short_name: SITE.name,
    description: SITE.description,
    id: "/",
    start_url: "/dashboard",
    scope: "/",
    orientation: "any",
    categories: ["education"],
    display: "standalone",
    background_color: THEME_COLORS.dark.bg,
    theme_color: THEME_COLORS.dark.bg,
    icons: [
      { src: "/icon-192.png", sizes: "192x192", type: "image/png" },
      { src: "/icon-512.png", sizes: "512x512", type: "image/png" },
      { src: "/icon.svg", sizes: "any", type: "image/svg+xml" },
    ],
  };
}
