import { getTracks, lessonPath } from "@/lib/curriculum";
import { DIFFICULTIES } from "@/lib/gamification";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

// https://llmstxt.org — a plain-markdown map of the site for language models.
export function GET() {
  const tracks = getTracks();
  const body = [
    `# ${SITE.name}`,
    "",
    `> ${SITE.description}`,
    "",
    "Baremetal is a free, browser-based course platform for learning low-level programming. Learners read short lessons, solve coding exercises in C, Rust, Go or x86-64 assembly that are compiled and tested in a sandbox, and can ask an AI tutor that guides them without giving away answers. Courses are meant to be taken in order.",
    "",
    ...tracks.flatMap((t, i) => [
      `## Course ${i + 1}: ${t.title} (${t.level}, ${LANGUAGE_LABEL[t.language]})`,
      "",
      t.description,
      "",
      `- [${t.title} overview](${absoluteUrl(`/learn/${t.slug}`)}): ${t.summary}`,
      ...t.lessons.map((l) => `- [${l.title}](${absoluteUrl(lessonPath(t.slug, l.slug))}) (${DIFFICULTIES[l.difficulty].label}): ${l.summary}`),
      "",
    ]),
    "## Optional",
    "",
    `- [Terms of Service](${absoluteUrl("/terms")})`,
    `- [Privacy Notice](${absoluteUrl("/privacy")})`,
    `- [Cookie Policy](${absoluteUrl("/cookies")})`,
    "",
  ].join("\n");
  return new Response(body, { headers: { "Content-Type": "text/plain; charset=utf-8", "Cache-Control": "public, max-age=3600" } });
}
