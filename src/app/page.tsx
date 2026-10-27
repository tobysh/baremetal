import type { Metadata } from "next";
import Link from "next/link";
import { Fragment, type CSSProperties } from "react";
import { redirect } from "next/navigation";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { Badge, ButtonLink, Card, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { PATHS } from "@/lib/curriculum/paths";
import { SITE, absoluteUrl } from "@/lib/site";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

export const metadata: Metadata = {
  title: { absolute: `${SITE.name} — Learn C, Rust, Go and assembly from zero, with an AI tutor` },
  description:
    "Learn low-level programming from absolute zero. Write real C, Rust, Go and assembly in your browser, get instant feedback, and build up to your own HTTP server. Free.",
  alternates: { canonical: "/" },
};

const FAQ = [
  {
    q: "I've never written a line of code. Is this for me?",
    a: "Yes. The first course, Programming from Zero, assumes nothing — not even what a program is. When you sign up you tell us your experience, and the AI tutor adjusts how it explains things to match.",
  },
  {
    q: "Why learn C, Rust, Go or assembly instead of Python or JavaScript?",
    a: "They show you what the computer is actually doing: memory, pointers, bytes and system calls. That understanding makes you better in every other language too — and it's what operating systems, browsers and game engines are built with.",
  },
  {
    q: "Does the AI tutor just give me the answers?",
    a: "No, on purpose. It reads your code and your error messages and asks the question that gets you unstuck. You'll learn far more solving it yourself with a nudge than by copying a solution.",
  },
  {
    q: "Do I need to install anything?",
    a: "No. You write code in the browser and we compile and run it in a secure sandbox on our servers, then show you the output and test results in about a second.",
  },
  {
    q: "How much does it cost?",
    a: "It's free. Create an account to save your progress and use the AI tutor.",
  },
];

export default function HomePage() {
  return (
    <Suspense fallback={<Container className="py-12"><Skeleton className="h-10 w-64" /><Skeleton className="mt-6 h-48" /></Container>}>
      <HomeContent />
    </Suspense>
  );
}

async function HomeContent() {
  const user = await getCurrentUser();
  if (user) redirect("/dashboard");

  const tracks = getTracks();
  const featuredTracks = PATHS.flatMap((path) => tracks.filter((track) => track.path === path.id).slice(0, 1));
  const lessonCount = tracks.reduce((n, t) => n + t.lessons.length, 0);

  return (
    <>
      <JsonLd
        data={[
          { "@context": "https://schema.org", "@type": "WebSite", name: SITE.name, url: SITE.url, description: SITE.description },
          { "@context": "https://schema.org", "@type": "Organization", name: SITE.name, url: SITE.url, logo: absoluteUrl("/icon.svg") },
          {
            "@context": "https://schema.org",
            "@type": "FAQPage",
            mainEntity: FAQ.map((f) => ({ "@type": "Question", name: f.q, acceptedAnswer: { "@type": "Answer", text: f.a } })),
          },
        ]}
      />

      {/* Hero: headline and primary CTA sit above the fold on every screen size. */}
      <section className="border-b border-border">
        <Container className="grid items-center gap-10 py-12 sm:py-16 lg:grid-cols-[1.05fr_1fr] lg:py-24">
          <div>
            <p className="text-sm font-medium text-accent">Free · {lessonCount} hands-on lessons · C, Rust, Go, asm</p>
            <h1 aria-label={SLOGAN.join(" ")} className="display mt-5 text-4xl font-bold leading-[1.08] sm:text-5xl lg:text-6xl">
              <TypedSlogan />
            </h1>
            <p className="mt-5 max-w-xl text-lg text-muted">
              Write C, Rust, Go and assembly in your browser. Start with your first program and build up to your own HTTP server, with an AI tutor to help you reason through the hard parts.
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <ButtonLink href="/signup" size="lg">
                Start learning free
              </ButtonLink>
              <ButtonLink href="/learn/first-steps/hello-world" size="lg" variant="secondary">
                Try a lesson first
              </ButtonLink>
            </div>
            <p className="mt-4 text-sm text-muted">No install. No card. Runs in your browser.</p>
          </div>
          <WorkspacePreview />
        </Container>
      </section>

      <section aria-labelledby="how" className="py-16">
        <Container>
          <h2 id="how" className="display-sm text-2xl font-bold sm:text-3xl">
            How it works
          </h2>
          <ol className="mt-8 grid gap-4 md:grid-cols-3">
            {[
              ["Read a short lesson", "Each idea is explained from first principles with small examples — no jargon left undefined."],
              ["Write real code", "Solve an exercise in the in-browser editor. Press Test and hidden tests check your work in about a second."],
              ["Get unstuck, not spoon-fed", "Stuck on an error? The AI tutor sees your code and output and asks the question that helps it click."],
            ].map(([title, body], i) => (
              <li key={title} className="border-t border-border pt-6">
                <span className="font-mono text-sm text-accent">0{i + 1}</span>
                <h3 className="mt-2 font-semibold">{title}</h3>
                <p className="mt-2 leading-relaxed text-muted">{body}</p>
              </li>
            ))}
          </ol>
        </Container>
      </section>

      <section aria-labelledby="courses" className="py-16">
        <Container>
          <div className="flex flex-wrap items-end justify-between gap-4">
            <h2 id="courses" className="display-sm text-2xl font-bold sm:text-3xl">
              Find your starting point
            </h2>
            <Link href="/learn" className="text-sm text-accent underline underline-offset-4">
              See all courses
            </Link>
          </div>
          <ol className="mt-8 grid gap-4 sm:grid-cols-2">
            {featuredTracks.map((t) => (
              <Card as="li" key={t.slug} className="flex flex-col p-6 transition-colors hover:border-accent/40">
                <div className="flex flex-wrap items-center gap-2">
                  <span className="font-mono text-sm text-muted">{PATHS.find((p) => p.id === t.path)?.title}</span>
                  <Badge>{t.level}</Badge>
                  <Badge>{LANGUAGE_LABEL[t.language]}</Badge>
                </div>
                <h3 className="mt-3 text-lg font-semibold">
                  <Link href={`/learn/${t.slug}`} className="hover:text-accent">
                    {t.title}
                  </Link>
                </h3>
                <p className="mt-2 flex-1 text-sm text-muted">{t.summary}</p>
                <p className="mt-4 text-sm text-muted">
                  {t.lessons.length} lessons · e.g. {t.lessons.slice(0, 3).map((l) => l.title.toLowerCase()).join(", ")}
                </p>
              </Card>
            ))}
          </ol>
        </Container>
      </section>

      <section aria-labelledby="faq" className="py-16">
        <Container className="max-w-3xl">
          <h2 id="faq" className="display-sm text-2xl font-bold sm:text-3xl">
            Questions
          </h2>
          <div className="mt-6 divide-y divide-border panel">
            {FAQ.map((f) => (
              <details key={f.q} className="group p-5 sm:p-6">
                <summary className="cursor-pointer list-none font-medium marker:hidden">
                  <span className="flex items-center justify-between gap-4">
                    {f.q}
                    <span aria-hidden="true" className="text-muted transition-transform group-open:rotate-45">
                      +
                    </span>
                  </span>
                </summary>
                <p className="mt-3 text-muted">{f.a}</p>
              </details>
            ))}
          </div>
        </Container>
      </section>

      <section className="py-16">
        <Container>
          <Card className="flex flex-col items-start gap-4 p-8 sm:flex-row sm:items-center sm:justify-between">
            <div>
              <h2 className="display-sm text-xl font-bold">Your first program is five minutes away.</h2>
              <p className="mt-1 text-muted">Create a free account to save progress and talk to the tutor.</p>
            </div>
            <ButtonLink href="/signup" size="lg">
              Create free account
            </ButtonLink>
          </Card>
        </Container>
      </section>
    </>
  );
}

const SLOGAN = ["Learn", "how", "computers", "really", "work."];
const SLOGAN_ACCENT = "really";

/** The headline, typed out by CSS (see .type-char). The h1 carries the plain sentence as its aria-label. */
function TypedSlogan() {
  let i = 0;
  const total = SLOGAN.join(" ").length;
  return (
    <>
      <span aria-hidden="true">
        {SLOGAN.map((word, w) => {
          if (w > 0) i++;
          return (
            <Fragment key={word}>
              {w > 0 && " "}
              <span className={`inline-block ${word === SLOGAN_ACCENT ? "text-accent" : ""}`}>
                {[...word].map((ch) => {
                  const index = i++;
                  return <span key={index} className="type-char" data-last={index === total - 1 ? "" : undefined} style={{ "--i": index } as CSSProperties}>{ch}</span>;
                })}
              </span>
            </Fragment>
          );
        })}
      </span>
    </>
  );
}

/** A static, server-rendered picture of the workspace — no JavaScript or images. */
function WorkspacePreview() {
  return (
    <div className="min-w-0 overflow-hidden panel" aria-label="Example lesson workspace" role="img">
      <div className="flex items-center gap-2 border-b border-border px-4 py-2.5 text-xs text-muted">
        <span className="font-mono">reverse.c</span>
        <span className="ml-auto">Pointers &amp; Memory · Lesson 4</span>
      </div>
      <pre className="overflow-x-auto px-4 py-4 font-mono text-[13px] leading-6">
        <code>
          <span className="text-muted">{"// Reverse a string in place"}</span>
          {"\n"}
          <span className="text-accent">void</span> reverse(<span className="text-accent">char</span> *s) {"{"}
          {"\n"}    <span className="text-accent">size_t</span> i = 0, j = strlen(s);
          {"\n"}    <span className="text-accent">while</span> (i {"<"} j) {"{"}
          {"\n"}        <span className="text-accent">char</span> t = s[i];
          {"\n"}        s[i++] = s[--j];
          {"\n"}        s[j] = t;
          {"\n"}    {"}"}
          {"\n"}
          {"}"}
        </code>
      </pre>
      <div className="space-y-1.5 border-t border-border px-4 py-3 font-mono text-[13px]">
        <p className="text-ok">✓ reverse(&quot;hello&quot;) gives &quot;olleh&quot;</p>
        <p className="text-ok">✓ reverse(&quot;&quot;) leaves an empty string</p>
        <p className="text-err">✗ reverse(&quot;ab&quot;) gives &quot;ba&quot; — got &quot;ab&quot;</p>
      </div>
      <div className="border-t border-border bg-surface-2 px-4 py-3 text-sm">
        <p className="text-xs font-medium text-muted">Tutor</p>
        <p className="mt-1">Nice — two of three pass. Trace the loop by hand for &quot;ab&quot;: what are i and j before the first swap?</p>
      </div>
    </div>
  );
}
