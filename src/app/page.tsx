import type { Metadata } from "next";
import Link from "next/link";
import { Fragment, type CSSProperties } from "react";
import { redirect } from "next/navigation";
import { Suspense } from "react";
import { JsonLd } from "@/components/json-ld";
import { ButtonLink, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { PATHS } from "@/lib/curriculum/paths";
import { SITE, absoluteUrl } from "@/lib/site";

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
  const start = tracks.find((t) => t.slug === "first-steps");
  const others = PATHS.filter((p) => p.id !== "start").filter((p) => tracks.some((t) => t.path === p.id)).map((path) => ({ path }));
  const courseCount = tracks.length;
  const firstMinutes = tracks.find((t) => t.slug === "first-steps")?.lessons[0]?.minutes;
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
      <section className="hero-atmos relative overflow-hidden">
        <Container className={`${W} relative grid items-center gap-10 py-12 sm:py-16 lg:grid-cols-[1.12fr_1fr] lg:gap-12 lg:py-24`}>
          <div>
            <p className="text-sm font-medium text-accent">Free · {lessonCount} hands-on lessons · C, Rust, Go, asm</p>
            <h1 aria-label={SLOGAN.join(" ")} className="display mt-5 text-4xl font-bold leading-[1.06] sm:text-5xl lg:text-[3.1rem] xl:text-[3.5rem]">
              <TypedSlogan />
            </h1>
            <p className="mt-5 max-w-xl text-lg text-muted">
              Write C, Rust, Go and assembly in your browser. Start with your first program and build up to your own HTTP server, with an AI tutor to help you reason through the hard parts.
            </p>
            <div className="mt-8 flex flex-col gap-3 sm:flex-row sm:flex-wrap">
              <ButtonLink href="/signup" size="lg">
                Start learning free
              </ButtonLink>
              <ButtonLink href="/learn/first-steps/hello-world" size="lg" variant="ghost">
                Try a lesson first
              </ButtonLink>
            </div>
            <p className="mt-4 text-sm text-muted">No install. No card. Runs in your browser.</p>
          </div>
          <RunDemo />
        </Container>
      </section>

      <section aria-labelledby="how" className="py-16 sm:py-24">
        <Container className={`${W} grid gap-10 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.1fr)] lg:gap-16`}>
          <div>
            <h2 id="how" className="display-sm text-2xl font-bold sm:text-3xl">
              How it works
            </h2>
            <ol className="mt-8 divide-y divide-border border-y border-border">
              {[
                ["Read a short lesson", "Each idea is explained from first principles with small examples — no jargon left undefined."],
                ["Write real code", "Solve an exercise in the in-browser editor. Press Test and hidden tests check your work in about a second."],
                ["Get unstuck, not spoon-fed", "Stuck on an error? The AI tutor sees your code and output and asks the question that helps it click."],
              ].map(([title, body], i) => (
                <li key={title} className="grid grid-cols-[2.5rem_1fr] gap-x-2 py-6">
                  <span className="font-mono text-sm text-accent">0{i + 1}</span>
                  <div>
                    <h3 className="font-semibold">{title}</h3>
                    <p className="mt-2 leading-relaxed text-muted">{body}</p>
                  </div>
                </li>
              ))}
            </ol>
          </div>
          <div className="self-center">
            <TestOutputSample />
          </div>
        </Container>
      </section>

      <section aria-labelledby="courses" className="border-y border-border bg-surface py-16 sm:py-24">
        <Container className={W}>
          <h2 id="courses" className="display-sm text-2xl font-bold sm:text-3xl">
            Find your starting point
          </h2>
          {start && (
            <div className="mt-8 max-w-3xl">
              <p className="text-sm font-semibold text-accent">New to programming? Begin here</p>
              <h3 className="display mt-2 text-3xl font-bold sm:text-4xl">{start.title}</h3>
              <p className="mt-3 text-lg text-text-2">Start with C fundamentals and write your first working program, with no prior experience assumed.</p>
              <p className="mt-3 text-sm text-muted">{start.level} · {start.lessons.length} lessons · No prior experience required</p>
              <div className="mt-6">
                <ButtonLink href={`/learn/${start.slug}`} size="lg" className="h-auto min-h-12 max-w-full !whitespace-normal py-2 text-center">Start with {start.title}</ButtonLink>
              </div>
            </div>
          )}
          <div className="mt-12 flex flex-wrap items-baseline justify-between gap-x-6 gap-y-1">
            <h3 className="text-sm font-semibold uppercase tracking-wide text-muted">Already coding? Pick a path</h3>
            <Link href="/learn" className="inline-flex min-h-11 items-center text-sm font-medium text-accent underline underline-offset-4">
              See all {courseCount} courses
            </Link>
          </div>
          <ul className="mt-2 grid gap-x-12 border-t border-border sm:grid-cols-2 lg:grid-cols-3">
            {others.map(({ path }) => (
              <li key={path.id} className="border-b border-border">
                <Link href={`/learn#path-${path.id}`} className="group flex min-h-14 items-baseline justify-between gap-4 py-3">
                  <span>
                    <span className="block font-medium group-hover:text-accent">{path.title}</span>
                    <span className="block text-sm text-muted">{path.blurb}</span>
                  </span>
                  <span aria-hidden="true" className="text-muted group-hover:text-accent">→</span>
                </Link>
              </li>
            ))}
          </ul>
        </Container>
      </section>

      <section aria-labelledby="faq" className="py-16 sm:py-24">
        <Container width="narrow">
          <h2 id="faq" className="display-sm text-2xl font-bold sm:text-3xl">
            Questions
          </h2>
          <div className="mt-6 divide-y divide-border border-y border-border">
            {FAQ.map((f) => (
              <details key={f.q} className="group">
                <summary className="flex min-h-14 cursor-pointer list-none items-center justify-between gap-4 py-4 font-medium marker:hidden [&::-webkit-details-marker]:hidden">
                  {f.q}
                  <span aria-hidden="true" className="text-muted transition-transform group-open:rotate-45">+</span>
                </summary>
                <p className="pb-5 text-muted">{f.a}</p>
              </details>
            ))}
          </div>
        </Container>
      </section>

      <section aria-labelledby="final" className="cta-glow relative overflow-hidden border-t border-border py-20 text-center sm:py-28">
        <Container width="narrow" className="relative">
          <h2 id="final" className="display text-3xl font-bold leading-tight sm:text-5xl">{firstMinutes ? `Your first program is ${firstMinutes} minutes away.` : "Your first program is a short lesson away."}</h2>
          <p className="mx-auto mt-4 max-w-md text-lg text-text-2">No installation. No experience required. Create a free account to save progress and talk to the tutor.</p>
          <div className="mt-8">
            <ButtonLink href="/signup" size="lg">Start learning free</ButtonLink>
          </div>
        </Container>
      </section>
    </>
  );
}

/** Shared inner width for the landing sections, a little wider than the default so the hero is not a narrow column. */
const W = "max-w-7xl";

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
              {w > 0 && (w === 3 ? <><span className="lg:hidden"> </span><br className="hidden lg:block" /></> : " ")}
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

const DEMO_SECONDS = 12;
const demoPct = (t: number) => Math.round((t / DEMO_SECONDS) * 10000) / 100;

/** Code lines for the demo. Each is [indent, parts]; a string is plain text, [text] is a keyword. */
const DEMO_CODE: Array<Array<string | [string]>> = [
  [[ "// Reverse a string in place" ]],
  [["void"], " reverse(", ["char"], " *s) {"],
  ["    ", ["size_t"], " i = 0, j = strlen(s);"],
  ["    ", ["while"], " (i < j) {"],
  ["        ", ["char"], " t = s[i];"],
  ["        s[i++] = s[--j];"],
  ["        s[j] = t;"],
  ["    }"],
  ["}"],
];
const DEMO_TESTS = ['reverse("hello") gives "olleh"', 'reverse("") leaves an empty string', 'reverse("ab") gives "ba"'];
const DEMO_DESCRIPTION =
  "Animated example of a lesson. Code for a function that reverses a string is typed into the editor, the Run button is pressed, and three tests appear one by one, all passing: 3 of 3 passed. With reduced motion the finished state is shown without animation.";

/**
 * A looping write, Run, tests-pass demo in plain CSS (no JavaScript, no images). Each moving piece
 * gets its own generated keyframes so it can start at its own moment in one 12 s timeline. The
 * default styles are the finished frame, and the animation is only attached when motion is
 * welcome, so reduced-motion visitors see a static, complete picture. It plays once and
 * pauses under the pointer, then rests on the final frame.
 */
function RunDemo() {
  const codeAt = (i: number) => 0.4 + i * 0.45;
  const runAt = 5.2;
  const testAt = (i: number) => 6.2 + i * 0.6;
  const doneAt = 8.2;
  const reveal = (t: number, d: number, hidden: string, shown: string) =>
    `0%,${demoPct(t)}%{${hidden}}${demoPct(t + d)}%,100%{${shown}}`;
  const frames: string[] = [];
  DEMO_CODE.forEach((_, i) => frames.push(`@keyframes demo-c${i}{${reveal(codeAt(i), 0.4, "clip-path:inset(0 100% 0 0)", "clip-path:inset(0 0 0 0)")}}`));
  DEMO_TESTS.forEach((_, i) => frames.push(`@keyframes demo-t${i}{${reveal(testAt(i), 0.15, "opacity:0", "opacity:1")}}`));
  frames.push(`@keyframes demo-done{${reveal(doneAt, 0.15, "opacity:0", "opacity:1")}}`);
  frames.push(`@keyframes demo-run{0%,${demoPct(runAt)}%{background:var(--surface-2);transform:none}${demoPct(runAt + 0.15)}%{background:var(--accent);color:var(--accent-ink);transform:translateY(1px)}${demoPct(runAt + 0.9)}%,100%{background:var(--surface-2);color:var(--text);transform:none}}`);
  frames.push(`@keyframes demo-ph{0%,${demoPct(runAt)}%{opacity:1}${demoPct(runAt + 0.15)}%,100%{opacity:0}}`);
  frames.push(`@keyframes demo-reset{0%,95%{opacity:1}98%,99.9%{opacity:0}100%{opacity:1}}`);
  return (
    <figure className="min-w-0">
    <div className="overflow-hidden panel shadow-2xl shadow-black/30 ring-1 ring-accent/10" role="img" aria-label={DEMO_DESCRIPTION}>
      <style>{frames.join("")}</style>
      <div aria-hidden="true" className="demo">
        <div className="flex items-center gap-2 border-b border-border px-4 py-2.5 text-xs text-muted">
          <span className="font-mono">reverse.c</span>
          <span className="ml-auto hidden sm:inline">Example, not live</span>
          <span className="demo-run ml-auto rounded-md border border-line-strong bg-surface-2 px-3 py-1 font-medium text-text sm:ml-3" style={{ animationName: "demo-run" }}>
            Run ▸
          </span>
        </div>
        <div className="demo-reset" style={{ animationName: "demo-reset" }}>
          <pre className="overflow-x-auto px-4 py-4 font-mono text-sm leading-7" style={{ background: "var(--code-bg)", color: "var(--code-text)" }}>
            <code>
              {DEMO_CODE.map((line, i) => (
                <span key={i} className="demo-a block whitespace-pre" style={{ animationName: `demo-c${i}` }}>
                  {line.map((part, k) => (typeof part === "string" ? part : <span key={k} style={{ color: i === 0 ? "var(--code-comment)" : part[0] === "void" || part[0] === "char" || part[0] === "size_t" ? "var(--code-type)" : "var(--code-keyword)" }}>{part[0]}</span>))}
                  {"\u200b"}
                </span>
              ))}
            </code>
          </pre>
          <div className="relative min-h-[8.5rem] space-y-1.5 border-t border-border px-4 py-3 font-mono text-[13px]">
            <p className="demo-ph absolute opacity-0 inset-x-4 top-3 text-muted">Press Run to check the code against the tests.</p>
            {DEMO_TESTS.map((t, i) => (
              <p key={t} className="demo-a text-ok" style={{ animationName: `demo-t${i}` }}>✓ {t}</p>
            ))}
          </div>
          <div className="demo-a border-t border-border bg-surface-2 px-4 py-3 text-sm font-medium text-ok" style={{ animationName: "demo-done" }}>
            3 of 3 tests passed
          </div>
        </div>
      </div>
    </div>
    <figcaption className="mt-3 text-sm text-muted">An illustrative walkthrough, not a live run. <Link href="/learn/first-steps/hello-world" className="text-accent underline underline-offset-4">Try a real lesson</Link> to run your own code.</figcaption>
    </figure>
  );
}

/** Static, obviously illustrative: the loop from a failed check to the next step. Mirrors real behaviour (hidden tests, hints, tutor questions). */
function TestOutputSample() {
  return (
    <figure aria-label="Example of test feedback (illustrative)">
      <div className="overflow-hidden rounded-lg border border-border" style={{ background: "var(--code-bg)", color: "var(--code-text)" }}>
        <pre className="overflow-x-auto px-4 py-4 font-mono text-sm leading-7">
          <span style={{ color: "var(--code-comment)" }}>{"Test  (example)\n"}</span>
          <span className="text-ok">PASS</span>{" empty input\n"}
          <span className="text-ok">PASS</span>{" single word\n"}
          <span className="text-err">FAIL</span>{" two words: expected 5, got 4\n"}
        </pre>
        <div className="space-y-1.5 border-t border-border px-4 py-3 text-sm">
          <p><span className="font-semibold text-accent">Next:</span> <span className="text-text-2">Open a hint, or ask the tutor. It reads your code and this output.</span></p>
          <p className="text-muted">Tutor: &ldquo;What happens to your counter when the loop reaches the space between the words?&rdquo;</p>
        </div>
      </div>
      <figcaption className="mt-2 text-sm text-muted">Illustrative feedback. Two checks pass, one fails, and you get a nudge rather than the answer.</figcaption>
    </figure>
  );
}
