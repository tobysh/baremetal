import type { MetadataRoute } from "next";
import { getTracks, lessonPath } from "@/lib/curriculum";
import { absoluteUrl } from "@/lib/site";

export default function sitemap(): MetadataRoute.Sitemap {
  const tracks = getTracks();
  return [
    { url: absoluteUrl("/"), changeFrequency: "weekly", priority: 1 },
    { url: absoluteUrl("/learn"), changeFrequency: "weekly", priority: 0.9 },
    ...tracks.map((t) => ({ url: absoluteUrl(`/learn/${t.slug}`), changeFrequency: "monthly" as const, priority: 0.8 })),
    ...tracks.flatMap((t) => t.lessons.map((l) => ({ url: absoluteUrl(lessonPath(t.slug, l.slug)), changeFrequency: "monthly" as const, priority: 0.7 }))),
    { url: absoluteUrl("/signup"), changeFrequency: "yearly", priority: 0.5 },
    { url: absoluteUrl("/login"), changeFrequency: "yearly", priority: 0.3 },
    { url: absoluteUrl("/terms"), changeFrequency: "yearly", priority: 0.2 },
    { url: absoluteUrl("/privacy"), changeFrequency: "yearly", priority: 0.2 },
    { url: absoluteUrl("/cookies"), changeFrequency: "yearly", priority: 0.2 },
  ];
}
